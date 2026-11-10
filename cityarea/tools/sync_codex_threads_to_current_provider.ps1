$ErrorActionPreference = "Stop"

$script = @'
import glob
import json
import os
import re
import shutil
import sqlite3
import sys
import time
from pathlib import Path

codex_home = Path.home() / ".codex"
config_path = codex_home / "config.toml"
state_path = codex_home / ".codex-global-state.json"
sqlite_path = codex_home / "state_5.sqlite"
cc_switch_db_path = Path.home() / ".cc-switch" / "cc-switch.db"
chat_hub_provider = "chat_hub"
backup_tag = time.strftime("%Y%m%d-%H%M%S")

if not config_path.exists():
    raise SystemExit(f"Missing config file: {config_path}")
if not state_path.exists():
    raise SystemExit(f"Missing state file: {state_path}")
if not sqlite_path.exists():
    raise SystemExit(f"Missing sqlite file: {sqlite_path}")
if not cc_switch_db_path.exists():
    raise SystemExit(f"Missing cc-switch db file: {cc_switch_db_path}")

cc_con = sqlite3.connect(str(cc_switch_db_path))
cc_cur = cc_con.cursor()
cc_cur.execute(
    """
    select id, name, settings_config
      from providers
     where app_type = 'codex' and is_current = 1
     limit 1
    """
)
current_provider_row = cc_cur.fetchone()
cc_con.close()

if not current_provider_row:
    raise SystemExit("No current codex provider found in cc-switch.db")

current_provider_id, current_provider_name, settings_config_json = current_provider_row
settings_config = json.loads(settings_config_json or "{}")
source_config_text = settings_config.get("config") or ""
if not source_config_text.strip():
    raise SystemExit("Current codex provider config is empty in cc-switch.db")

match = re.search(r'(?m)^\s*model_provider\s*=\s*"([^"]+)"\s*$', source_config_text)
current_provider = match.group(1).strip() if match else None
if not current_provider:
    raise SystemExit("No model_provider found in current provider config")

provider_block_pattern = re.compile(
    rf'(?ms)^\[model_providers\.{re.escape(current_provider)}\]\s*\n.*?(?=^\[|\Z)'
)
provider_block_match = provider_block_pattern.search(source_config_text)
if not provider_block_match:
    raise SystemExit(f"No [model_providers.{current_provider}] block found in current provider config")

provider_block = provider_block_match.group(0)
hub_block_pattern = re.compile(
    rf'(?ms)^\[model_providers\.{re.escape(chat_hub_provider)}\]\s*\n.*?(?=^\[|\Z)'
)
hub_block = provider_block.replace(
    f"[model_providers.{current_provider}]",
    f"[model_providers.{chat_hub_provider}]",
    1,
)

disk_config_text = config_path.read_text(encoding="utf-8-sig", errors="replace")
next_config_text = re.sub(
    r'(?m)^\s*model_provider\s*=\s*"[^"]+"\s*$',
    f'model_provider = "{chat_hub_provider}"',
    source_config_text,
    count=1,
)
if next_config_text == source_config_text:
    next_config_text = f'model_provider = "{chat_hub_provider}"\n' + source_config_text
if hub_block_pattern.search(next_config_text):
    next_config_text = hub_block_pattern.sub(hub_block.rstrip() + "\n\n", next_config_text, count=1)
else:
    next_config_text = next_config_text.rstrip() + "\n\n" + hub_block.rstrip() + "\n"

config_changed = False
if next_config_text != disk_config_text:
    config_backup = config_path.with_name(config_path.name + f".provider-sync-{backup_tag}.bak")
    shutil.copy2(config_path, config_backup)
    tmp_config = config_path.with_suffix(config_path.suffix + ".tmp")
    tmp_config.write_text(next_config_text, encoding="utf-8", newline="\n")
    os.replace(tmp_config, config_path)
    config_changed = True

def normalize_path(value):
    if isinstance(value, str) and value.startswith("\\\\?\\"):
        return value[4:]
    return value

state_backup = state_path.with_name(state_path.name + f".provider-sync-{backup_tag}.bak")
shutil.copy2(state_path, state_backup)
state = json.loads(state_path.read_text(encoding="utf-8-sig"))
state_changed = False

for key in ("thread-workspace-root-hints",):
    mapping = state.get(key)
    if isinstance(mapping, dict):
        for item_key, item_value in list(mapping.items()):
            next_value = normalize_path(item_value)
            if next_value != item_value:
                mapping[item_key] = next_value
                state_changed = True

