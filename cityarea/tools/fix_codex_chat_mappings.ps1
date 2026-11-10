$ErrorActionPreference = "Stop"

$codexHome = Join-Path $env:USERPROFILE ".codex"
$statePath = Join-Path $codexHome ".codex-global-state.json"

if (-not (Test-Path -LiteralPath $statePath)) {
    throw "Codex state file not found: $statePath"
}

$running = Get-Process -ErrorAction SilentlyContinue |
    Where-Object {
        ($_.ProcessName -eq "Codex") -or
        (($_.ProcessName -eq "codex") -and ($_.Path -like "*OpenAI.Codex*"))
    }

if ($running) {
    Write-Warning "Codex Desktop is still running. Close Codex Desktop from the tray first, then run this script again."
    $running | Select-Object ProcessName, Id, Path | Format-Table -AutoSize
    exit 2
}

$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    $python = Get-Command py -ErrorAction SilentlyContinue
}
if (-not $python) {
    throw "Python not found in PATH. Install Python or run this from a shell where python is available."
}

$script = @'
import glob
import json
import os
import re
import shutil
import sqlite3
import sys
import time

codex_home = os.path.join(os.path.expanduser("~"), ".codex")
state_path = os.path.join(codex_home, ".codex-global-state.json")
config_path = os.path.join(codex_home, "config.toml")

backup_path = state_path + ".fix-no-chats-" + time.strftime("%Y%m%d-%H%M%S") + ".bak"
shutil.copy2(state_path, backup_path)

with open(state_path, "r", encoding="utf-8-sig", errors="strict") as f:
    state = json.load(f)

hints = state.setdefault("thread-workspace-root-hints", {})
saved_roots = list(state.get("electron-saved-workspace-roots") or [])
project_order = list(state.get("project-order") or [])

def add_unique(items, value):
    if value and value not in items:
        items.append(value)
        return True
    return False

seen = 0
updated = 0
roots_added = 0
session_header_updates = 0
session_dirs = [
    os.path.join(codex_home, "sessions"),
    os.path.join(codex_home, "archived_sessions"),
]

state_current_provider = None
config_model_provider = None

if os.path.exists(config_path):
    try:
        with open(config_path, "r", encoding="utf-8-sig", errors="replace") as f:
            config_text = f.read()
        match = re.search(r'(?m)^\s*model_provider\s*=\s*"([^"]+)"\s*$', config_text)
        if match:
            config_model_provider = match.group(1).strip()
    except Exception:
        pass

for item in glob.glob(os.path.join(codex_home, "sessions", "**", "*.jsonl"), recursive=True):
    try:
        with open(item, "r", encoding="utf-8-sig", errors="replace") as f:
            header = f.readline()
        meta = json.loads(header)
        payload = meta.get("payload") or {}
        provider = payload.get("model_provider")
        timestamp = payload.get("timestamp") or meta.get("timestamp") or ""
        if provider:
            if state_current_provider is None or timestamp > state_current_provider[0]:
                state_current_provider = (timestamp, provider)
    except Exception:
        pass
current_provider_from_rollouts = state_current_provider[1] if state_current_provider else None
target_model_provider = config_model_provider or current_provider_from_rollouts

for session_dir in session_dirs:
    if not os.path.isdir(session_dir):
        continue

    for path in glob.glob(os.path.join(session_dir, "**", "*.jsonl"), recursive=True):
        try:
            with open(path, "r", encoding="utf-8-sig", errors="replace") as f:
                header = f.readline()
            meta = json.loads(header)
            payload = meta.get("payload") or {}
            thread_id = payload.get("id")
            cwd = payload.get("cwd")
        except Exception as exc:
            print(f"Skipped unreadable session header: {path} ({exc})")
            continue

        if not thread_id or not cwd:
            continue

        seen += 1
        if hints.get(thread_id) != cwd:
            hints[thread_id] = cwd
            updated += 1

        if os.path.exists(cwd):
            if add_unique(saved_roots, cwd):
                roots_added += 1
            add_unique(project_order, cwd)

        if target_model_provider and payload.get("model_provider") != target_model_provider:
            session_backup_path = path + ".fix-no-chats-" + time.strftime("%Y%m%d-%H%M%S") + ".bak"
            shutil.copy2(path, session_backup_path)
            payload["model_provider"] = target_model_provider
            meta["payload"] = payload
            with open(path, "r", encoding="utf-8-sig", errors="replace") as f:
                lines = f.readlines()
            lines[0] = json.dumps(meta, ensure_ascii=False, separators=(",", ":")) + "\n"
            tmp_session_path = path + ".tmp"
            with open(tmp_session_path, "w", encoding="utf-8") as f:
                f.writelines(lines)
            os.replace(tmp_session_path, path)
            session_header_updates += 1

