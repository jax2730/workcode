#!/usr/bin/env python3
"""
Export VS Code GitHub Copilot Chat caches for one workspace.

The exporter creates a mini-wiki-style reference archive with:
- readable Markdown pages under wiki/
- complete structured request records under data/
- source-cache metadata under cache/

VS Code and Copilot cache formats are internal. This script is deliberately
defensive and records source paths so the export can be audited later.
"""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import os
import re
import shutil
import sqlite3
from collections import Counter
from pathlib import Path
from typing import Any
from urllib.parse import unquote, urlparse


APPDATA = Path(os.environ["APPDATA"])
VSCODE_USER = APPDATA / "Code" / "User"
WORKSPACE_STORAGE = VSCODE_USER / "workspaceStorage"
GLOBAL_STORAGE = VSCODE_USER / "globalStorage"
EMPTY_WINDOW_SESSIONS = GLOBAL_STORAGE / "emptyWindowChatSessions"


def normalize_path(path: str | Path) -> str:
    return str(Path(path).resolve()).lower().replace("\\", "/")


def file_uri_to_path(uri: str) -> str:
    parsed = urlparse(uri)
    if parsed.scheme != "file":
        return uri
    path = unquote(parsed.path)
    if re.match(r"^/[a-zA-Z]:/", path):
        path = path[1:]
    return path.replace("/", "\\")


def safe_filename(value: str, limit: int = 80) -> str:
    value = re.sub(r"[^\w.-]+", "-", value.strip(), flags=re.UNICODE).strip("-")
    return (value or "untitled")[:limit]


def json_dump(obj: Any, *, indent: int | None = 2) -> str:
    return json.dumps(obj, ensure_ascii=False, indent=indent)


def read_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def ms_to_iso(value: Any) -> str | None:
    if not isinstance(value, (int, float)):
        return None
    try:
        return dt.datetime.fromtimestamp(value / 1000, tz=dt.timezone.utc).isoformat()
    except Exception:
        return None


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def file_record(path: Path, *, include_hash: bool = False) -> dict[str, Any]:
    rec = {
        "path": str(path),
        "size": path.stat().st_size if path.exists() else None,
        "mtime": path.stat().st_mtime if path.exists() else None,
        "mtimeIso": dt.datetime.fromtimestamp(path.stat().st_mtime, tz=dt.timezone.utc).isoformat()
        if path.exists()
        else None,
    }
    if include_hash and path.exists() and path.is_file():
        rec["sha256"] = sha256_file(path)
    return rec


def short(text: Any, limit: int = 700) -> str:
    if text is None:
        return ""
    if not isinstance(text, str):
        text = json.dumps(text, ensure_ascii=False)
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    return text if len(text) <= limit else text[:limit] + "\n...[truncated]"


def md(text: Any) -> str:
    if text is None:
        return ""
    if not isinstance(text, str):
        text = json.dumps(text, ensure_ascii=False)
    return text.replace("\r\n", "\n").replace("\r", "\n")


def uri_fs_path(obj: Any) -> str:
    if isinstance(obj, dict):
        if isinstance(obj.get("fsPath"), str):
            return obj["fsPath"]
        if isinstance(obj.get("path"), str):
            return obj["path"]
        if isinstance(obj.get("external"), str):
            return file_uri_to_path(obj["external"])
        if isinstance(obj.get("uri"), dict):
            return uri_fs_path(obj["uri"])
    if isinstance(obj, str):
        return file_uri_to_path(obj)
    return ""


def find_workspace_storage(workspace: Path) -> Path:
    target = normalize_path(workspace)
    matches: list[Path] = []

    for entry in WORKSPACE_STORAGE.iterdir():
        if not entry.is_dir():
            continue
        meta = entry / "workspace.json"
        if not meta.exists():
            continue
        try:
            data = read_json(meta)
        except Exception:
            continue
        raw = data.get("folder") or data.get("workspace")
        if not raw:
            continue
        if normalize_path(file_uri_to_path(raw)) == target:
            matches.append(entry)

    if not matches:
        raise SystemExit(f"No VS Code workspaceStorage entry found for {workspace}")
    return max(matches, key=lambda p: p.stat().st_mtime)


def ensure_list_index(values: list[Any], index: int) -> None:
    while len(values) <= index:
        values.append({})


def apply_key_set(root: dict[str, Any], key: list[Any], value: Any) -> None:
    cur: Any = root
    for i, part in enumerate(key[:-1]):
        nxt = key[i + 1]
        if isinstance(part, int):
            if not isinstance(cur, list):
                return
            ensure_list_index(cur, part)
            if not isinstance(cur[part], (dict, list)):
                cur[part] = [] if isinstance(nxt, int) else {}
            cur = cur[part]
        else:
            if not isinstance(cur, dict):
                return
            if part not in cur or not isinstance(cur[part], (dict, list)):
                cur[part] = [] if isinstance(nxt, int) else {}
            cur = cur[part]

    last = key[-1]
    if isinstance(last, int):
        if isinstance(cur, list):
            ensure_list_index(cur, last)
            cur[last] = value
    elif isinstance(cur, dict):
        cur[last] = value


def apply_key_append(root: dict[str, Any], key: list[Any], value: Any) -> None:
    if key == ["requests"] and isinstance(value, list):
        root.setdefault("requests", [])
        if isinstance(root["requests"], list):
            root["requests"].extend(value)
        return

    if len(key) >= 3 and key[0] == "requests" and isinstance(key[1], int):
        requests = root.setdefault("requests", [])
        if not isinstance(requests, list):
            return
        ensure_list_index(requests, key[1])
        req = requests[key[1]]
        if not isinstance(req, dict):
            req = {}
            requests[key[1]] = req
        field = key[2]
        if len(key) == 3:
            if isinstance(value, list):
                req.setdefault(field, [])
                if isinstance(req[field], list):
                    req[field].extend(value)
                    return
            req[field] = value
            return

    apply_key_set(root, key, value)


def parse_jsonl_session(path: Path) -> dict[str, Any]:
    state: dict[str, Any] = {}
    with path.open(encoding="utf-8") as f:
        for line in f:
            if not line.strip():
                continue
            try:
                obj = json.loads(line)
            except json.JSONDecodeError:
                continue
            kind = obj.get("kind")
            key = obj.get("k")
            value = obj.get("v")
            if kind == 0 and isinstance(value, dict):
                state = value
            elif isinstance(key, list):
                if kind == 1:
                    apply_key_set(state, key, value)
                elif kind == 2:
                    apply_key_append(state, key, value)
    return state


def parse_session(path: Path) -> tuple[dict[str, Any], list[dict[str, Any]], dict[str, Any]]:
    state = read_json(path) if path.suffix == ".json" else parse_jsonl_session(path)
    requests = state.get("requests") if isinstance(state.get("requests"), list) else []
    clean_requests = [r for r in requests if isinstance(r, dict)]
    metadata = {k: v for k, v in state.items() if k != "requests"}
    return metadata, clean_requests, state