for key in ("electron-saved-workspace-roots", "project-order", "active-workspace-roots"):
    values = state.get(key)
    if isinstance(values, list):
        next_values = []
        seen = set()
        for value in values:
            next_value = normalize_path(value)
            if next_value != value:
                state_changed = True
            if next_value not in seen:
                seen.add(next_value)
                next_values.append(next_value)
        if next_values != values:
            state[key] = next_values
            state_changed = True

if state_changed:
    tmp_state = state_path.with_suffix(state_path.suffix + ".tmp")
    tmp_state.write_text(json.dumps(state, ensure_ascii=False, separators=(",", ":")), encoding="utf-8")
    os.replace(tmp_state, state_path)

session_updates = 0
session_backups = 0
session_skips = 0
for session_root in (codex_home / "sessions", codex_home / "archived_sessions"):
    if not session_root.exists():
        continue
    for path in glob.glob(str(session_root / "**" / "*.jsonl"), recursive=True):
        p = Path(path)
        try:
            with p.open("r", encoding="utf-8-sig", errors="replace") as f:
                lines = f.readlines()
            if not lines:
                continue
            meta = json.loads(lines[0])
        except Exception:
            continue

        payload = meta.get("payload") or {}
        changed = False

        if payload.get("model_provider") != chat_hub_provider:
            payload["model_provider"] = chat_hub_provider
            changed = True

        next_cwd = normalize_path(payload.get("cwd"))
        if next_cwd != payload.get("cwd"):
            payload["cwd"] = next_cwd
            changed = True

        if changed:
            tmp_session = p.with_name(p.name + ".tmp")
            try:
                backup = p.with_name(p.name + f".provider-sync-{backup_tag}.bak")
                if not backup.exists():
                    shutil.copy2(p, backup)
                    session_backups += 1
                meta["payload"] = payload
                lines[0] = json.dumps(meta, ensure_ascii=False, separators=(",", ":")) + "\n"
                with tmp_session.open("w", encoding="utf-8") as f:
                    f.writelines(lines)
                os.replace(tmp_session, p)
                session_updates += 1
            except PermissionError:
                session_skips += 1
                try:
                    if tmp_session.exists():
                        tmp_session.unlink()
                except Exception:
                    pass

sqlite_backup = sqlite_path.with_name(sqlite_path.name + f".provider-sync-{backup_tag}.bak")
shutil.copy2(sqlite_path, sqlite_backup)
for suffix in ("-wal", "-shm"):
    sidecar = Path(str(sqlite_path) + suffix)
    if sidecar.exists():
        shutil.copy2(sidecar, Path(str(sqlite_backup) + suffix))

con = sqlite3.connect(str(sqlite_path))
cur = con.cursor()
cur.execute("select id, cwd, rollout_path, has_user_event, first_user_message, model_provider from threads")
rows = cur.fetchall()
row_updates = 0

for thread_id, cwd, rollout_path, has_user_event, first_user_message, model_provider in rows:
    next_cwd = normalize_path(cwd)
    next_rollout = normalize_path(rollout_path)
    expected_user_event = 1 if first_user_message else has_user_event
    cur.execute(
        """
        update threads
           set model_provider = ?,
               cwd = ?,
               rollout_path = ?,
               has_user_event = case when has_user_event = 1 then 1 else ? end
         where id = ?
           and (
                model_provider <> ?
                or cwd <> ?
                or rollout_path <> ?
                or has_user_event <> case when has_user_event = 1 then 1 else ? end
           )
        """,
        (
            chat_hub_provider,
            next_cwd,
            next_rollout,
            expected_user_event,
            thread_id,
            chat_hub_provider,
            next_cwd,
            next_rollout,
            expected_user_event,
        ),
    )
    row_updates += cur.rowcount

con.commit()
try:
    cur.execute("pragma wal_checkpoint(TRUNCATE)")
except Exception:
    pass
cur.execute("select model_provider, count(*) from threads group by model_provider order by count(*) desc")
providers_after = cur.fetchall()
con.close()

print(f"current_provider_id={current_provider_id}")
print(f"current_provider_name={current_provider_name}")
print(f"current_provider={current_provider}")
print(f"chat_hub_provider={chat_hub_provider}")
print(f"config_changed={config_changed}")
print(f"state_changed={state_changed}")
print(f"session_updates={session_updates}")
print(f"session_backups={session_backups}")
print(f"session_skips={session_skips}")
print(f"sqlite_row_updates={row_updates}")
print(f"providers_after={providers_after}")
print(f"state_backup={state_backup}")
print(f"sqlite_backup={sqlite_backup}")
'@

$tempScript = Join-Path $env:TEMP "sync_codex_threads_to_current_provider.py"
[System.IO.File]::WriteAllText($tempScript, $script, [System.Text.UTF8Encoding]::new($false))
python $tempScript