state["electron-saved-workspace-roots"] = saved_roots
state["project-order"] = project_order

tmp_path = state_path + ".tmp"
with open(tmp_path, "w", encoding="utf-8") as f:
    json.dump(state, f, ensure_ascii=False, separators=(",", ":"))
os.replace(tmp_path, state_path)

print(f"Backup: {backup_path}")
print(f"Sessions scanned: {seen}")
print(f"Thread mappings added/updated: {updated}")
print(f"Workspace roots added: {roots_added}")
print(f"Config model_provider: {config_model_provider or '(none)'}")
print(f"Latest rollout provider: {current_provider_from_rollouts or '(none)'}")
print(f"Session header provider normalized to: {target_model_provider or '(none)'}")
print(f"Session headers updated: {session_header_updates}")

sqlite_path = os.path.join(codex_home, "state_5.sqlite")
if os.path.exists(sqlite_path):
    sqlite_backup_path = sqlite_path + ".fix-no-chats-" + time.strftime("%Y%m%d-%H%M%S") + ".bak"
    shutil.copy2(sqlite_path, sqlite_backup_path)
    wal_path = sqlite_path + "-wal"
    shm_path = sqlite_path + "-shm"
    if os.path.exists(wal_path):
        shutil.copy2(wal_path, sqlite_backup_path + "-wal")
    if os.path.exists(shm_path):
        shutil.copy2(shm_path, sqlite_backup_path + "-shm")

    def normalize_path(value):
        if not isinstance(value, str):
            return value
        if value.startswith("\\\\?\\"):
            return value[4:]
        return value

    con = sqlite3.connect(sqlite_path)
    cur = con.cursor()
    current_model_provider = target_model_provider
    if not current_model_provider:
        cur.execute(
            """
            select model_provider
              from threads
             where model_provider is not null and model_provider <> ''
             group by model_provider
             order by max(updated_at) desc
             limit 1
            """
        )
        row = cur.fetchone()
        current_model_provider = row[0] if row else None

    cur.execute("select id, cwd, rollout_path, first_user_message from threads")
    thread_rows = cur.fetchall()
    sqlite_updates = 0
    for thread_id, cwd, rollout_path, first_user_message in thread_rows:
        next_cwd = normalize_path(cwd)
        next_rollout_path = normalize_path(rollout_path)
        should_have_user_event = 1 if first_user_message else 0
        cur.execute(
            """
            update threads
               set cwd = ?,
                   rollout_path = ?,
                   has_user_event = case when has_user_event = 1 then 1 else ? end
             where id = ?
               and (cwd <> ? or rollout_path <> ? or has_user_event <> case when has_user_event = 1 then 1 else ? end)
            """,
            (
                next_cwd,
                next_rollout_path,
                should_have_user_event,
                thread_id,
                next_cwd,
                next_rollout_path,
                should_have_user_event,
            ),
        )
        sqlite_updates += cur.rowcount

    provider_updates = 0
    if current_model_provider:
        cur.execute(
            """
            update threads
               set model_provider = ?
             where model_provider is null
                or model_provider = ''
                or model_provider <> ?
            """,
            (current_model_provider, current_model_provider),
        )
        provider_updates = cur.rowcount

    con.commit()
    cur.execute("pragma wal_checkpoint(TRUNCATE)")
    con.close()

    print(f"SQLite backup: {sqlite_backup_path}")
    print(f"SQLite thread rows updated: {sqlite_updates}")
    print(f"SQLite provider source: {'config.toml' if config_model_provider else 'latest rollout' if current_provider_from_rollouts else 'sqlite fallback'}")
    print(f"SQLite provider normalized to: {current_model_provider or '(none)'}")
    print(f"SQLite provider rows updated: {provider_updates}")

print("Done. Start Codex Desktop again.")
'@

$tempScript = Join-Path $env:TEMP "fix_codex_chat_mappings.py"
[System.IO.File]::WriteAllText($tempScript, $script, [System.Text.UTF8Encoding]::new($false))
& $python.Source $tempScript
