param(
    [Parameter(Mandatory = $true)]
    [string]$SessionTitle,
    [int]$MaxTurns = 12
)

$ErrorActionPreference = "Stop"
$outDir = Join-Path $PSScriptRoot "..\reports"
if (!(Test-Path $outDir)) { New-Item -ItemType Directory -Path $outDir | Out-Null }

$py = @'
import json, os, glob, re, sys, html, subprocess
from datetime import datetime

query = sys.argv[1].strip().lower()
max_turns = int(sys.argv[2]) if len(sys.argv) > 2 else 12
repo = os.getcwd()
ws_root = os.path.expanduser(r"~\AppData\Roaming\Code\User\workspaceStorage")

def run(cmd):
    p = subprocess.run(cmd, cwd=repo, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, encoding="utf-8", errors="replace")
    return p.returncode, p.stdout, p.stderr

def read_jsonl(path):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line=line.strip()
            if not line: continue
            try: yield json.loads(line)
            except: continue

def flatten(x):
    if x is None: return ""
    if isinstance(x,str): return x
    if isinstance(x,(int,float,bool)): return str(x)
    if isinstance(x,list): return "\n".join(flatten(i) for i in x if flatten(i))
    if isinstance(x,dict):
        parts=[]
        for k in ("text","content","message","response","markdown","value"):
            if k in x:
                t=flatten(x.get(k))
                if t: parts.append(t)
        if parts: return "\n".join(parts)
        try: return json.dumps(x, ensure_ascii=False)
        except: return str(x)
    return str(x)

session_files = glob.glob(os.path.join(ws_root, "**", "chatSessions", "*.jsonl"), recursive=True)
target = None
for fp in session_files:
    title = None
    for r in read_jsonl(fp):
        if r.get("k")==["customTitle"]:
            title = r.get("v")
            break
    if not title: continue
    lt = str(title).lower()
    if query == lt or query in lt:
        target = (fp, title)
        if query == lt: break

if not target:
    print("NOT_FOUND")
    sys.exit(0)

session_path, title = target
events = []
session_creation = None

for r in read_jsonl(session_path):
    k = r.get("k")
    v = r.get("v")
    if r.get("kind")==0 and isinstance(v,dict):
        session_creation = v.get("creationDate")
    if k == ["requests"] and isinstance(v, list):
        for idx, req in enumerate(v):
            ts = req.get("timestamp")
            user_text = flatten(req.get("message") or "")
            resp_text = flatten(req.get("response") or "")
            if user_text or resp_text:
                events.append({
                    "idx": idx,
                    "ts": ts,
                    "user": user_text,
                    "assistant": resp_text
                })

events = sorted(events, key=lambda x: x["ts"] or 0)
if not events:
    print("NO_EVENTS")
    sys.exit(0)
if len(events) > max_turns:
    events = events[-max_turns:]

# Build windows: event[i].ts .. event[i+1].ts
windows = []
for i, e in enumerate(events):
    start_ms = e["ts"] or session_creation or 0
    end_ms = events[i+1]["ts"] if i+1 < len(events) else int(datetime.now().timestamp()*1000)
    windows.append((start_ms, end_ms, e))

def ms_to_iso(ms):
    try:
        return datetime.fromtimestamp(ms/1000).isoformat(sep=' ', timespec='seconds')
    except:
        return str(ms)

def git_changes_between(start_iso, end_iso):
    rc, out, err = run([
        "git","log","--since",start_iso,"--until",end_iso,
        "--name-only","--pretty=format:@@@%H|%ad|%an|%s","--date=iso"
    ])
    commits = []
    cur = None
    for line in out.splitlines():
        if line.startswith("@@@"):
            if cur: commits.append(cur)
            h, ad, an, s = (line[3:].split("|",3)+["","","",""])[:4]
            cur = {"hash":h,"date":ad,"author":an,"subject":s,"files":[]}
        elif cur is not None:
            t=line.strip()
            if t:
                cur["files"].append(t)
    if cur: commits.append(cur)
    return commits

timeline = []
for start_ms, end_ms, e in windows:
    start_iso = ms_to_iso(start_ms)
    end_iso = ms_to_iso(end_ms)
    commits = git_changes_between(start_iso, end_iso)
    files = sorted({f for c in commits for f in c["files"]})
    timeline.append({
        "idx": e["idx"],
        "start_iso": start_iso,
        "end_iso": end_iso,
        "user": e["user"],
        "assistant": e["assistant"],
        "commits": commits,
        "files": files
    })

def esc(s): return html.escape(s or "")

