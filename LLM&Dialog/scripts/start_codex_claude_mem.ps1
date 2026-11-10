param(
  [int]$BridgeIntervalSeconds = 10,
  [string]$ClaudeMemVersion = "12.3.9"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$dataRoot = Join-Path $env:USERPROFILE ".claude-mem"
$bridgeRoot = Join-Path $dataRoot "codex-desktop-bridge"
$bridgeScript = Join-Path $PSScriptRoot "codex_desktop_claude_mem_bridge.py"
$pidPath = Join-Path $bridgeRoot "bridge.pid"
$logPath = Join-Path $bridgeRoot "bridge.log"
$errPath = Join-Path $bridgeRoot "bridge.err.log"

New-Item -ItemType Directory -Force -Path $bridgeRoot | Out-Null

Push-Location $repoRoot
try {
  npx -y "claude-mem@$ClaudeMemVersion" start | Out-Null
}
finally {
  Pop-Location
}

$existingBridge = $null
if (Test-Path $pidPath) {
  $pidText = (Get-Content -Raw $pidPath).Trim()
  if ($pidText -match '^\d+$') {
    $existingBridge = Get-Process -Id ([int]$pidText) -ErrorAction SilentlyContinue
  }
}

if (-not $existingBridge) {
  $python = (Get-Command python -ErrorAction Stop).Source
  $args = @(
    "-u",
    $bridgeScript,
    "--watch",
    "--interval",
    [string]$BridgeIntervalSeconds
  )
  $process = Start-Process -FilePath $python `
    -ArgumentList $args `
    -WorkingDirectory $repoRoot `
    -WindowStyle Hidden `
    -RedirectStandardOutput $logPath `
    -RedirectStandardError $errPath `
    -PassThru
  Set-Content -Path $pidPath -Value ([string]$process.Id) -Encoding ascii
}

Write-Output "claude-mem worker requested; Codex bridge watcher is running or already active."