def request_user_text(req: dict[str, Any]) -> str:
    msg = req.get("message")
    if isinstance(msg, dict):
        if isinstance(msg.get("text"), str):
            return msg["text"]
        parts = msg.get("parts")
        if isinstance(parts, list):
            return "\n".join(p.get("text", "") for p in parts if isinstance(p, dict))
    return ""


def extract_text_parts(parts: Any, *, include_thinking: bool) -> list[str]:
    out: list[str] = []
    if not isinstance(parts, list):
        return out

    for part in parts:
        if isinstance(part, str):
            if part.strip():
                out.append(part)
            continue
        if not isinstance(part, dict):
            continue

        kind = part.get("kind")
        if kind == "thinking" and not include_thinking:
            title = part.get("generatedTitle")
            if title:
                out.append(f"[thinking: {title}]")
            elif part.get("metadata", {}).get("vscodeReasoningDone"):
                out.append("[thinking: done]")
            continue

        if isinstance(part.get("value"), str) and part["value"].strip():
            out.append(part["value"])

        if kind == "inlineReference":
            ref = part.get("inlineReference")
            path = uri_fs_path(ref)
            if path:
                out.append(f"`{path}`")

        if kind == "toolInvocationSerialized":
            tool_id = part.get("toolId", "tool")
            msg = part.get("pastTenseMessage") or part.get("invocationMessage")
            value = msg.get("value") if isinstance(msg, dict) else msg
            if value:
                out.append(f"[{tool_id}] {value}")
            details = part.get("resultDetails")
            if details:
                out.append(f"[{tool_id} result] {short(details, 900)}")

        if kind in {"progressMessage", "warning", "error"}:
            value = part.get("message") or part.get("text")
            if value:
                out.append(f"[{kind}] {value}")

    return out


def inline_reference_label(part: dict[str, Any]) -> str:
    ref = part.get("inlineReference")
    if isinstance(ref, dict):
        if isinstance(ref.get("name"), str) and ref["name"]:
            return ref["name"]
        path = uri_fs_path(ref)
        if path:
            return Path(path).name
    return ""


def ui_visible_response_text(req: dict[str, Any]) -> str:
    """Approximate Copilot chat panel text, excluding expanded tool metadata."""
    chunks: list[str] = []
    for part in req.get("response") or []:
        if isinstance(part, str):
            if part.strip():
                chunks.append(part)
            continue
        if not isinstance(part, dict):
            continue
        kind = part.get("kind")
        if kind in {
            "toolInvocationSerialized",
            "prepareToolInvocation",
            "mcpServersStarting",
            "thinking",
        }:
            continue
        if kind == "inlineReference":
            label = inline_reference_label(part)
            if label:
                chunks.append(label)
            continue
        value = part.get("value")
        if isinstance(value, str) and value.strip():
            chunks.append(value)
    return "".join(chunks).replace("\r\n", "\n").replace("\r", "\n")


def extract_references(req: dict[str, Any]) -> list[str]:
    refs: list[str] = []
    for ref in req.get("contentReferences") or []:
        if isinstance(ref, dict):
            path = uri_fs_path(ref.get("reference"))
            if path:
                refs.append(path)
    variables = ((req.get("variableData") or {}).get("variables") or [])
    if isinstance(variables, list):
        for var in variables:
            if isinstance(var, dict):
                path = uri_fs_path(var.get("value"))
                if path:
                    refs.append(path)
    return sorted(set(refs))


def metadata_for_request(req: dict[str, Any]) -> dict[str, Any]:
    result = req.get("result") if isinstance(req.get("result"), dict) else {}
    metadata = result.get("metadata") if isinstance(result.get("metadata"), dict) else {}
    return metadata if isinstance(metadata, dict) else {}


def rendered_parts_to_text(parts: Any) -> str:
    if not isinstance(parts, list):
        return ""
    chunks: list[str] = []
    for part in parts:
        if isinstance(part, str):
            chunks.append(part)
        elif isinstance(part, dict):
            for key in ("text", "value", "response"):
                value = part.get(key)
                if isinstance(value, str) and value:
                    chunks.append(value)
                    break
    return "\n\n".join(chunks)


def parse_json_maybe(value: Any) -> Any:
    if not isinstance(value, str):
        return value
    try:
        return json.loads(value)
    except Exception:
        return value


def extract_attachment_tags(rendered_text: str) -> list[dict[str, str]]:
    attachments: list[dict[str, str]] = []
    pattern = re.compile(r"<attachment\b(?P<attrs>[^>]*)>", re.IGNORECASE)
    attr_pattern = re.compile(r"(\w+)=\"([^\"]*)\"")
    for match in pattern.finditer(rendered_text):
        attrs = {k: v for k, v in attr_pattern.findall(match.group("attrs"))}
        if attrs:
            attachments.append(attrs)
    return attachments


def normalize_variable(var: Any) -> dict[str, Any]:
    if not isinstance(var, dict):
        return {"raw": var}
    value = var.get("value")
    normalized = {
        "kind": var.get("kind"),
        "id": var.get("id"),
        "name": var.get("name"),
        "modelDescription": var.get("modelDescription"),
        "isRoot": var.get("isRoot"),
        "automaticallyAdded": var.get("automaticallyAdded"),
        "originLabel": var.get("originLabel"),
        "path": uri_fs_path(value),
    }
    if isinstance(value, str):
        normalized["text"] = value
    elif isinstance(value, dict):
        for key in ("fsPath", "path", "external"):
            if value.get(key):
                normalized[key] = value.get(key)
    return {k: v for k, v in normalized.items() if v not in (None, "", [])}


def normalize_tool_rounds(rounds: Any) -> list[dict[str, Any]]:
    if not isinstance(rounds, list):
        return []
    normalized: list[dict[str, Any]] = []
    for round_item in rounds:
        if not isinstance(round_item, dict):
            continue
        calls = []
        for call in round_item.get("toolCalls") or []:
            if not isinstance(call, dict):
                continue
            calls.append({
                "id": call.get("id"),
                "name": call.get("name"),
                "arguments": parse_json_maybe(call.get("arguments")),
            })
        normalized.append({
            "id": round_item.get("id"),
            "response": round_item.get("response"),
            "toolInputRetry": round_item.get("toolInputRetry"),
            "toolCalls": calls,
            "statefulMarker": round_item.get("statefulMarker"),
        })
    return normalized


