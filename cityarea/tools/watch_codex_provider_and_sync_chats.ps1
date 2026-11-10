$ErrorActionPreference = "Stop"

$ccSwitchDbPath = Join-Path $env:USERPROFILE ".cc-switch\cc-switch.db"
$syncScript = "E:\C++pandan\workcode\cityarea\tools\sync_codex_threads_to_current_provider.ps1"

if (-not (Test-Path -LiteralPath $ccSwitchDbPath)) {
    throw "cc-switch db not found: $ccSwitchDbPath"
}
if (-not (Test-Path -LiteralPath $syncScript)) {
    throw "Sync script not found: $syncScript"
}

function Get-CurrentProviderId {
    $pythonCode = @'
import json
import sqlite3
from pathlib import Path

db_path = Path.home() / ".cc-switch" / "cc-switch.db"
con = sqlite3.connect(str(db_path))
cur = con.cursor()
cur.execute("select id from providers where app_type='codex' and is_current=1 limit 1")
row = cur.fetchone()
con.close()
print(row[0] if row else "")
'@
    $value = $pythonCode | python -
    if ($LASTEXITCODE -eq 0) {
        return ($value | Out-String).Trim()
    }
    return $null
}

$lastProvider = Get-CurrentProviderId
if ($lastProvider) {
    try {
        & $syncScript | Out-Null
    } catch {
    }
}

while ($true) {
    Start-Sleep -Seconds 2
    try {
        $currentProvider = Get-CurrentProviderId
        if ($currentProvider -and $currentProvider -ne $lastProvider) {
            & $syncScript | Out-Null
            $lastProvider = $currentProvider
        }
    } catch {
    }
}
