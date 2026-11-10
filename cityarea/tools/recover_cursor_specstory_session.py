#!/usr/bin/env python3
"""Build an offline Cursor-session replay from SpecStory history files."""

from __future__ import annotations

import argparse
import hashlib
import html
import json
import re
import zipfile
from dataclasses import asdict, dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Iterable
from urllib.parse import quote


MESSAGE_RE = re.compile(r"(?m)^_\*\*(User|Assistant) \(([^\r\n]+)\)\*\*_\s*$")
SESSION_RE = re.compile(r"<!-- cursor Session ([^ ]+) \(([^)]+)\) -->")
TITLE_RE = re.compile(r"(?m)^# (.+)$")
EDIT_RE = re.compile(
    r"<details><summary>Edit file: (.*?)</summary>(.*?)</details>", re.DOTALL
)
DIFF_RE = re.compile(r"```diff\s*\r?\n(.*?)```", re.DOTALL)
CHUNK_RE = re.compile(
    r"\*\*Chunk (\d+)\*\*\s*\r?\nLines added: (\d+), lines removed: (\d+)",
    re.MULTILINE,
)
THOUGHT_RE = re.compile(
    r"<think><details><summary>Thought Process</summary>(.*?)</details></think>",
    re.DOTALL,
)
TOOL_RE = re.compile(r"(?m)^Tool use: \*\*([^*]+)\*\*\s*$")


@dataclass
class Edit:
    file: str
    chunks: list[dict[str, int]] = field(default_factory=list)
    diffs: list[str] = field(default_factory=list)
    source_offset: int = 0


@dataclass
class Message:
    role: str
    label: str
    content: str
    edits: list[Edit] = field(default_factory=list)
    thought_count: int = 0
    tools: list[str] = field(default_factory=list)


@dataclass
class Session:
    source: str
    session_id: str
    timestamp: str
    title: str
    messages: list[Message]


def read_session(path: Path) -> Session:
    text = path.read_text(encoding="utf-8", errors="replace")
    session_match = SESSION_RE.search(text)
    title_match = TITLE_RE.search(text)
    matches = list(MESSAGE_RE.finditer(text))
    messages: list[Message] = []

    for index, match in enumerate(matches):
        end = matches[index + 1].start() if index + 1 < len(matches) else len(text)
        content = text[match.end() : end].strip()
        edits: list[Edit] = []
        for edit_match in EDIT_RE.finditer(content):
            body = edit_match.group(2)
            chunks = [
                {
                    "number": int(chunk.group(1)),
                    "added": int(chunk.group(2)),
                    "removed": int(chunk.group(3)),
                }
                for chunk in CHUNK_RE.finditer(body)
            ]
            edits.append(
                Edit(
                    file=html.unescape(edit_match.group(1).strip()),
                    chunks=chunks,
                    diffs=[diff.rstrip() for diff in DIFF_RE.findall(body)],
                    source_offset=edit_match.start(),
                )
            )

        messages.append(
            Message(
                role=match.group(1).lower(),
                label=match.group(2),
                content=content,
                edits=edits,
                thought_count=len(THOUGHT_RE.findall(content)),
                tools=TOOL_RE.findall(content),
            )
        )

    return Session(
        source=str(path.resolve()),
        session_id=session_match.group(1) if session_match else path.stem,
        timestamp=session_match.group(2) if session_match else "unknown",
        title=title_match.group(1) if title_match else path.stem,
        messages=messages,
    )


def file_uri(project: Path, value: str) -> str:
    candidate = Path(value)
    if not candidate.is_absolute():
        candidate = project / candidate
    return "file:///" + quote(candidate.resolve().as_posix(), safe="/:@")


def diff_html(diff: str) -> str:
    lines = []
    for line in diff.splitlines():
        css = "ctx"
        if line.startswith("+") and not line.startswith("+++"):
            css = "add"
        elif line.startswith("-") and not line.startswith("---"):
            css = "del"
        elif line.startswith("@@"):
            css = "hunk"
        lines.append(f'<span class="{css}">{html.escape(line)}</span>')
    return "\n".join(lines)