def build_canonical_turn(session_id: str, turn_index: int, req: dict[str, Any]) -> dict[str, Any]:
    metadata = metadata_for_request(req)
    rendered_text = rendered_parts_to_text(metadata.get("renderedUserMessage"))
    variable_data = req.get("variableData") if isinstance(req.get("variableData"), dict) else {}
    variables = variable_data.get("variables") if isinstance(variable_data.get("variables"), list) else []
    normalized_variables = [normalize_variable(v) for v in variables]
    attachments = extract_attachment_tags(rendered_text)
    for var in normalized_variables:
        if var.get("kind") in {"file", "directory"} and var.get("path"):
            attachments.append({
                "kind": str(var.get("kind")),
                "id": str(var.get("id") or ""),
                "name": str(var.get("name") or ""),
                "filePath": str(var.get("path") or ""),
            })

    assistant_parts = extract_text_parts(req.get("response"), include_thinking=False)
    code_blocks = metadata.get("codeBlocks") if isinstance(metadata.get("codeBlocks"), list) else []

    return {
        "schemaVersion": 1,
        "sessionId": session_id,
        "turnIndex": turn_index,
        "requestId": req.get("requestId"),
        "timestamp": req.get("timestamp"),
        "timestampIso": ms_to_iso(req.get("timestamp")),
        "modelId": req.get("modelId"),
        "responseId": req.get("responseId"),
        "user": {
            "promptText": request_user_text(req),
            "renderedText": rendered_text,
            "attachments": attachments,
            "variables": normalized_variables,
        },
        "assistant": {
            "responseText": "\n\n".join(assistant_parts),
            "responsePartsCount": len(req.get("response") or []) if isinstance(req.get("response"), list) else 0,
            "toolRounds": normalize_tool_rounds(metadata.get("toolCallRounds")),
            "toolCallResults": metadata.get("toolCallResults"),
            "codeBlocks": code_blocks,
        },
        "engineeringContext": {
            "references": extract_references(req),
            "contentReferenceCount": len(req.get("contentReferences") or [])
            if isinstance(req.get("contentReferences"), list)
            else 0,
            "variableCount": len(normalized_variables),
            "attachmentCount": len(attachments),
            "toolRoundCount": len(metadata.get("toolCallRounds") or [])
            if isinstance(metadata.get("toolCallRounds"), list)
            else 0,
            "codeBlockCount": len(code_blocks),
        },
        "raw": {
            "requestObjectPath": f"data/chat-sessions/{session_id}.requests.jsonl",
            "requestObjectLine": turn_index,
            "topLevelKeys": sorted(req.keys()),
            "metadataKeys": sorted(metadata.keys()),
        },
    }


def select_session_files(session_dir: Path) -> tuple[list[Path], dict[str, list[Path]]]:
    grouped: dict[str, list[Path]] = {}
    if session_dir.exists():
        for path in session_dir.glob("*.*"):
            if path.suffix.lower() in {".json", ".jsonl"}:
                grouped.setdefault(path.stem, []).append(path)
    selected = [max(paths, key=lambda p: p.stat().st_mtime) for paths in grouped.values()]
    selected.sort(key=lambda p: p.stat().st_mtime, reverse=True)
    return selected, grouped


