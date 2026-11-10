$ErrorActionPreference = "Stop"

$watchScript = "E:\C++pandan\workcode\cityarea\tools\watch_codex_provider_and_sync_chats.ps1"

if (-not (Test-Path -LiteralPath $watchScript)) {
    throw "Watcher script not found: $watchScript"
}

$existing = Get-CimInstance Win32_Process -ErrorAction SilentlyContinue |
    Where-Object { $_.CommandLine -like '*watch_codex_provider_and_sync_chats.ps1*' }

if (-not $existing) {
    Start-Process powershell `
        -ArgumentList '-NoProfile','-ExecutionPolicy','Bypass','-File',$watchScript `
        -WindowStyle Hidden
}
