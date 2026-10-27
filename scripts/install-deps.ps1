<#
.SYNOPSIS
    Installs all dependencies required to build Pomodoro Desktop App on Windows.
    Run as Administrator (required for winget).

.DESCRIPTION
    This script installs:
    - CMake (via winget)
    - WinLibs GCC 16.2 (MinGW-w64, UCRT, POSIX threads) — copied to C:\pbuild\mingw64 to avoid space-in-path issues
    - Python 3.12 (for GIF regeneration)
    - Git (if not present)

.NOTES
    WinLibs GCC fails when the install path contains spaces (e.g., C:\Users\My Name\...).
    This script works around that by copying the toolchain to C:\pbuild\mingw64.
#>

# Require Administrator
if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error "This script must be run as Administrator. Right-click PowerShell → 'Run as Administrator'."
    exit 1
}

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  Pomodoro Desktop App — Dependency Installer" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# --- Helper: install via winget ---
function Install-WingetPackage {
    param(
        [string]$Id,
        [string]$Name
    )
    Write-Host "Installing $Name..." -ForegroundColor Yellow
    try {
        winget install --id $Id -e --silent --accept-package-agreements --accept-source-agreements
        Write-Host "  ✓ $Name installed" -ForegroundColor Green
    } catch {
        Write-Warning "  ⚠ $Name may already be installed or failed: $_"
    }
}

# --- 1. CMake ---
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Install-WingetPackage "Kitware.CMake" "CMake"
} else {
    Write-Host "✓ CMake already installed: $(cmake --version | Select-Object -First 1)" -ForegroundColor Green
}

# --- 2. WinLibs GCC (MinGW) ---
$mingwTarget = "C:\pbuild\mingw64"
$wingetMingwId = "BrechtSanders.WinLibs.POSIX.UCRT"

if (-not (Test-Path "$mingwTarget\bin\gcc.exe")) {
    Write-Host "Installing WinLibs GCC (MinGW-w64 UCRT)..." -ForegroundColor Yellow
    try {
        winget install --id $wingetMingwId -e --silent --accept-package-agreements --accept-source-agreements
    } catch {
        Write-Warning "winget install may have failed, continuing..."
    }

    # Find where winget actually installed it
    $wingetPkgPath = "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\${wingetMingwId}_Microsoft.Winget.Source_8wekyb3d8bbwe"
    $installedMingw = Get-ChildItem "$wingetPkgPath" -Directory -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
    if ($installedMingw) {
        $installedMingw = Join-Path $installedMingw "mingw64"
        if (Test-Path "$installedMingw\bin\gcc.exe") {
            Write-Host "Copying MinGW to space-free path: $mingwTarget" -ForegroundColor Yellow
            if (Test-Path $mingwTarget) { Remove-Item $mingwTarget -Recurse -Force -ErrorAction SilentlyContinue }
            Copy-Item $installedMingw $mingwTarget -Recurse -Force
            Write-Host "  ✓ MinGW copied to $mingwTarget" -ForegroundColor Green
        } else {
            Write-Error "Could not find mingw64 in winget package: $installedMingw"
            exit 1
        }
    } else {
        Write-Error "WinLibs package not found in winget cache. Try installing manually."
        exit 1
    }
} else {
    Write-Host "✓ MinGW already at $mingwTarget" -ForegroundColor Green
}

# Verify GCC
& "$mingwTarget\bin\gcc.exe" --version | Select-Object -First 1 | ForEach-Object { Write-Host "  GCC: $_" -ForegroundColor Green }

# --- 3. Python ---
if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
    Install-WingetPackage "Python.Python.3.12" "Python 3.12"
} else {
    Write-Host "✓ Python already installed: $(python --version)" -ForegroundColor Green
}

# --- 4. Git ---
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Install-WingetPackage "Git.Git" "Git"
} else {
    Write-Host "✓ Git already installed: $(git --version)" -ForegroundColor Green
}

# --- 5. Verify Ninja (comes with WinLibs) ---
if (Test-Path "$mingwTarget\bin\ninja.exe") {
    Write-Host "✓ Ninja found: $mingwTarget\bin\ninja.exe" -ForegroundColor Green
} else {
    Write-Warning "Ninja not found in MinGW — build may fail."
}

Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  All dependencies installed!" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Open a NEW PowerShell window (PATH must refresh)"
Write-Host "  2. cd to PomodoroApp folder"
Write-Host "  3. Run the build commands from README.md" -ForegroundColor Yellow
Write-Host ""
Write-Host "MinGW path for CMake:" -ForegroundColor Cyan
Write-Host "  \$gcc = \"C:\pbuild\mingw64\bin\"" -ForegroundColor Gray
Write-Host ""