def strip_edit_blocks(content: str) -> str:
    return EDIT_RE.sub("\n[Edited file block shown below]\n", content).strip()


def render_edit(edit: Edit, project: Path, edit_id: str) -> str:
    added = sum(chunk["added"] for chunk in edit.chunks)
    removed = sum(chunk["removed"] for chunk in edit.chunks)
    stats = f'<span class="plus">+{added}</span> <span class="minus">-{removed}</span>'
    if not edit.chunks:
        stats = '<span class="muted">stats unavailable</span>'
    diffs = "".join(
        f'<pre class="diff">{diff_html(diff)}</pre>' for diff in edit.diffs
    ) or '<div class="empty">No diff body was captured for this edit.</div>'
    return f"""
    <details class="edit" id="{edit_id}">
      <summary><span class="edited">Edited</span><span class="file-name">{html.escape(edit.file)}</span><span class="stats">{stats}</span></summary>
      <div class="edit-actions"><a href="{file_uri(project, edit.file)}">Open current file</a><button type="button" data-copy="{edit_id}">Copy diff</button></div>
      <div class="diffs">{diffs}</div>
    </details>"""


def render_message(message: Message, project: Path, session_index: int, message_index: int) -> str:
    message_id = f"s{session_index}-m{message_index}"
    edits = "".join(
        render_edit(edit, project, f"{message_id}-e{edit_index}")
        for edit_index, edit in enumerate(message.edits, 1)
    )
    tool_summary = ""
    if message.tools:
        tool_summary = (
            '<div class="tool-summary"><strong>Tools:</strong> '
            + ", ".join(html.escape(tool) for tool in message.tools)
            + "</div>"
        )
    body = html.escape(strip_edit_blocks(message.content))
    return f"""
    <article class="message {message.role}" id="{message_id}" data-search="{html.escape(message.content.lower(), quote=True)}">
      <header><span class="role">{message.role.title()}</span><span class="label">{html.escape(message.label)}</span><span class="counts">{len(message.edits)} edits · {message.thought_count} thoughts</span></header>
      {tool_summary}
      <pre class="message-body">{body}</pre>
      <section class="edit-list">{edits}</section>
    </article>"""


