#!/usr/bin/env python3
"""
Bridge Codex Desktop rollout logs into claude-mem's transcript watcher format.

Codex Desktop stores thread metadata in ~/.codex/state_5.sqlite and rollout
JSONL files under ~/.codex/sessions. claude-mem already knows how to ingest
Codex-style JSONL, but the desktop app may not create new session files in the
same way the CLI watcher expects. This bridge produces a normalized, append-only
JSONL stream under ~/.claude-mem/codex-desktop-bridge/out.
"""

from __future__ import annotations

import argparse
import datetime as dt
import json
import os
import re
import sqlite3
import time
from pathlib import Path
from typing import Any


HOME = Path.home()
CODEX_HOME = Path(os.environ.get("CODEX_HOME", HOME / ".codex"))
CLAUDE_MEM_HOME = Path(os.environ.get("CLAUDE_MEM_HOME", HOME / ".claude-mem"))
DEFAULT_STATE_DB = CODEX_HOME / "state_5.sqlite"
DEFAULT_OUT = CLAUDE_MEM_HOME / "codex-desktop-bridge" / "out"
DEFAULT_BRIDGE_STATE = CLAUDE_MEM_HOME / "codex-desktop-bridge" / "bridge-state.json"
DEFAULT_CLAUDE_MEM_DB = CLAUDE_MEM_HOME / "claude-mem.db"


def now_iso() -> str:
    return dt.datetime.now(dt.timezone.utc).isoformat()


def parse_epoch_ms(value: str | None) -> int:
    if not value:
        return int(time.time() * 1000)
    text = value.replace("Z", "+00:00")
    try:
        return int(dt.datetime.fromisoformat(text).timestamp() * 1000)
    except ValueError:
        return int(time.time() * 1000)


def normalize_windows_path(path: str) -> str:
    if path.startswith("\\\\?\\"):
        path = path[4:]
    return path.replace("/", "\\")


def project_from_cwd(cwd: str) -> str:
    cwd = normalize_windows_path(cwd)
    if not cwd:
        return "unknown"
    # Codex Desktop worktrees preserve the original tail after "...\\workcode\\".
    marker = "\\workcode\\"
    lower = cwd.lower()
    idx = lower.rfind(marker)
    if idx >= 0:
        rest = cwd[idx + len(marker) :]
        first = rest.split("\\", 1)[0].strip()
        if first:
            return first
    return Path(cwd).name or "unknown"


def canonical_cwd(cwd: str) -> str:
    cwd = normalize_windows_path(cwd)
    project = project_from_cwd(cwd)
    preferred = Path(f"E:\\C++pandan\\workcode\\{project}")
    if preferred.exists():
        return str(preferred)
    return cwd


def safe_name(value: str) -> str:
    value = re.sub(r"[^\w.-]+", "-", value, flags=re.UNICODE).strip("-")
    return value or "unknown"


def load_bridge_state(path: Path) -> dict[str, Any]:
    if not path.exists():
        return {"version": 1, "sources": {}}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return {"version": 1, "sources": {}}