def write_session(
    wiki_dir: Path,
    data_dir: Path,
    source: Path,
    alternatives: list[Path],
    include_thinking: bool,
    index_entry: dict[str, Any] | None = None,
) -> dict[str, Any]:
    metadata, requests, state = parse_session(source)
    if index_entry:
        metadata["chatSessionStoreIndexEntry"] = index_entry
        if not metadata.get("lastMessageDate") and index_entry.get("lastMessageDate"):
            metadata["lastMessageDate"] = index_entry["lastMessageDate"]
        if not metadata.get("customTitle") and index_entry.get("title"):
            metadata["customTitle"] = index_entry["title"]
    session_id = metadata.get("sessionId") or source.stem
    title = metadata.get("customTitle") or metadata.get("title") or session_id

    session_data_dir = data_dir / "chat-sessions"
    session_data_dir.mkdir(parents=True, exist_ok=True)
    meta_path = session_data_dir / f"{session_id}.metadata.json"
    requests_path = session_data_dir / f"{session_id}.requests.jsonl"
    turns_dir = data_dir / "dialog-turns"
    turns_dir.mkdir(parents=True, exist_ok=True)
    turns_path = turns_dir / f"{session_id}.turns.jsonl"
    context_dir = data_dir / "dialog-context" / session_id
    context_dir.mkdir(parents=True, exist_ok=True)
    meta_payload = {
        "metadata": metadata,
        "source": file_record(source),
        "alternativeCacheFiles": [file_record(p) for p in alternatives],
        "requestCount": len(requests),
    }
    meta_path.write_text(json_dump(meta_payload), encoding="utf-8")
    with requests_path.open("w", encoding="utf-8") as f:
        for req in requests:
            f.write(json.dumps(req, ensure_ascii=False, separators=(",", ":")) + "\n")

    canonical_turns = [build_canonical_turn(session_id, i, req) for i, req in enumerate(requests, 1)]
    with turns_path.open("w", encoding="utf-8") as f:
        for turn in canonical_turns:
            rendered = turn["user"].get("renderedText") or ""
            if rendered:
                rendered_path = context_dir / f"turn-{turn['turnIndex']:03d}-rendered-user.md"
                rendered_path.write_text(rendered, encoding="utf-8")
                turn["user"]["renderedTextPath"] = str(rendered_path)
            f.write(json.dumps(turn, ensure_ascii=False, separators=(",", ":")) + "\n")

    pages_dir = wiki_dir / "copilot-sessions"
    pages_dir.mkdir(parents=True, exist_ok=True)
    page_path = pages_dir / f"{session_id}.md"
    canonical_dir = wiki_dir / "canonical-dialog"
    canonical_dir.mkdir(parents=True, exist_ok=True)
    canonical_page_path = canonical_dir / f"{session_id}.md"

    lines = [
        f"# {title}",
        "",
        "## Overview",
        "",
        f"| Field | Value |",
        f"|---|---|",
        f"| Session ID | `{session_id}` |",
        f"| Source cache | `{source}` |",
        f"| Request count | `{len(requests)}` |",
        f"| Canonical dialog | [`wiki/canonical-dialog/{session_id}.md`](../canonical-dialog/{session_id}.md) |",
        f"| Canonical turn data | `../../data/dialog-turns/{session_id}.turns.jsonl` |",
        f"| Created | `{metadata.get('creationDate', '')}` / `{ms_to_iso(metadata.get('creationDate')) or ''}` |",
        f"| Last message | `{metadata.get('lastMessageDate', '')}` / `{ms_to_iso(metadata.get('lastMessageDate')) or ''}` |",
        f"| Model(s) | `{', '.join(sorted({str(r.get('modelId')) for r in requests if r.get('modelId')}))}` |",
        "",
        "## Conversation Flow",
        "",
        "```mermaid",
        "sequenceDiagram",
        "    participant U as User",
        "    participant C as Copilot",
        f"    Note over U,C: {len(requests)} requests exported from local cache",
        "```",
        "",
        "## Full Conversation",
        "",
    ]

    for i, req in enumerate(requests, 1):
        rid = req.get("requestId", f"request-{i}")
        lines.extend([f"### Request {i}: `{rid}`", ""])
        facts = []
        if req.get("timestamp") is not None:
            facts.append(f"`{req.get('timestamp')}` / `{ms_to_iso(req.get('timestamp')) or ''}`")
        if req.get("modelId"):
            facts.append(f"model `{req.get('modelId')}`")
        if facts:
            lines.extend(["- " + " | ".join(facts), ""])

        user = request_user_text(req)
        if user.strip():
            lines.extend(["#### User", "", md(user), ""])

        response_parts = extract_text_parts(req.get("response"), include_thinking=include_thinking)
        if response_parts:
            lines.extend(["#### Assistant And Tool Output", ""])
            for part in response_parts:
                lines.extend([md(part), ""])

        refs = extract_references(req)
        if refs:
            lines.extend(["#### References", ""])
            for ref in refs[:100]:
                lines.append(f"- `{ref}`")
            if len(refs) > 100:
                lines.append(f"- ... {len(refs) - 100} more references in structured data")
            lines.append("")

        lines.extend([
            "#### Structured Record",
            "",
            f"- Full request object: `../../data/chat-sessions/{session_id}.requests.jsonl`, line {i}",
            "",
        ])

    lines.extend([
        "## Section Sources",
        "",
        f"- Cache source: `{source}`",
        f"- Metadata export: `../../data/chat-sessions/{session_id}.metadata.json`",
        f"- Request export: `../../data/chat-sessions/{session_id}.requests.jsonl`",
        f"- Canonical turn export: `../../data/dialog-turns/{session_id}.turns.jsonl`",
    ])
    page_path.write_text("\n".join(lines), encoding="utf-8")

    canonical_lines = [
        f"# Canonical Dialog: {title}",
        "",
        "This page is intended for memory ingestion and project wiki generation. It preserves the engineering context that VS Code Copilot injects around each request: rendered user message, attachments, prompt/instruction variables, tool rounds, code blocks, and references.",
        "",
        "## Data Contract",
        "",
        f"- Exact raw request objects: `../../data/chat-sessions/{session_id}.requests.jsonl`",
        f"- Canonical turn JSONL: `../../data/dialog-turns/{session_id}.turns.jsonl`",
        f"- Rendered user-context blocks: `../../data/dialog-context/{session_id}/turn-###-rendered-user.md`",
        "",
        "```mermaid",
        "flowchart LR",
        "    Raw[VS Code request object] --> Turn[Canonical turn]",
        "    Turn --> User[User prompt + rendered context]",
        "    Turn --> Tools[Tool rounds + calls]",
        "    Turn --> Assistant[Assistant response]",
        "    Turn --> Memory[Memory/wiki ingestion]",
        "```",
        "",
        "## Turns",
        "",
    ]

    for turn in canonical_turns:
        idx = turn["turnIndex"]
        prompt = turn["user"].get("promptText") or ""
        canonical_lines.extend([
            f"### Turn {idx}: `{turn.get('requestId')}`",
            "",
            f"- Timestamp: `{turn.get('timestamp')}` / `{turn.get('timestampIso') or ''}`",
            f"- Model: `{turn.get('modelId') or ''}`",
            f"- Attachments: `{turn['engineeringContext']['attachmentCount']}`",
            f"- Variables: `{turn['engineeringContext']['variableCount']}`",
            f"- Tool rounds: `{turn['engineeringContext']['toolRoundCount']}`",
            f"- Code blocks: `{turn['engineeringContext']['codeBlockCount']}`",
            "",
            "#### User Prompt",
            "",
            md(prompt) if prompt else "_No direct prompt text retained._",
            "",
        ])
        rendered_path = turn["user"].get("renderedTextPath")
        if rendered_path:
            rel_rendered = Path(rendered_path).relative_to(wiki_dir.parent).as_posix()
            preview = short(turn["user"].get("renderedText"), 1200)
            canonical_lines.extend([
                "#### Rendered User Context",
                "",
                f"- Full rendered context: `../{rel_rendered}`",
                "",
                "<details>",
                "<summary>Preview</summary>",
                "",
                "```text",
                preview,
                "```",
                "",
                "</details>",
                "",
            ])
        refs = turn["engineeringContext"].get("references") or []
        if refs:
            canonical_lines.extend(["#### References", ""])
            for ref in refs[:50]:
                canonical_lines.append(f"- `{ref}`")
            if len(refs) > 50:
                canonical_lines.append(f"- ... {len(refs) - 50} more references in canonical JSONL")
            canonical_lines.append("")

        canonical_lines.extend(["#### Tool Rounds", ""])
        tool_rounds = turn["assistant"].get("toolRounds") or []
        if tool_rounds:
            for round_i, round_item in enumerate(tool_rounds, 1):
                canonical_lines.append(f"- Round {round_i}: {short(round_item.get('response'), 300)}")
                for call in round_item.get("toolCalls") or []:
                    canonical_lines.append(
                        f"  - `{call.get('name')}` `{short(call.get('arguments'), 500)}`"
                    )
        else:
            canonical_lines.append("- No tool rounds retained.")
        canonical_lines.extend([
            "",
            "#### Assistant Response",
            "",
            short(turn["assistant"].get("responseText"), 2500) or "_No assistant response text retained._",
            "",
            "#### Raw Links",
            "",
            f"- Raw request line: `../../data/chat-sessions/{session_id}.requests.jsonl:{idx}`",
            f"- Canonical turn line: `../../data/dialog-turns/{session_id}.turns.jsonl:{idx}`",
            "",
        ])

    canonical_lines.extend([
        "## Section Sources",
        "",
        f"- Cache source: `{source}`",
        f"- Raw request export: `../../data/chat-sessions/{session_id}.requests.jsonl`",
        f"- Canonical turn export: `../../data/dialog-turns/{session_id}.turns.jsonl`",
    ])
    canonical_page_path.write_text("\n".join(canonical_lines), encoding="utf-8")

    ui_dir = wiki_dir / "ui-transcripts"
    ui_dir.mkdir(parents=True, exist_ok=True)
    ui_path = ui_dir / f"{session_id}.md"
    ui_lines = [
        f"# UI Transcript: {title}",
        "",
        "This transcript approximates the Copilot chat panel's visible user/copilot text. It excludes expanded tool-call metadata and hidden rendered context. Use canonical dialog files for memory/wiki ingestion.",
        "",
        f"- Session ID: `{session_id}`",
        f"- Source cache: `{source}`",
        f"- Turns: `{len(requests)}`",
        "",
    ]
    for i, req in enumerate(requests, 1):
        user_text = request_user_text(req)
        assistant_text = ui_visible_response_text(req)
        ui_lines.extend([
            f"## Turn {i}: `{req.get('requestId', '')}`",
            "",
            "### user",
            "",
            md(user_text) if user_text else "_No visible user text retained._",
            "",
            "### copilot",
            "",
            md(assistant_text) if assistant_text else "_No visible Copilot text retained._",
            "",
        ])
    ui_lines.extend([
        "## Section Sources",
        "",
        f"- Raw request export: `../../data/chat-sessions/{session_id}.requests.jsonl`",
        f"- Canonical turn export: `../../data/dialog-turns/{session_id}.turns.jsonl`",
    ])
    ui_path.write_text("\n".join(ui_lines), encoding="utf-8")

    return {
        "sessionId": session_id,
        "title": title,
        "source": str(source),
        "alternatives": [str(p) for p in alternatives],
        "markdown": str(page_path),
        "canonicalMarkdown": str(canonical_page_path),
        "uiTranscriptMarkdown": str(ui_path),
        "metadata": str(meta_path),
        "requestsJsonl": str(requests_path),
        "canonicalTurnsJsonl": str(turns_path),
        "created": metadata.get("creationDate"),
        "createdIso": ms_to_iso(metadata.get("creationDate")),
        "lastMessageDate": metadata.get("lastMessageDate"),
        "lastMessageIso": ms_to_iso(metadata.get("lastMessageDate")),
        "requests": len(requests),
        "size": source.stat().st_size,
    }