def build_html(sessions: Iterable[Session], project: Path) -> str:
    sessions = list(sessions)
    nav = []
    content = []
    total_edits = sum(len(message.edits) for session in sessions for message in session.messages)
    total_diffs = sum(
        len(edit.diffs)
        for session in sessions
        for message in session.messages
        for edit in message.edits
    )

    for session_index, session in enumerate(sessions, 1):
        nav_messages = []
        rendered_messages = []
        for message_index, message in enumerate(session.messages, 1):
            message_id = f"s{session_index}-m{message_index}"
            preview = re.sub(r"\s+", " ", strip_edit_blocks(message.content))[:72]
            nav_messages.append(
                f'<a href="#{message_id}"><span>{message.role.title()}</span>{html.escape(preview)}</a>'
            )
            rendered_messages.append(
                render_message(message, project, session_index, message_index)
            )
        nav.append(
            f'<section><h2>{html.escape(session.timestamp)}</h2>{"".join(nav_messages)}</section>'
        )
        content.append(
            f"""
            <section class="session" id="session-{session_index}">
              <div class="session-head"><h1>{html.escape(session.title)}</h1><div><code>{html.escape(session.session_id)}</code> · source: <code>{html.escape(session.source)}</code></div></div>
              {''.join(rendered_messages)}
            </section>"""
        )

    generated = datetime.now().astimezone().isoformat(timespec="seconds")
    return f"""<!doctype html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Recovered Cursor Session - traffic-simulation-de-master</title>
<style>
:root {{ color-scheme: dark; --bg:#0d1117; --panel:#161b22; --line:#30363d; --text:#e6edf3; --muted:#8b949e; --blue:#58a6ff; --green:#3fb950; --red:#f85149; }}
* {{ box-sizing:border-box; }}
html {{ scroll-behavior:smooth; }}
body {{ margin:0; background:var(--bg); color:var(--text); font:14px/1.55 "Segoe UI",Arial,sans-serif; letter-spacing:0; }}
.layout {{ display:grid; grid-template-columns:320px minmax(0,1fr); min-height:100vh; }}
aside {{ position:sticky; top:0; height:100vh; overflow:auto; border-right:1px solid var(--line); background:#0b0f14; padding:16px; }}
.brand {{ font-size:18px; font-weight:700; }}
.meta,.label,.counts,.muted {{ color:var(--muted); }}
.meta {{ margin:5px 0 14px; font-size:12px; }}
#search {{ width:100%; padding:9px 10px; color:var(--text); background:var(--panel); border:1px solid var(--line); border-radius:6px; }}
aside section {{ margin-top:18px; }}
aside h2 {{ font-size:12px; color:var(--muted); }}
aside a {{ display:block; color:var(--text); text-decoration:none; padding:7px 8px; border-radius:5px; white-space:nowrap; overflow:hidden; text-overflow:ellipsis; }}
aside a:hover {{ background:var(--panel); }}
aside a span {{ display:inline-block; width:66px; color:var(--blue); font-weight:600; }}
main {{ min-width:0; padding:22px clamp(14px,3vw,42px) 80px; }}
.session {{ max-width:1280px; margin:auto; }}
.session + .session {{ margin-top:55px; }}
.session-head {{ border-bottom:1px solid var(--line); padding-bottom:14px; margin-bottom:18px; }}
.session-head h1 {{ font-size:23px; margin:0 0 5px; }}
.session-head div {{ color:var(--muted); font-size:12px; overflow-wrap:anywhere; }}
.message {{ border:1px solid var(--line); background:var(--panel); margin:12px 0; border-radius:7px; overflow:hidden; }}
.message.user {{ border-left:3px solid var(--blue); }}
.message.assistant {{ border-left:3px solid var(--green); }}
.message header {{ display:flex; gap:10px; align-items:center; padding:10px 12px; border-bottom:1px solid var(--line); }}
.role {{ font-weight:700; }}
.counts {{ margin-left:auto; font-size:12px; }}
.message-body {{ margin:0; padding:14px; white-space:pre-wrap; overflow-wrap:anywhere; font:13px/1.55 Consolas,"Microsoft YaHei UI",monospace; background:#10151c; max-height:620px; overflow:auto; }}
.tool-summary {{ padding:8px 12px; color:var(--muted); border-bottom:1px solid var(--line); overflow-wrap:anywhere; }}
.edit-list {{ padding:10px; }}
.edit {{ border:1px solid var(--line); border-radius:6px; margin:7px 0; background:#0d1117; }}
.edit summary {{ display:flex; align-items:center; gap:9px; min-height:39px; padding:7px 10px; cursor:pointer; }}
.edited {{ padding:2px 7px; color:white; background:#1f6feb; border-radius:4px; font-size:12px; font-weight:700; }}
.file-name {{ font-family:Consolas,monospace; min-width:0; overflow-wrap:anywhere; }}
.stats {{ margin-left:auto; white-space:nowrap; }}
.plus {{ color:var(--green); }} .minus {{ color:var(--red); }}
.edit-actions {{ display:flex; gap:10px; padding:7px 10px; border-top:1px solid var(--line); }}
.edit-actions a,.edit-actions button {{ color:var(--blue); background:transparent; border:0; padding:0; cursor:pointer; font:inherit; text-decoration:none; }}
.diff {{ margin:0; padding:10px; overflow:auto; white-space:pre; font:12px/1.48 Consolas,monospace; border-top:1px solid var(--line); }}
.diff span {{ display:block; min-height:1.48em; }}
.diff .add {{ color:#aff5b4; background:#033a16; }} .diff .del {{ color:#ffdcd7; background:#67060c; }} .diff .hunk {{ color:#d2a8ff; }}
.empty {{ padding:10px; color:var(--muted); }}
.hidden {{ display:none !important; }}
code {{ font-family:Consolas,monospace; }}
@media (max-width:820px) {{ .layout {{ grid-template-columns:1fr; }} aside {{ position:relative; height:auto; border-right:0; border-bottom:1px solid var(--line); }} main {{ padding-top:16px; }} }}
</style>
</head>
<body>
<div class="layout">
<aside>
  <div class="brand">Recovered Cursor Session</div>
  <div class="meta">Project: {html.escape(str(project))}<br>Generated: {generated}<br>{len(sessions)} sessions · {total_edits} edits · {total_diffs} diff blocks</div>
  <input id="search" type="search" placeholder="Search conversation or file...">
  {''.join(nav)}
</aside>
<main>{''.join(content)}</main>
</div>
<script>
const search = document.getElementById('search');
search.addEventListener('input', () => {{
  const q = search.value.trim().toLowerCase();
  document.querySelectorAll('.message').forEach(el => {{
    el.classList.toggle('hidden', q && !el.dataset.search.includes(q));
  }});
}});
document.querySelectorAll('[data-copy]').forEach(button => button.addEventListener('click', async () => {{
  const edit = document.getElementById(button.dataset.copy);
  const text = [...edit.querySelectorAll('.diff')].map(x => x.innerText).join('\n\n');
  await navigator.clipboard.writeText(text);
  const old = button.textContent; button.textContent = 'Copied'; setTimeout(() => button.textContent = old, 1200);
}}));
</script>
</body>
</html>"""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project", type=Path, required=True)
    parser.add_argument("--history", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()

    sources = sorted(args.history.glob("*.md"))
    if not sources:
        raise SystemExit(f"No SpecStory history files found in {args.history}")
    sessions = [read_session(source) for source in sources]
    args.output_dir.mkdir(parents=True, exist_ok=True)
    html_path = args.output_dir / "cursor-recovered-traffic-simulation-de.html"
    json_path = args.output_dir / "cursor-recovered-traffic-simulation-de.json"
    html_path.write_text(build_html(sessions, args.project), encoding="utf-8")
    json_path.write_text(
        json.dumps([asdict(session) for session in sessions], ensure_ascii=False, indent=2),
        encoding="utf-8",
    )

    def sha256(path: Path) -> str:
        digest = hashlib.sha256()
        with path.open("rb") as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(block)
        return digest.hexdigest()

    manifest_path = args.output_dir / "recovery-manifest.json"
    manifest = {
        "generated_at": datetime.now().astimezone().isoformat(timespec="seconds"),
        "project": str(args.project.resolve()),
        "sources": [
            {"path": str(source.resolve()), "sha256": sha256(source)} for source in sources
        ],
        "outputs": [
            {"path": html_path.name, "sha256": sha256(html_path)},
            {"path": json_path.name, "sha256": sha256(json_path)},
        ],
    }
    manifest_path.write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8"
    )

    zip_path = args.output_dir / "cursor-recovered-traffic-simulation-de.zip"
    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        archive.write(html_path, html_path.name)
        archive.write(json_path, json_path.name)
        archive.write(manifest_path, manifest_path.name)
        archive.write(Path(__file__).resolve(), "tools/" + Path(__file__).name)
        for source in sources:
            archive.write(source, "source-history/" + source.name)

    edit_count = sum(len(message.edits) for session in sessions for message in session.messages)
    diff_count = sum(
        len(edit.diffs)
        for session in sessions
        for message in session.messages
        for edit in message.edits
    )
    print(f"Sessions: {len(sessions)}")
    print(f"Messages: {sum(len(session.messages) for session in sessions)}")
    print(f"Edits: {edit_count}")
    print(f"Diff blocks: {diff_count}")
    print(f"HTML: {html_path.resolve()}")
    print(f"JSON: {json_path.resolve()}")
    print(f"Manifest: {manifest_path.resolve()}")
    print(f"Archive: {zip_path.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
