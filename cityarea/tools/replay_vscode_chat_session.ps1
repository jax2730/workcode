param(
    [Parameter(Mandatory = $true)]
    [string]$SessionTitle
)

$ErrorActionPreference = "Stop"

$reportDir = Join-Path $PSScriptRoot "..\reports"
if (!(Test-Path $reportDir)) {
    New-Item -ItemType Directory -Path $reportDir | Out-Null
}

$py = @'
import json, os, glob, re, sys
from datetime import datetime

query = sys.argv[1].strip().lower()
ws_root = os.path.expanduser(r"~\AppData\Roaming\Code\User\workspaceStorage")

def read_jsonl(path):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line=line.strip()
            if not line:
                continue
            try:
                yield json.loads(line)
            except Exception:
                continue

def flatten_text(x):
    if x is None:
        return ""
    if isinstance(x, str):
        return x
    if isinstance(x, (int, float, bool)):
        return str(x)
    if isinstance(x, list):
        return "\n".join(flatten_text(i) for i in x if flatten_text(i))
    if isinstance(x, dict):
        # common chat payload keys
        parts = []
        for k in ("text", "content", "message", "response", "markdown"):
            if k in x:
                t = flatten_text(x.get(k))
                if t:
                    parts.append(t)
        if parts:
            return "\n".join(parts)
        # fallback: stringify short dict
        try:
            return json.dumps(x, ensure_ascii=False)
        except Exception:
            return str(x)
    return str(x)

session_files = glob.glob(os.path.join(ws_root, "**", "chatSessions", "*.jsonl"), recursive=True)
best = None
for fp in session_files:
    title = None
    for r in read_jsonl(fp):
        if r.get("k") == ["customTitle"]:
            title = r.get("v")
            break
    if not title:
        continue
    lt = str(title).lower()
    score = 0
    if query == lt:
        score = 1000
    elif query in lt:
        score = len(query)
    if score > 0 and (best is None or score > best[0]):
        best = (score, fp, title)

if best is None:
    print("NOT_FOUND")
    sys.exit(0)

_, path, title = best
msgs = []
file_refs = set()
cmd_refs = []

for r in read_jsonl(path):
    k = r.get("k")
    v = r.get("v")
    if k == ["requests"] and isinstance(v, list):
        for it in v:
            text = flatten_text(it.get("message") or it.get("text") or it)
            if text:
                msgs.append(("user", text))
    elif k == ["responses"] and isinstance(v, list):
        for it in v:
            text = flatten_text(it.get("response") or it.get("content") or it.get("text") or it)
            if text:
                msgs.append(("assistant", text))

    if isinstance(v, str):
        for m in re.findall(r'([A-Za-z]:\\\\[^\\n\\r\\t]+\\.(?:py|cpp|h|hpp|md|json|yml|yaml|txt))', v):
            file_refs.add(m)
        for c in re.findall(r'`([^`]*(?:python|git|npm|node|cmake|powershell)[^`]*)`', v, flags=re.I):
            cmd_refs.append(c)

out = []
out.append(f"# VSCode Chat Replay: {title}")
out.append("")
out.append(f"- Source: `{path}`")
out.append(f"- Generated: `{datetime.now().isoformat(timespec='seconds')}`")
out.append("")
out.append("## Message Timeline")
if msgs:
    for role, text in msgs[:120]:
        one = flatten_text(text).replace("\n", " ").strip()
        out.append(f"- **{role}**: {one[:500]}")
else:
    out.append("- No parsed messages (session structure may be different).")
out.append("")
out.append("## File References")
if file_refs:
    for f in sorted(file_refs):
        out.append(f"- `{f}`")
else:
    out.append("- None detected in plain text.")
out.append("")
out.append("## Command-like Snippets")
if cmd_refs:
    for c in cmd_refs[:80]:
        out.append(f"- `{c}`")
else:
    out.append("- None detected in plain text.")
out.append("")
out.append("## Note")
out.append("- This is a recovery replay from VS Code local session storage.")
out.append("- If you need exact code diffs, we can pair this with git history in the same time window.")

safe = re.sub(r'[^A-Za-z0-9._-]+', '_', title).strip('_') or "session"
report = os.path.abspath(os.path.join(os.getcwd(), "reports", f"vscode-chat-replay-{safe}.md"))
with open(report, "w", encoding="utf-8") as f:
    f.write("\n".join(out))

print(report)
'@

$tmp = Join-Path $env:TEMP "replay_vscode_chat_session.py"
[System.IO.File]::WriteAllText($tmp, $py, [System.Text.UTF8Encoding]::new($false))

$out = & python $tmp $SessionTitle
if ($out -eq "NOT_FOUND") {
    Write-Host "Session not found: $SessionTitle"
    exit 1
}
Write-Host "Report generated: $out"