def read_db_values(db: Path, interesting_only: bool = True) -> dict[str, Any]:
    if not db.exists():
        return {}
    con = sqlite3.connect(db)
    try:
        rows = con.execute("select key, value from ItemTable order by key").fetchall()
    finally:
        con.close()
    out: dict[str, Any] = {}
    markers = ("chat", "copilot", "interactive", "agent", "terminalChat")
    for key, value in rows:
        if interesting_only and not any(m.lower() in key.lower() for m in markers):
            continue
        try:
            out[key] = json.loads(value)
        except Exception:
            out[key] = value
    return out


def export_input_history(wiki_dir: Path, data_dir: Path, workspace_state: dict[str, Any]) -> dict[str, Any]:
    memento = workspace_state.get("memento/interactive-session")
    history = {}
    if isinstance(memento, dict):
        history = memento.get("history") or {}
    entries = history.get("copilot") if isinstance(history, dict) else []
    if not isinstance(entries, list):
        entries = []

    out_json = data_dir / "local-input-history.json"
    out_json.write_text(json_dump({"source": "workspace state.vscdb:memento/interactive-session", "entries": entries}), encoding="utf-8")

    page = wiki_dir / "local-input-history.md"
    lines = [
        "# Local Input History",
        "",
        "This page exports the VS Code chat input history stored in the workspace state database. It is not the full assistant transcript; it records prompt text, attachments, model selection, and editor context retained by VS Code.",
        "",
        f"- Entries: `{len(entries)}`",
        f"- Structured data: `../data/local-input-history.json`",
        "",
        "## Entries",
        "",
    ]
    for i, entry in enumerate(entries, 1):
        if not isinstance(entry, dict):
            continue
        text = entry.get("inputText") or ""
        attachments = entry.get("attachments") or []
        model = ((entry.get("selectedModel") or {}).get("metadata") or {}).get("id")
        lines.extend([
            f"### Entry {i}",
            "",
            f"- Model: `{model or ''}`",
            f"- Attachments: `{len(attachments) if isinstance(attachments, list) else 0}`",
            "",
            "#### Prompt",
            "",
            md(text) if text else "_Empty input retained by VS Code._",
            "",
        ])
        if isinstance(attachments, list) and attachments:
            lines.extend(["#### Attachments", ""])
            for att in attachments[:50]:
                if isinstance(att, dict):
                    lines.append(f"- `{uri_fs_path(att.get('value')) or att.get('name') or att.get('id')}`")
            lines.append("")
    lines.extend([
        "## Section Sources",
        "",
        "- Workspace DB key: `memento/interactive-session`",
        "- Structured export: `../data/local-input-history.json`",
    ])
    page.write_text("\n".join(lines), encoding="utf-8")
    return {"entries": len(entries), "json": str(out_json), "markdown": str(page)}


def scan_empty_window_sessions(markers: list[str]) -> list[dict[str, Any]]:
    results: list[dict[str, Any]] = []
    if not EMPTY_WINDOW_SESSIONS.exists():
        return results
    lower_markers = [m.lower() for m in markers if m]
    for path in sorted(EMPTY_WINDOW_SESSIONS.glob("*.*"), key=lambda p: p.stat().st_mtime, reverse=True):
        if path.suffix.lower() not in {".json", ".jsonl"}:
            continue
        try:
            text = path.read_text(encoding="utf-8", errors="ignore")
        except Exception as exc:
            results.append({"source": str(path), "error": repr(exc)})
            continue
        hits = [markers[i] for i, m in enumerate(lower_markers) if m in text.lower()]
        title = ""
        request_count: int | None = None
        try:
            if path.suffix == ".json":
                obj = json.loads(text)
            else:
                obj = parse_jsonl_session(path)
            title = obj.get("customTitle") or obj.get("title") or ""
            request_count = len(obj.get("requests") or []) if isinstance(obj.get("requests"), list) else 0
        except Exception:
            pass
        results.append({
            "source": str(path),
            "size": path.stat().st_size,
            "mtimeIso": dt.datetime.fromtimestamp(path.stat().st_mtime, tz=dt.timezone.utc).isoformat(),
            "title": title,
            "requests": request_count,
            "matchedWorkspaceMarkers": hits,
        })
    return results


