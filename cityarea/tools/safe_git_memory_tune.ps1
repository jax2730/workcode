param(
    [switch]$ApplyDefenderExclusions,
    [switch]$CleanupZombieGit,
    [switch]$CleanupDuplicatedGit
)

$ErrorActionPreference = "Stop"

function Write-Info($msg) { Write-Host "[INFO] $msg" -ForegroundColor Cyan }
function Write-Warn($msg) { Write-Host "[WARN] $msg" -ForegroundColor Yellow }
function Write-Ok($msg)   { Write-Host "[OK]   $msg" -ForegroundColor Green }

Write-Info "Start safe Git/VSCode memory tune..."

# 1) Global git performance knobs (low risk)
$gitConfigs = @(
    @("core.fsmonitor", "true"),
    @("feature.manyFiles", "true"),
    @("gc.auto", "50")
)

foreach ($pair in $gitConfigs) {
    $key = $pair[0]
    $val = $pair[1]
    git config --global $key $val
    Write-Ok "git config --global $key $val"
}

# 2) VS Code user settings adjustments (low risk, reversible)
$settingsPath = Join-Path $env:APPDATA "Code\User\settings.json"
if (Test-Path $settingsPath) {
    $raw = Get-Content -LiteralPath $settingsPath -Raw
    $obj = $raw | ConvertFrom-Json
    $settings = @{}
    if ($null -ne $obj) {
        $obj.PSObject.Properties | ForEach-Object {
            $settings[$_.Name] = $_.Value
        }
    }
} else {
    Write-Warn "VS Code settings not found, create new: $settingsPath"
    $dir = Split-Path -Parent $settingsPath
    if (!(Test-Path $dir)) { New-Item -ItemType Directory -Path $dir | Out-Null }
    $settings = @{}
}

$settings["git.autofetch"] = $false
$settings["git.autoRepositoryDetection"] = "openEditors"
$settings["git.untrackedChanges"] = "hidden"

$json = $settings | ConvertTo-Json -Depth 100
[System.IO.File]::WriteAllText($settingsPath, $json, [System.Text.UTF8Encoding]::new($false))
Write-Ok "Updated VS Code git settings: $settingsPath"

# 3) Diagnose git-like processes and parent process
$procs = Get-CimInstance Win32_Process |
    Where-Object { $_.Name -match "^(git|bash|sh|ssh-agent|gpg-agent)(\.exe)?$" } |
    Select-Object ProcessId, ParentProcessId, Name, CommandLine, WorkingSetSize

if ($procs.Count -eq 0) {
    Write-Info "No git-like process currently running."
} else {
    Write-Info "Git-like process snapshot:"
    $procs |
        Sort-Object WorkingSetSize -Descending |
        Select-Object Name, ProcessId, ParentProcessId,
            @{Name="MemMB";Expression={[math]::Round($_.WorkingSetSize/1MB,1)}},
            CommandLine |
        Format-Table -AutoSize
}

# 4) Optional: cleanup suspicious zombie git subprocesses only
# Safety policy:
# - Never kill parent GUI processes (Code.exe, Codex.exe, etc.)
# - Only kill process names: git.exe/bash.exe/sh.exe
# - Only when memory > 200MB and parent process is missing or already exited
if ($CleanupZombieGit) {
    Write-Info "CleanupZombieGit enabled. Applying conservative cleanup..."
    $killed = 0
    foreach ($p in $procs) {
        if ($p.Name -notmatch "^(git|bash|sh)(\.exe)?$") { continue }
        $memMB = [math]::Round($p.WorkingSetSize/1MB,1)
        if ($memMB -lt 200) { continue }

        $parent = Get-Process -Id $p.ParentProcessId -ErrorAction SilentlyContinue
        if ($null -ne $parent) { continue }

        try {
            Stop-Process -Id $p.ProcessId -Force
            Write-Ok "Killed zombie $($p.Name) pid=$($p.ProcessId) mem=${memMB}MB"
            $killed++
        } catch {
            Write-Warn "Failed to kill pid=$($p.ProcessId): $($_.Exception.Message)"
        }
    }
    if ($killed -eq 0) {
        Write-Info "No zombie git subprocess matched cleanup conditions."
    }
}

# 5) Optional: cleanup duplicated heavy git workers (safe-conservative)
# Policy:
# - Only target git.exe
# - Group by exact command line
# - If same command has >1 process and each process mem >= 500MB:
#   keep newest one, kill older duplicates
if ($CleanupDuplicatedGit) {
    Write-Info "CleanupDuplicatedGit enabled. Removing duplicated heavy git workers..."
    $allGit = Get-CimInstance Win32_Process |
        Where-Object { $_.Name -match "^git(\.exe)?$" -and $_.CommandLine } |
        Select-Object ProcessId, ParentProcessId, Name, CommandLine, WorkingSetSize, CreationDate

    $killedDup = 0
    $groups = $allGit | Group-Object CommandLine
    foreach ($g in $groups) {
        $heavy = $g.Group | Where-Object { ([math]::Round($_.WorkingSetSize/1MB,1) -ge 500) }
        if ($heavy.Count -le 1) { continue }

        $sorted = $heavy | Sort-Object CreationDate -Descending
        $keep = $sorted | Select-Object -First 1
        $toKill = $sorted | Select-Object -Skip 1

        foreach ($p in $toKill) {
            try {
                Stop-Process -Id $p.ProcessId -Force
                $memMB = [math]::Round($p.WorkingSetSize/1MB,1)
                Write-Ok "Killed duplicated git pid=$($p.ProcessId) mem=${memMB}MB"
                $killedDup++
            } catch {
                Write-Warn "Failed to kill duplicated pid=$($p.ProcessId): $($_.Exception.Message)"
            }
        }
        Write-Info "Kept newest duplicated git pid=$($keep.ProcessId)"
    }

    if ($killedDup -eq 0) {
        Write-Info "No duplicated heavy git workers matched cleanup conditions."
    }
}

# 5) Defender exclusions are useful but optional due to security policy
$suggestedExclusions = @(
    "E:\C++pandan\workcode",
    "$env:USERPROFILE\.codex",
    "$env:USERPROFILE\.cc-switch"
)

if ($ApplyDefenderExclusions) {
    Write-Info "ApplyDefenderExclusions enabled. Attempting to add exclusions..."
    foreach ($path in $suggestedExclusions) {
        if (Test-Path $path) {
            try {
                Add-MpPreference -ExclusionPath $path
                Write-Ok "Defender exclusion added: $path"
            } catch {
                Write-Warn "Cannot add exclusion for $path (maybe requires admin): $($_.Exception.Message)"
            }
        } else {
            Write-Warn "Path not found, skip exclusion: $path"
        }
    }
} else {
    Write-Info "Defender exclusions not applied (safe default). Suggested paths:"
    $suggestedExclusions | ForEach-Object { Write-Host "  - $_" }
}

Write-Info "Done."
Write-Host ""
Write-Host "Recommended next steps:"
Write-Host "1) Restart VS Code to load new git settings."
Write-Host "2) If memory still spikes, run:"
Write-Host "   powershell -ExecutionPolicy Bypass -File .\tools\safe_git_memory_tune.ps1 -CleanupZombieGit"
Write-Host "3) To cleanup duplicated heavy git workers safely, run:"
Write-Host "   powershell -ExecutionPolicy Bypass -File .\tools\safe_git_memory_tune.ps1 -CleanupDuplicatedGit"
Write-Host "4) If you accept Defender tuning, run:"
Write-Host "   powershell -ExecutionPolicy Bypass -File .\tools\safe_git_memory_tune.ps1 -ApplyDefenderExclusions"
