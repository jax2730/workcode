param(
    [Parameter(Mandatory = $true)]
    [string]$SessionTitle
)

$ErrorActionPreference = "Stop"

$outDir = Join-Path $PSScriptRoot "..\reports"
if (!(Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }

$py = @'
import json, os, glob, re, sys, html, subprocess
from datetime import datetime

query = sys.argv[1].strip().lower()
cwd = os.getcwd()
ws_root = os.path.expanduser(r"~\AppData\Roaming\Code\User\workspaceStorage")

def read_jsonl(path):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                yield json.loads(line)
            except Exception:
                continue

def flatten_text(x):
    if x is None: return ""
    if isinstance(x, str): return x
    if isinstance(x, (int,float,bool)): return str(x)
    if isinstance(x, list): return "\n".join(flatten_text(i) for i in x if flatten_text(i))
    if isinstance(x, dict):
        parts = []
        for k in ("text","content","message","response","markdown"):
            if k in x:
                t = flatten_text(x.get(k))
                if t: parts.append(t)
        if parts: return "\n".join(parts)
        try: return json.dumps(x, ensure_ascii=False)
        except: return str(x)
    return str(x)

session_files = glob.glob(os.path.join(ws_root, "**", "chatSessions", "*.jsonl"), recursive=True)
best = None
for fp in session_files:
    title = None
    for r in read_jsonl(fp):
        if r.get("k") == ["customTitle"]:
            title = r.get("v")
            break
    if not title: continue
    lt = str(title).lower()
    score = 0
    if query == lt: score = 1000
    elif query in lt: score = len(query)
    if score > 0 and (best is None or score > best[0]):
        best = (score, fp, title)

if not best:
    print("NOT_FOUND")
    sys.exit(0)

_, session_path, title = best
texts = []
for r in read_jsonl(session_path):
    if r.get("k") in (["requests"], ["responses"]):
        v = r.get("v")
        texts.append(flatten_text(v))

all_text = "\n".join(t for t in texts if t)

# Extract "Edited xxx" and file-like paths
edited_files = set()
for m in re.findall(r'Edited\s+([A-Za-z0-9_./\\-]+\.[A-Za-z0-9_]+)', all_text):
    edited_files.add(m.replace("\\","/"))
for m in re.findall(r'([A-Za-z0-9_./\\-]+\.(?:py|cpp|h|hpp|c|cc|cs|js|ts|tsx|jsx|json|yml|yaml|md))', all_text):
    edited_files.add(m.replace("\\","/"))

def resolve_file(rel):
    cands = [
        os.path.join(cwd, rel),
        os.path.join(cwd, rel.lstrip("./")),
    ]
    for c in cands:
        if os.path.exists(c):
            return os.path.abspath(c)
    return None

resolved = []
for f in sorted(edited_files):
    ap = resolve_file(f)
    if ap:
        resolved.append((f, ap))

def run(cmd):
    try:
        p = subprocess.run(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, encoding="utf-8", errors="replace")
        return p.returncode, p.stdout.strip(), p.stderr.strip()
    except Exception as e:
        return 1, "", str(e)

cards = []
for rel, ap in resolved:
    rel_git = os.path.relpath(ap, cwd).replace("\\","/")
    rc1, diff_out, _ = run(["git", "diff", "--", rel_git])
    rc2, log_out, _ = run(["git", "log", "-n", "1", "--pretty=format:%h %ad %an %s", "--date=iso", "--", rel_git])
    if not diff_out:
        rc3, show_out, _ = run(["git", "show", "HEAD~1..HEAD", "--", rel_git])
        diff_out = show_out
    cards.append({
        "rel": rel,
        "abs": ap,
        "git_rel": rel_git,
        "log": log_out if log_out else "(no git log found)",
        "diff": diff_out if diff_out else "(no working-tree diff found)"
    })

def esc(s): return html.escape(s or "")

rows = []
for i, c in enumerate(cards, 1):
    rows.append(f"""
<details class="card">
  <summary><span class="badge">Edited</span> {esc(c['rel'])}</summary>
  <div class="meta">Path: <code>{esc(c['abs'])}</code></div>
  <div class="meta">Git: <code>{esc(c['git_rel'])}</code></div>
  <div class="meta">Last commit: <code>{esc(c['log'])}</code></div>
  <h4>Diff</h4>
  <pre>{esc(c['diff'])}</pre>
</details>
""")

html_doc = f"""<!doctype html>
<html lang="zh">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Session Replay - {esc(title)}</title>
  <style>
    body {{ font-family: Segoe UI, Arial, sans-serif; margin: 24px; background:#0b0f14; color:#e6edf3; }}
    .wrap {{ max-width: 1200px; margin: 0 auto; }}
    h1 {{ margin: 0 0 8px 0; font-size: 26px; }}
    .sub {{ color:#8b949e; margin-bottom: 20px; }}
    .badge {{ display:inline-block; background:#1f6feb; color:white; border-radius:10px; padding:2px 8px; font-size:12px; margin-right:8px; }}
    .card {{ border:1px solid #30363d; border-radius:10px; padding:10px 12px; margin-bottom:12px; background:#11161c; }}
    summary {{ cursor:pointer; font-weight:600; outline:none; }}
    .meta {{ margin-top:8px; color:#9da7b1; font-size:13px; }}
    pre {{ white-space:pre-wrap; word-break:break-word; background:#0d1117; border:1px solid #30363d; border-radius:8px; padding:12px; overflow:auto; }}
    code {{ color:#c9d1d9; }}
  </style>
</head>
<body>
  <div class="wrap">
    <h1>{esc(title)}</h1>
    <div class="sub">Source: <code>{esc(session_path)}</code><br/>Generated: <code>{datetime.now().isoformat(timespec='seconds')}</code></div>
    <p>下面是从该 session 文本里提取到的 <strong>Edited 文件</strong>，每个条目可展开查看当前 diff / 最近提交信息。</p>
    {''.join(rows) if rows else '<p>未提取到可解析的 Edited 文件，可能该会话存储的是结构化片段而非纯文本。</p>'}
  </div>
</body>
</html>
"""

safe = re.sub(r'[^A-Za-z0-9._-]+', '_', title).strip('_') or "session"
out_path = os.path.abspath(os.path.join(cwd, "reports", f"session-replay-enhanced-{safe}.html"))
with open(out_path, "w", encoding="utf-8") as f:
    f.write(html_doc)

print(out_path)
'@

$tmp = Join-Path $env:TEMP "replay_vscode_session_enhanced.py"
[System.IO.File]::WriteAllText($tmp, $py, [System.Text.UTF8Encoding]::new($false))
$out = & python $tmp $SessionTitle
if ($out -eq "NOT_FOUND") {
    Write-Host "Session not found: $SessionTitle"
    exit 1
}
Write-Host "Enhanced HTML generated: $out"