def export_edits(storage: Path, wiki_dir: Path, data_dir: Path) -> list[dict[str, Any]]:
    edit_root = storage / "chatEditingSessions"
    if not edit_root.exists():
        return []

    wiki_edit_dir = wiki_dir / "editing-sessions"
    data_edit_dir = data_dir / "editing-sessions"
    wiki_edit_dir.mkdir(parents=True, exist_ok=True)
    data_edit_dir.mkdir(parents=True, exist_ok=True)
    summaries: list[dict[str, Any]] = []

    for state_path in sorted(edit_root.glob("*/state.json"), key=lambda p: p.stat().st_mtime, reverse=True):
        session_id = state_path.parent.name
        try:
            data = read_json(state_path)
        except Exception as exc:
            summaries.append({"sessionId": session_id, "source": str(state_path), "error": repr(exc)})
            continue

        timeline = data.get("timeline") or {}
        ops = timeline.get("operations") if isinstance(timeline, dict) else []
        checkpoints = timeline.get("checkpoints") if isinstance(timeline, dict) else []
        file_counts: Counter[str] = Counter()
        type_counts: Counter[str] = Counter()
        request_counts: Counter[str] = Counter()
        concise_ops: list[dict[str, Any]] = []

        if isinstance(ops, list):
            for op in ops:
                if not isinstance(op, dict):
                    continue
                typ = op.get("type", "unknown")
                path = uri_fs_path(op.get("uri"))
                request_id = op.get("requestId", "")
                if path:
                    file_counts[path] += 1
                type_counts[typ] += 1
                if request_id:
                    request_counts[request_id] += 1
                item = {
                    "sessionId": session_id,
                    "type": typ,
                    "requestId": request_id,
                    "epoch": op.get("epoch"),
                    "path": path,
                }
                if typ == "textEdit":
                    edits = op.get("edits")
                    item["editCount"] = len(edits) if isinstance(edits, list) else 0
                    if isinstance(edits, list) and edits:
                        first = edits[0]
                        if isinstance(first, dict):
                            item["sampleText"] = short(first.get("text"), 300)
                            item["sampleRange"] = first.get("range")
                if typ == "create":
                    item["initialContentLength"] = len(op.get("initialContent") or "")
                concise_ops.append(item)

        operations_path = data_edit_dir / f"{session_id}.operations.jsonl"
        with operations_path.open("w", encoding="utf-8") as f:
            for op in concise_ops:
                f.write(json.dumps(op, ensure_ascii=False, separators=(",", ":")) + "\n")

        page_path = wiki_edit_dir / f"{session_id}.md"
        lines = [
            f"# Editing Session `{session_id}`",
            "",
            "## Overview",
            "",
            f"| Field | Value |",
            f"|---|---|",
            f"| Source | `{state_path}` |",
            f"| Operations | `{len(ops) if isinstance(ops, list) else 0}` |",
            f"| Checkpoints | `{len(checkpoints) if isinstance(checkpoints, list) else 0}` |",
            f"| Changed files | `{len(file_counts)}` |",
            f"| Operation types | `{dict(type_counts)}` |",
            "",
            "## Edit Flow",
            "",
            "```mermaid",
            "flowchart LR",
            "    Request[Copilot request] --> Operation[Text/create operation]",
            "    Operation --> Workspace[Workspace files]",
            "    Operation --> Checkpoint[Checkpoint timeline]",
            "```",
            "",
            "## Top Changed Files",
            "",
        ]
        for path, count in file_counts.most_common(50):
            lines.append(f"- `{path}`: {count}")
        lines.extend(["", "## Top Requests", ""])
        for request_id, count in request_counts.most_common(50):
            lines.append(f"- `{request_id}`: {count}")
        lines.extend([
            "",
            "## Section Sources",
            "",
            f"- Cache source: `{state_path}`",
            f"- Normalized operations: `../../data/editing-sessions/{session_id}.operations.jsonl`",
        ])
        page_path.write_text("\n".join(lines), encoding="utf-8")

        summaries.append({
            "sessionId": session_id,
            "source": str(state_path),
            "operations": len(ops) if isinstance(ops, list) else 0,
            "checkpoints": len(checkpoints) if isinstance(checkpoints, list) else 0,
            "operationTypes": dict(type_counts),
            "changedFiles": len(file_counts),
            "topFiles": file_counts.most_common(25),
            "topRequests": request_counts.most_common(25),
            "markdown": str(page_path),
            "operationsJsonl": str(operations_path),
            "lastWriteTime": state_path.stat().st_mtime,
        })

    return summaries


def copy_raw_sources(raw_dir: Path, paths: list[Path]) -> list[dict[str, Any]]:
    raw_dir.mkdir(parents=True, exist_ok=True)
    copied: list[dict[str, Any]] = []
    seen: set[str] = set()
    for path in paths:
        if not path.exists() or not path.is_file():
            continue
        key = str(path.resolve()).lower()
        if key in seen:
            continue
        seen.add(key)
        digest = sha256_file(path)
        dest = raw_dir / f"{safe_filename(path.parent.name)}__{safe_filename(path.name)}"
        shutil.copy2(path, dest)
        copied.append({
            "source": str(path),
            "copy": str(dest),
            "size": path.stat().st_size,
            "sha256": digest,
        })
    return copied


