param(
    [Parameter(Mandatory = $true)]
    [string]$SessionQuery
)

$ErrorActionPreference = "Stop"

$reportDir = Join-Path $PSScriptRoot "..\reports"
if (!(Test-Path $reportDir)) {
    New-Item -ItemType Directory -Path $reportDir | Out-Null
}

$py = @'
import json
import os
import glob
import re
import sys
from datetime import datetime

query = sys.argv[1].strip().lower()
home = os.path.expanduser("~")
sessions_root = os.path.join(home, ".codex", "sessions")
archived_root = os.path.join(home, ".codex", "archived_sessions")

def load_jsonl(path):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                yield json.loads(line)
            except Exception:
                continue

def extract_title(rec):
    t = rec.get("type")
    p = rec.get("payload") or {}
    if t == "session_meta":
        return p.get("title") or p.get("thread_name")
    if t == "thread_renamed":
        return p.get("new_title")
    return None

files = sorted(glob.glob(os.path.join(sessions_root, "**", "*.jsonl"), recursive=True))
files += sorted(glob.glob(os.path.join(archived_root, "**", "*.jsonl"), recursive=True))
best = None
for fp in files:
    score = 0
    matched_title = None
    matched_id = None
    for rec in load_jsonl(fp):
        title = extract_title(rec)
        if rec.get("type") == "session_meta":
            sid = (rec.get("payload") or {}).get("id")
            if sid:
                matched_id = sid
                if query == sid.lower():
                    score = 2000
                    matched_title = title or sid
                    break
        if title:
            lt = title.lower()
            if query == lt:
                score = 1000
                matched_title = title
                break
            if query in lt:
                score = max(score, len(query))
                matched_title = title
    if score == 0 and query in os.path.basename(fp).lower():
        score = 100
        matched_title = matched_title or os.path.basename(fp)
    if matched_title:
        cand = (score, fp, matched_title)
        if best is None or cand[0] > best[0]:
            best = cand

if best is None:
    print("NOT_FOUND")
    sys.exit(0)

_, session_path, found_title = best

cmds = []
tool_calls = []
file_mentions = set()
assistant_msgs = []
user_msgs = []
apply_patches = []

for rec in load_jsonl(session_path):
    t = rec.get("type")
    ts = rec.get("timestamp")
    p = rec.get("payload") or {}

    if t == "response_item":
        it = p.get("type")
        if it == "function_call":
            name = p.get("name")
            args = p.get("arguments")
            tool_calls.append((ts, name, args))
            if name == "shell_command" and isinstance(args, str):
                cmds.append((ts, args))
            if name == "apply_patch" and isinstance(args, str):
                apply_patches.append((ts, args))
        elif it == "message":
            role = p.get("role")
            content = p.get("content") or []
            texts = []
            for c in content:
                if c.get("type") in ("input_text", "output_text"):
                    tx = c.get("text")
                    if tx:
                        texts.append(tx)
            text = "\n".join(texts).strip()
            if text:
                if role == "assistant":
                    assistant_msgs.append((ts, text))
                elif role == "user":
                    user_msgs.append((ts, text))

    if t == "event_msg":
        ep = p.get("aggregated_output") or p.get("stdout") or ""
        if isinstance(ep, str):
            for m in re.findall(r"([A-Za-z]:\\\\[^\\n\\r\\t]+\\.(?:py|cpp|h|hpp|md|json|yml|yaml|txt))", ep):
                file_mentions.add(m)

    # simple extraction from message text
    for bag in (user_msgs[-1][1:] if user_msgs else [], assistant_msgs[-1][1:] if assistant_msgs else []):
        pass

def summarize_patch(patch_text):
    files = []
    for line in patch_text.splitlines():
        if line.startswith("*** Update File: ") or line.startswith("*** Add File: ") or line.startswith("*** Delete File: "):
            files.append(line.split(": ", 1)[1].strip())
    return files

patched_files = set()
for _, ap in apply_patches:
    for f in summarize_patch(ap):
        patched_files.add(f)

out = []
out.append(f"# Session Replay: {found_title}")
out.append("")
out.append(f"- Source file: `{session_path}`")
out.append(f"- Generated at: `{datetime.now().isoformat(timespec='seconds')}`")
out.append("")
out.append("## Key Commands")
if cmds:
    for ts, c in cmds[:120]:
        out.append(f"- `{ts}`")
        out.append(f"```text\n{c}\n```")
else:
    out.append("- None found")
out.append("")
out.append("## Files Patched (apply_patch)")
if patched_files:
    for f in sorted(patched_files):
        out.append(f"- `{f}`")
else:
    out.append("- None found")
out.append("")
out.append("## Other File Mentions")
if file_mentions:
    for f in sorted(file_mentions):
        out.append(f"- `{f}`")
else:
    out.append("- None found")
out.append("")
out.append("## User Messages (excerpt)")
for ts, m in user_msgs[:30]:
    out.append(f"- `{ts}` {m[:280].replace(chr(10), ' ')}")
out.append("")
out.append("## Assistant Messages (excerpt)")
for ts, m in assistant_msgs[:40]:
    out.append(f"- `{ts}` {m[:280].replace(chr(10), ' ')}")

safe_name = re.sub(r"[^A-Za-z0-9._-]+", "_", found_title).strip("_")
if not safe_name:
    safe_name = "session"
report_path = os.path.abspath(os.path.join(os.getcwd(), "reports", f"session-replay-{safe_name}.md"))
with open(report_path, "w", encoding="utf-8") as f:
    f.write("\n".join(out))

print(report_path)
'@

$tmpPy = Join-Path $env:TEMP "replay_codex_session.py"
[System.IO.File]::WriteAllText($tmpPy, $py, [System.Text.UTF8Encoding]::new($false))

$reportPath = & python $tmpPy $SessionQuery
if ($reportPath -eq "NOT_FOUND") {
    Write-Host "Session not found by query: $SessionQuery"
    exit 1
}

Write-Host "Report generated: $reportPath"