left_items = []
right_items = []
for i, t in enumerate(timeline):
    aid = f"item{i}"
    left_items.append(f'<a class="tl-item" href="#{aid}"><span class="n">{i+1}</span><span>{esc(t["start_iso"])}</span></a>')
    commits_html = "".join(
        f"<li><code>{esc(c['hash'][:10])}</code> {esc(c['date'])} {esc(c['author'])} {esc(c['subject'])}</li>"
        for c in t["commits"]
    ) or "<li>(no commit detected in this window)</li>"

    files_html_parts = []
    if t["files"]:
        for f in t["files"]:
            files_html_parts.append(f"""
            <details class='file'>
              <summary><span class='edited'>Edited</span> {esc(f)}</summary>
              <pre>Run to view hunks:
git log --since "{esc(t['start_iso'])}" --until "{esc(t['end_iso'])}" -p --unified=0 -- "{esc(f)}"</pre>
            </details>
            """)
    else:
        files_html_parts.append("<div class='muted'>(no file-level hunks in this window)</div>")

    right_items.append(f"""
    <section id="{aid}" class="card">
      <h3>Turn {i+1}</h3>
      <div class="meta">Window: <code>{esc(t['start_iso'])}</code> ~ <code>{esc(t['end_iso'])}</code></div>
      <h4>User</h4>
      <pre>{esc(t['user'])}</pre>
      <h4>Assistant</h4>
      <pre>{esc(t['assistant'])}</pre>
      <h4>Commits In Window</h4>
      <ul>{commits_html}</ul>
      <h4>Files & Hunks</h4>
      {''.join(files_html_parts)}
    </section>
    """)

doc = f"""<!doctype html>
<html lang='zh'>
<head>
<meta charset='utf-8'/>
<meta name='viewport' content='width=device-width, initial-scale=1'/>
<title>Native Replay - {esc(title)}</title>
<style>
body {{ margin:0; font-family:Segoe UI,Arial,sans-serif; background:#0b0f14; color:#e6edf3; }}
.layout {{ display:grid; grid-template-columns:320px 1fr; min-height:100vh; }}
.left {{ border-right:1px solid #30363d; padding:16px; position:sticky; top:0; height:100vh; overflow:auto; }}
.right {{ padding:18px; overflow:auto; }}
.title {{ font-size:18px; font-weight:700; margin-bottom:8px; }}
.sub {{ color:#9aa4af; font-size:12px; margin-bottom:14px; }}
.tl-item {{ display:flex; gap:10px; align-items:center; text-decoration:none; color:#c9d1d9; padding:8px; border-radius:8px; }}
.tl-item:hover {{ background:#161b22; }}
.n {{ background:#1f6feb; color:#fff; border-radius:999px; width:22px; height:22px; display:inline-flex; align-items:center; justify-content:center; font-size:12px; }}
.card {{ border:1px solid #30363d; border-radius:10px; padding:12px; margin-bottom:14px; background:#11161c; }}
.meta {{ color:#9aa4af; font-size:12px; margin-bottom:8px; }}
pre {{ white-space:pre-wrap; word-break:break-word; background:#0d1117; border:1px solid #30363d; border-radius:8px; padding:10px; }}
.file {{ margin:8px 0; border:1px solid #2f3944; border-radius:8px; padding:6px 8px; background:#0f141a; }}
.file summary {{ cursor:pointer; font-weight:600; }}
.edited {{ background:#1f6feb; color:#fff; border-radius:10px; padding:2px 8px; font-size:12px; margin-right:8px; }}
.muted {{ color:#9aa4af; }}
code {{ color:#c9d1d9; }}
</style>
</head>
<body>
<div class="layout">
  <aside class="left">
    <div class="title">{esc(title)}</div>
    <div class="sub">Source: <code>{esc(session_path)}</code><br/>Generated: <code>{datetime.now().isoformat(timespec='seconds')}</code></div>
    {''.join(left_items)}
  </aside>
  <main class="right">
    {''.join(right_items)}
  </main>
</div>
</body>
</html>"""

safe = re.sub(r'[^A-Za-z0-9._-]+','_',title).strip('_') or 'session'
out_path = os.path.abspath(os.path.join(repo, "reports", f"native-replay-{safe}.html"))
with open(out_path, "w", encoding="utf-8") as f:
    f.write(doc)
print(out_path)
'@

$tmp = Join-Path $env:TEMP "replay_vscode_session_timeline_git.py"
[System.IO.File]::WriteAllText($tmp, $py, [System.Text.UTF8Encoding]::new($false))
$out = & python $tmp $SessionTitle $MaxTurns
if ($out -eq "NOT_FOUND") { Write-Host "Session not found: $SessionTitle"; exit 1 }
if ($out -eq "NO_EVENTS") { Write-Host "Session has no request events: $SessionTitle"; exit 1 }
Write-Host "Native replay HTML generated: $out"