def write_wiki_pages(
    base: Path,
    workspace: Path,
    storage: Path,
    sessions: list[dict[str, Any]],
    edits: list[dict[str, Any]],
    workspace_state: dict[str, Any],
    global_state: dict[str, Any],
    empty_windows: list[dict[str, Any]],
    input_history: dict[str, Any],
) -> None:
    wiki_dir = base / "wiki"

    matched_empty = [e for e in empty_windows if e.get("matchedWorkspaceMarkers")]
    index_lines = [
        "# LLM&Dialog Copilot Session Reference Wiki",
        "",
        "This wiki is generated from local VS Code and GitHub Copilot cache files for the LLM&Dialog workspace. It separates readable transcript pages from structured data exports so the archive can be used both by humans and by future agents.",
        "",
        "## Export Summary",
        "",
        "| Area | Count | Notes |",
        "|---|---:|---|",
        f"| Workspace chat sessions | {len(sessions)} | Full readable transcript pages plus request JSONL data |",
        f"| Workspace editing sessions | {len(edits)} | File operation timelines and normalized operation JSONL |",
        f"| Local input-history entries | {input_history.get('entries', 0)} | VS Code retained prompt/attachment history |",
        f"| Matching empty-window sessions | {len(matched_empty)} | Scanned global empty-window cache |",
        "",
        "## Navigation",
        "",
        "- [Source Inventory](sources.md)",
        "- [Session Index](session-index.md)",
        "- [Editing Index](editing-index.md)",
        "- [Local Input History](local-input-history.md)",
        "- [Dialog Extraction Policy](dialog-extraction-policy.md)",
        "- [Document Map](doc-map.md)",
        "",
        "## Cache Coverage",
        "",
        f"- Workspace cache: `{storage}`",
        f"- Global empty-window cache: `{EMPTY_WINDOW_SESSIONS}`",
        f"- Global state database: `{GLOBAL_STORAGE / 'state.vscdb'}`",
        "",
        "## Section Sources",
        "",
        f"- Workspace folder: `{workspace}`",
        f"- Workspace storage: `{storage}`",
        "- Structured export index: `../data/index.json`",
    ]
    (wiki_dir / "index.md").write_text("\n".join(index_lines), encoding="utf-8")

    session_lines = [
        "# Session Index",
        "",
        "| Session | UI Transcript | Canonical Dialog | Title | Requests | Last Message | Source |",
        "|---|---|---|---|---:|---|---|",
    ]
    for s in sorted(sessions, key=lambda x: x.get("lastMessageDate") or 0, reverse=True):
        rel = Path(s["markdown"]).relative_to(wiki_dir).as_posix()
        canonical_rel = Path(s["canonicalMarkdown"]).relative_to(wiki_dir).as_posix()
        ui_rel = Path(s["uiTranscriptMarkdown"]).relative_to(wiki_dir).as_posix()
        title = str(s.get("title", "")).replace("|", "\\|")
        session_lines.append(
            f"| [{s['sessionId']}]({rel}) | [ui]({ui_rel}) | [canonical]({canonical_rel}) | {title} | {s.get('requests', 0)} | `{s.get('lastMessageIso') or ''}` | `{s.get('source')}` |"
        )
    session_lines.extend([
        "",
        "## Section Sources",
        "",
        "- `../data/index.json`",
        "- `copilot-sessions/*.md`",
    ])
    (wiki_dir / "session-index.md").write_text("\n".join(session_lines), encoding="utf-8")

    edit_lines = [
        "# Editing Index",
        "",
        "| Editing Session | Operations | Checkpoints | Changed Files | Top Operation Types |",
        "|---|---:|---:|---:|---|",
    ]
    for e in sorted(edits, key=lambda x: x.get("operations") or 0, reverse=True):
        rel = Path(e["markdown"]).relative_to(wiki_dir).as_posix() if e.get("markdown") else ""
        edit_lines.append(
            f"| [{e.get('sessionId')}]({rel}) | {e.get('operations', 0)} | {e.get('checkpoints', 0)} | {e.get('changedFiles', 0)} | `{e.get('operationTypes', {})}` |"
        )
    edit_lines.extend([
        "",
        "## Section Sources",
        "",
        "- `../data/index.json`",
        "- `editing-sessions/*.md`",
    ])
    (wiki_dir / "editing-index.md").write_text("\n".join(edit_lines), encoding="utf-8")

    sources_lines = [
        "# Source Inventory",
        "",
        "## Workspace Cache",
        "",
        f"- Workspace folder: `{workspace}`",
        f"- Workspace storage: `{storage}`",
        f"- Chat sessions directory: `{storage / 'chatSessions'}`",
        f"- Editing sessions directory: `{storage / 'chatEditingSessions'}`",
        f"- Workspace state DB: `{storage / 'state.vscdb'}`",
        "",
        "## Local / Global Cache",
        "",
        f"- Empty-window sessions directory: `{EMPTY_WINDOW_SESSIONS}`",
        f"- Global state DB: `{GLOBAL_STORAGE / 'state.vscdb'}`",
        f"- Empty-window sessions scanned: `{len(empty_windows)}`",
        f"- Empty-window sessions matched to LLM&Dialog markers: `{len(matched_empty)}`",
        "",
        "## Relevant State Keys",
        "",
        "### Workspace State",
        "",
    ]
    for key in workspace_state:
        sources_lines.append(f"- `{key}`")
    sources_lines.extend(["", "### Global State", ""])
    for key in global_state:
        sources_lines.append(f"- `{key}`")
    sources_lines.extend([
        "",
        "## Section Sources",
        "",
        "- `../data/state/workspace-state-keys.json`",
        "- `../data/state/global-state-keys.json`",
        "- `../data/local-empty-window-index.json`",
    ])
    (wiki_dir / "sources.md").write_text("\n".join(sources_lines), encoding="utf-8")

    policy_lines = [
        "# Dialog Extraction Policy",
        "",
        "## Purpose",
        "",
        "VS Code Copilot chat cache is not a simple transcript. A single user-visible turn can contain the direct prompt, rendered attachments, automatically injected instruction files, skills, tool call rounds, tool results, response fragments, references, and editing timelines. A reliable memory/wiki pipeline must preserve these layers separately instead of flattening everything into one Markdown paragraph.",
        "",
        "## Canonical Turn Contract",
        "",
        "Each line in `../data/dialog-turns/<session-id>.turns.jsonl` is one canonical dialog turn with these fields:",
        "",
        "| Field | Meaning | Memory/Wiki Use |",
        "|---|---|---|",
        "| `user.promptText` | The direct text typed by the user | Primary user intent |",
        "| `user.renderedText` | VS Code-rendered message including attachments and injected context | Full context reconstruction |",
        "| `user.attachments` | Parsed attachment tags and file/directory variables | Artifact trail |",
        "| `user.variables` | Prompt files, instruction files, selected files, and automatic context variables | Context provenance |",
        "| `assistant.toolRounds` | Agent progress messages and tool calls with parsed arguments | Engineering workflow reconstruction |",
        "| `assistant.responseText` | Human-readable assistant response fragments | Dialogue narrative |",
        "| `assistant.codeBlocks` | Code blocks captured by Copilot metadata | Implementation evidence |",
        "| `engineeringContext.references` | Files and URIs referenced by the request | Source traceability |",
        "| `raw.requestObjectLine` | Pointer back to the exact raw request JSONL line | Audit and reparse path |",
        "",
        "## Recommended Ingestion Order",
        "",
        "1. Read `wiki/session-index.md` to select meaningful non-empty sessions.",
        "2. Read `data/dialog-turns/<session-id>.turns.jsonl` for memory ingestion.",
        "3. Use `data/dialog-context/<session-id>/turn-###-rendered-user.md` when the memory record needs the full rendered attachment/instruction context.",
        "4. Use `data/chat-sessions/<session-id>.requests.jsonl` only when exact raw VS Code fields are required.",
        "5. Join with `data/editing-sessions/*.operations.jsonl` by `requestId` to reconstruct file modifications.",
        "",
        "## Why Earlier Markdown Looked Incomplete",
        "",
        "The first exporter rendered only `message.text` and top-level `response` fragments. That missed `result.metadata.renderedUserMessage`, `result.metadata.toolCallRounds`, `variableData.variables`, and tool metadata. The canonical format fixes this by treating those fields as first-class dialog context.",
        "",
        "## Section Sources",
        "",
        "- `../data/dialog-turns/*.turns.jsonl`",
        "- `../data/chat-sessions/*.requests.jsonl`",
        "- `../data/editing-sessions/*.operations.jsonl`",
        "- `../../../../scripts/export_vscode_copilot.py`",
    ]
    (wiki_dir / "dialog-extraction-policy.md").write_text("\n".join(policy_lines), encoding="utf-8")

    doc_map = [
        "# Document Map",
        "",
        "```mermaid",
        "flowchart TB",
        "    Root[index.md] --> Sources[sources.md]",
        "    Root --> Sessions[session-index.md]",
        "    Root --> Edits[editing-index.md]",
        "    Root --> History[local-input-history.md]",
        "    Root --> Policy[dialog-extraction-policy.md]",
        "    Sessions --> SessionPages[copilot-sessions/*.md]",
        "    Sessions --> UITranscripts[ui-transcripts/*.md]",
        "    Sessions --> CanonicalPages[canonical-dialog/*.md]",
        "    Edits --> EditPages[editing-sessions/*.md]",
        "    SessionPages --> RequestData[data/chat-sessions/*.requests.jsonl]",
        "    CanonicalPages --> TurnData[data/dialog-turns/*.turns.jsonl]",
        "    CanonicalPages --> ContextBlocks[data/dialog-context/*/*.md]",
        "    EditPages --> EditData[data/editing-sessions/*.operations.jsonl]",
        "    Sources --> RawCache[cache/raw-cache]",
        "```",
        "",
        "## Reading Paths",
        "",
        "| Goal | Start Here | Then Read |",
        "|---|---|---|",
        "| Reconstruct all chat context | [Session Index](session-index.md) | `data/chat-sessions/*.requests.jsonl` |",
        "| Understand file modifications | [Editing Index](editing-index.md) | `data/editing-sessions/*.operations.jsonl` |",
        "| Audit cache provenance | [Source Inventory](sources.md) | `cache/source-manifest.json` |",
        "| Recover prompt history | [Local Input History](local-input-history.md) | `data/local-input-history.json` |",
        "",
        "## Section Sources",
        "",
        "- `../data/index.json`",
        "- `../cache/source-manifest.json`",
    ]
    (wiki_dir / "doc-map.md").write_text("\n".join(doc_map), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--workspace", required=True, help="Workspace folder path, e.g. E:\\repo")
    parser.add_argument("--out", required=True, help="Output .mini-wiki directory")
    parser.add_argument("--include-thinking", action="store_true", help="Include Copilot thinking text when present")
    parser.add_argument("--copy-raw", action="store_true", help="Copy exact source cache files into cache/raw-cache")
    args = parser.parse_args()

    workspace = Path(args.workspace).resolve()
    base = Path(args.out).resolve()
    wiki_dir = base / "wiki"
    data_dir = base / "data"
    cache_dir = base / "cache"
    for d in (wiki_dir, data_dir, cache_dir):
        d.mkdir(parents=True, exist_ok=True)

    storage = find_workspace_storage(workspace)
    selected_session_files, grouped_session_files = select_session_files(storage / "chatSessions")
    workspace_state = read_db_values(storage / "state.vscdb")
    chat_index = workspace_state.get("chat.ChatSessionStore.index")
    chat_index_entries = chat_index.get("entries", {}) if isinstance(chat_index, dict) else {}

    sessions: list[dict[str, Any]] = []
    raw_sources: list[Path] = []
    for source in selected_session_files:
        alternatives = sorted(grouped_session_files.get(source.stem, []), key=lambda p: p.stat().st_mtime, reverse=True)
        raw_sources.extend(alternatives)
        sessions.append(
            write_session(
                wiki_dir,
                data_dir,
                source,
                alternatives,
                args.include_thinking,
                chat_index_entries.get(source.stem),
            )
        )

    edits = export_edits(storage, wiki_dir, data_dir)
    raw_sources.extend([Path(e["source"]) for e in edits if e.get("source")])
    raw_sources.append(storage / "state.vscdb")
    if (storage / "state.vscdb.backup").exists():
        raw_sources.append(storage / "state.vscdb.backup")

    global_state = read_db_values(GLOBAL_STORAGE / "state.vscdb")
    state_dir = data_dir / "state"
    state_dir.mkdir(exist_ok=True)
    (state_dir / "workspace-state-keys.json").write_text(json_dump(workspace_state), encoding="utf-8")
    (state_dir / "global-state-keys.json").write_text(json_dump(global_state), encoding="utf-8")

    markers = [
        workspace.name,
        str(workspace),
        str(workspace).replace("\\", "/"),
        "LLM%26Dialog",
        "C%2B%2Bpandan",
        "ui-main",
        "Agent-Skills-for-Context",
        "superpowers-main",
        "mini-wiki",
    ]
    empty_windows = scan_empty_window_sessions(markers)
    (data_dir / "local-empty-window-index.json").write_text(json_dump(empty_windows), encoding="utf-8")
    for item in empty_windows:
        if item.get("matchedWorkspaceMarkers"):
            raw_sources.append(Path(item["source"]))

    input_history = export_input_history(wiki_dir, data_dir, workspace_state)

    raw_copies: list[dict[str, Any]] = []
    if args.copy_raw:
        raw_copies = copy_raw_sources(cache_dir / "raw-cache", raw_sources)

    index = {
        "workspace": str(workspace),
        "workspaceStorage": str(storage),
        "exportedAt": dt.datetime.now(dt.timezone.utc).isoformat(),
        "sessions": sessions,
        "editingSessions": edits,
        "inputHistory": input_history,
        "emptyWindowSessions": empty_windows,
        "rawCopies": raw_copies,
    }
    (data_dir / "index.json").write_text(json_dump(index), encoding="utf-8")

    source_manifest = {
        "workspace": str(workspace),
        "workspaceStorage": file_record(storage / "workspace.json"),
        "sessionCacheFiles": [file_record(p, include_hash=False) for paths in grouped_session_files.values() for p in paths],
        "editingStateFiles": [file_record(Path(e["source"]), include_hash=False) for e in edits if e.get("source")],
        "stateDb": file_record(storage / "state.vscdb", include_hash=False),
        "globalStateDb": file_record(GLOBAL_STORAGE / "state.vscdb", include_hash=False),
        "emptyWindowSessionFiles": empty_windows,
        "rawCopies": raw_copies,
    }
    (cache_dir / "source-manifest.json").write_text(json_dump(source_manifest), encoding="utf-8")

    config = [
        "generation:",
        "  language: zh",
        "  detail_level: detailed",
        "  include_diagrams: true",
        "  link_to_source: true",
        "export:",
        "  source: vscode-copilot-local-cache",
        "  workspace: LLM&Dialog",
    ]
    (base / "config.yaml").write_text("\n".join(config) + "\n", encoding="utf-8")
    (base / "meta.json").write_text(
        json_dump({
            "generator": "scripts/export_vscode_copilot.py",
            "quality_standard": "reference-wiki",
            "generated_at": dt.datetime.now(dt.timezone.utc).isoformat(),
            "workspace": str(workspace),
            "workspaceStorage": str(storage),
        }),
        encoding="utf-8",
    )

    write_wiki_pages(
        base,
        workspace,
        storage,
        sessions,
        edits,
        workspace_state,
        global_state,
        empty_windows,
        input_history,
    )

    print(json_dump({
        "output": str(base),
        "wiki": str(wiki_dir / "index.md"),
        "storage": str(storage),
        "sessions": len(sessions),
        "edits": len(edits),
        "inputHistoryEntries": input_history.get("entries", 0),
        "matchedEmptyWindowSessions": len([e for e in empty_windows if e.get("matchedWorkspaceMarkers")]),
        "rawCopies": len(raw_copies),
    }))


if __name__ == "__main__":
    main()
