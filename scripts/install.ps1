<#
.SYNOPSIS
    Installs Pomodoro (pre-built) to your local programs folder and creates
    Desktop + Start Menu shortcuts.

.DESCRIPTION
    Downloads the latest release zip from GitHub, extracts it to
        %LOCALAPPDATA%\Programs\Pomodoro
    and creates shortcuts so you can launch it from the Desktop / Start Menu.
    No admin rights required. No dependencies required.

.EXAMPLE
    # One-liner (no download, no admin)
    irm https://raw.githubusercontent.com/tahergaming13/pomodoro-gifs/main/scripts/install.ps1 | iex

.EXAMPLE
    # Or from a clone
    .\scripts\install.ps1 -Uninstall
#>

[CmdletBinding()]
param(
    [string]$Version = "v1.0.0",
    [string]$InstallDir = "$env:LOCALAPPDATA\Programs\Pomodoro",
    [switch]$Uninstall,
    [switch]$NoShortcuts
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$Repo    = "tahergaming13/pomodoro-gifs"
$ExeName = "Pomodoro.exe"
$ZipName = "Pomodoro-$Version-win64.zip"

function New-Shortcut {
    param([string]$LinkPath, [string]$TargetPath, [string]$WorkingDir, [string]$IconPath)

    $shell = New-Object -ComObject WScript.Shell
    $sc = $shell.CreateShortcut($LinkPath)
    $sc.TargetPath       = $TargetPath
    $sc.WorkingDirectory = $WorkingDir
    $sc.IconLocation     = $IconPath
    $sc.Description      = "Floating Pomodoro timer"
    $sc.Save()
}

function Remove-OurShortcuts {
    $desktop   = [Environment]::GetFolderPath("Desktop")
    $startMenu = Join-Path ([Environment]::GetFolderPath("Programs")) "Pomodoro"

    foreach ($lnk in @("$desktop\Pomodoro.lnk")) {
        if (Test-Path $lnk) { Remove-Item $lnk -Force; Write-Host "  removed $lnk" }
    }
    if (Test-Path $startMenu) { Remove-Item $startMenu -Recurse -Force; Write-Host "  removed $startMenu" }
}

# ---------------------------------------------------------------------------
# Uninstall
# ---------------------------------------------------------------------------
if ($Uninstall) {
    Write-Host "Uninstalling Pomodoro..." -ForegroundColor Yellow
    Remove-OurShortcuts
    if (Test-Path $InstallDir) {
        Remove-Item $InstallDir -Recurse -Force
        Write-Host "  removed $InstallDir" -ForegroundColor Green
    }
    Write-Host "Done. Pomodoro has been uninstalled." -ForegroundColor Green
    return
}

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  Pomodoro Installer ($Version)" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# A running instance would lock the .exe
Get-Process -Name "Pomodoro" -ErrorAction SilentlyContinue | ForEach-Object {
    Write-Host "Closing running instance (PID $($_.Id))..." -ForegroundColor Yellow
    $_.CloseMainWindow() | Out-Null
    Start-Sleep -Milliseconds 400
    if (-not $_.HasExited) { $_.Kill() }
}

$tmp = Join-Path $env:TEMP "pomodoro-install"
if (Test-Path $tmp) { Remove-Item $tmp -Recurse -Force }
New-Item -ItemType Directory -Path $tmp -Force | Out-Null

$url = "https://github.com/$Repo/releases/download/$Version/$ZipName"
$zip = Join-Path $tmp $ZipName

Write-Host "Downloading $ZipName ..." -ForegroundColor Yellow
try {
    Invoke-WebRequest -Uri $url -OutFile $zip -UseBasicParsing
} catch {
    Write-Error "Download failed: $url`n$($_.Exception.Message)"
    exit 1
}
Write-Host "  OK ($([math]::Round((Get-Item $zip).Length / 1MB, 1)) MB)" -ForegroundColor Green

Write-Host "Extracting to $InstallDir ..." -ForegroundColor Yellow
if (-not (Test-Path $InstallDir)) { New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null }
Expand-Archive -Path $zip -DestinationPath $InstallDir -Force

$exe = Join-Path $InstallDir $ExeName
if (-not (Test-Path $exe)) {
    Write-Error "Install incomplete: $exe not found after extraction."
    exit 1
}
Write-Host "  OK" -ForegroundColor Green

Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue

if (-not $NoShortcuts) {
    Write-Host "Creating shortcuts ..." -ForegroundColor Yellow

    $desktop = [Environment]::GetFolderPath("Desktop")
    New-Shortcut -LinkPath (Join-Path $desktop "Pomodoro.lnk") `
                 -TargetPath $exe -WorkingDir $InstallDir -IconPath $exe
    Write-Host "  Desktop\Pomodoro.lnk" -ForegroundColor Green

    $startDir = Join-Path ([Environment]::GetFolderPath("Programs")) "Pomodoro"
    if (-not (Test-Path $startDir)) { New-Item -ItemType Directory -Path $startDir -Force | Out-Null }
    New-Shortcut -LinkPath (Join-Path $startDir "Pomodoro.lnk") `
                 -TargetPath $exe -WorkingDir $InstallDir -IconPath $exe
    Write-Host "  Start Menu\Pomodoro\Pomodoro.lnk" -ForegroundColor Green
}

Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  Installed successfully!" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "  Launch:  $exe"
Write-Host "  Quit:    right-click the widget -> Quit"
Write-Host "  Hotkey:  Ctrl+Alt+P  (start / pause)"
Write-Host "  Uninstall:  .\scripts\install.ps1 -Uninstall"
Write-Host ""

$answer = Read-Host "Launch Pomodoro now? (y/N)"
if ($answer -match '^(y|yes)$') {
    Start-Process -FilePath $exe -WorkingDirectory $InstallDir
}