def save_bridge_state(path: Path, state: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    tmp.write_text(json.dumps(state, ensure_ascii=False, indent=2), encoding="utf-8")
    tmp.replace(path)


def read_threads(state_db: Path) -> list[dict[str, Any]]:
    con = sqlite3.connect(state_db)
    con.row_factory = sqlite3.Row
    try:
        rows = con.execute(
            """
            select id, cwd, title, rollout_path, source, created_at, updated_at,
                   first_user_message, model
            from threads
            order by updated_at asc
            """
        ).fetchall()
        return [dict(r) for r in rows]
    finally:
        con.close()


def convert_event(obj: dict[str, Any], thread: dict[str, Any], cwd: str) -> list[dict[str, Any]]:
    typ = obj.get("type")
    payload = obj.get("payload")
    if typ == "session_meta":
        out = {
            "timestamp": obj.get("timestamp") or now_iso(),
            "type": "session_meta",
            "payload": {
                "id": thread["id"],
                "cwd": cwd,
                "originator": "Codex Desktop",
                "source": thread.get("source") or "vscode",
                "title": thread.get("title") or "",
                "model": thread.get("model") or "",
            },
        }
        return [out]

    if typ == "turn_context" and isinstance(payload, dict):
        payload = dict(payload)
        payload["cwd"] = cwd
        return [{"timestamp": obj.get("timestamp") or now_iso(), "type": typ, "payload": payload}]

    if typ == "event_msg" and isinstance(payload, dict):
        ptype = payload.get("type")
        if ptype in {"user_message", "agent_message", "turn_aborted"}:
            return [{"timestamp": obj.get("timestamp") or now_iso(), "type": typ, "payload": payload}]
        if ptype == "task_complete":
            return [
                {
                    "timestamp": obj.get("timestamp") or now_iso(),
                    "type": "event_msg",
                    "payload": {"type": "turn_completed"},
                }
            ]
        return []

    if typ == "response_item" and isinstance(payload, dict):
        if payload.get("type") in {
            "function_call",
            "function_call_output",
            "custom_tool_call",
            "custom_tool_call_output",
            "web_search_call",
            "exec_command",
            "exec_command_output",
        }:
            return [{"timestamp": obj.get("timestamp") or now_iso(), "type": typ, "payload": payload}]

    return []


def bridge_thread(thread: dict[str, Any], out_root: Path, state: dict[str, Any]) -> dict[str, Any]:
    rollout_path = Path(thread.get("rollout_path") or "")
    if not rollout_path.exists():
        return {"thread": thread.get("id"), "status": "missing_rollout", "path": str(rollout_path)}

    cwd = canonical_cwd(thread.get("cwd") or "")
    project = project_from_cwd(cwd)
    out_dir = out_root / safe_name(project)
    out_dir.mkdir(parents=True, exist_ok=True)
    out_file = out_dir / f"{safe_name(thread['id'])}.jsonl"

    source_key = str(rollout_path.resolve()).lower()
    source_state = state.setdefault("sources", {}).setdefault(source_key, {})
    already_lines = int(source_state.get("lines", 0) or 0)
    if not out_file.exists():
        already_lines = 0

    written = 0
    total = 0
    with rollout_path.open(encoding="utf-8", errors="replace") as src, out_file.open(
        "a", encoding="utf-8"
    ) as dst:
        for line in src:
            total += 1
            if total <= already_lines:
                continue
            if not line.strip():
                continue
            try:
                obj = json.loads(line)
            except json.JSONDecodeError:
                continue
            for event in convert_event(obj, thread, cwd):
                dst.write(json.dumps(event, ensure_ascii=False, separators=(",", ":")) + "\n")
                written += 1

    source_state.update(
        {
            "thread_id": thread["id"],
            "project": project,
            "cwd": cwd,
            "rollout_path": str(rollout_path),
            "out_file": str(out_file),
            "lines": total,
            "last_run_at": now_iso(),
        }
    )
    return {
        "thread": thread["id"],
        "project": project,
        "cwd": cwd,
        "source_lines": total,
        "new_events": written,
        "out_file": str(out_file),
    }


def collect_user_messages(rollout_path: Path) -> list[dict[str, Any]]:
    messages: list[dict[str, Any]] = []
    if not rollout_path.exists():
        return messages
    with rollout_path.open(encoding="utf-8", errors="replace") as src:
        for line in src:
            if not line.strip():
                continue
            try:
                obj = json.loads(line)
            except json.JSONDecodeError:
                continue
            if obj.get("type") != "event_msg":
                continue
            payload = obj.get("payload")
            if not isinstance(payload, dict) or payload.get("type") != "user_message":
                continue
            message = payload.get("message")
            if isinstance(message, str) and message.strip():
                messages.append(
                    {
                        "timestamp": obj.get("timestamp") or now_iso(),
                        "message": message.strip(),
                    }
                )
    return messages


def sync_user_prompts_to_db(threads: list[dict[str, Any]], db_path: Path) -> list[dict[str, Any]]:
    if not db_path.exists():
        return [{"status": "missing_claude_mem_db", "path": str(db_path)}]

    con = sqlite3.connect(db_path)
    con.row_factory = sqlite3.Row
    results: list[dict[str, Any]] = []
    try:
        for thread in threads:
            session_id = thread["id"]
            rollout_path = Path(thread.get("rollout_path") or "")
            cwd = canonical_cwd(thread.get("cwd") or "")
            project = project_from_cwd(cwd)
            messages = collect_user_messages(rollout_path)
            if not messages:
                results.append({"thread": session_id, "project": project, "prompts": 0, "inserted": 0})
                continue

            first = messages[0]
            started_at = first["timestamp"]
            started_epoch = parse_epoch_ms(started_at)
            existing = con.execute(
                "select id from sdk_sessions where content_session_id = ?",
                (session_id,),
            ).fetchone()
            if existing is None:
                con.execute(
                    """
                    insert into sdk_sessions
                    (content_session_id, memory_session_id, project, user_prompt,
                     started_at, started_at_epoch, status, platform_source)
                    values (?, null, ?, ?, ?, ?, 'active', 'codex')
                    """,
                    (session_id, project, first["message"], started_at, started_epoch),
                )
            else:
                con.execute(
                    """
                    update sdk_sessions
                    set project = ?, platform_source = 'codex',
                        user_prompt = coalesce(nullif(user_prompt, ''), ?)
                    where content_session_id = ?
                    """,
                    (project, first["message"], session_id),
                )

            con.execute("delete from user_prompts where content_session_id = ?", (session_id,))
            for index, item in enumerate(messages, start=1):
                con.execute(
                    """
                    insert into user_prompts
                    (content_session_id, prompt_number, prompt_text, created_at, created_at_epoch)
                    values (?, ?, ?, ?, ?)
                    """,
                    (
                        session_id,
                        index,
                        item["message"],
                        item["timestamp"],
                        parse_epoch_ms(item["timestamp"]),
                    ),
                )
            results.append(
                {
                    "thread": session_id,
                    "project": project,
                    "prompts": len(messages),
                    "synced": len(messages),
                }
            )
        con.commit()
    finally:
        con.close()
    return results


def run_once(state_db: Path, out_root: Path, state_file: Path, claude_mem_db: Path) -> dict[str, Any]:
    out_root.mkdir(parents=True, exist_ok=True)
    state = load_bridge_state(state_file)
    threads = read_threads(state_db)
    results = []
    for thread in threads:
        results.append(bridge_thread(thread, out_root, state))
    prompt_sync = sync_user_prompts_to_db(threads, claude_mem_db)
    save_bridge_state(state_file, state)
    return {"bridge": results, "prompt_sync": prompt_sync}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--state-db", default=str(DEFAULT_STATE_DB))
    parser.add_argument("--out", default=str(DEFAULT_OUT))
    parser.add_argument("--state-file", default=str(DEFAULT_BRIDGE_STATE))
    parser.add_argument("--claude-mem-db", default=str(DEFAULT_CLAUDE_MEM_DB))
    parser.add_argument("--watch", action="store_true", help="Poll Codex Desktop state and append new events.")
    parser.add_argument("--interval", type=float, default=10.0)
    args = parser.parse_args()

    state_db = Path(args.state_db)
    out_root = Path(args.out)
    state_file = Path(args.state_file)
    claude_mem_db = Path(args.claude_mem_db)

    if args.watch:
        while True:
            results = run_once(state_db, out_root, state_file, claude_mem_db)
            print(json.dumps({"at": now_iso(), "results": results}, ensure_ascii=False))
            time.sleep(args.interval)
    else:
        results = run_once(state_db, out_root, state_file, claude_mem_db)
        print(json.dumps({"results": results}, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
