# USD Environment Setup Script
# Run this script before using USD: . .\env\setup_usd_env.ps1

$USD_ROOT = "D:\Softwares\USD" # Change this path to your USD installation directory

# Set PYTHONPATH to USD's Python modules
$env:PYTHONPATH = "$USD_ROOT\lib\python"

# Remove existing USD paths from PATH to avoid duplicates
$env:PATH = $env:PATH -replace [regex]::Escape("$USD_ROOT\lib;"), ""
$env:PATH = $env:PATH -replace [regex]::Escape("$USD_ROOT\bin;"), ""
$env:PATH = $env:PATH -replace [regex]::Escape("$USD_ROOT\python\Scripts;"), ""
$env:PATH = $env:PATH -replace [regex]::Escape("$USD_ROOT\python;"), ""

# Add USD paths to the FRONT of PATH (highest priority)
$env:PATH = "$USD_ROOT\python;$USD_ROOT\python\Scripts;$USD_ROOT\bin;$USD_ROOT\lib;" + $env:PATH

Write-Host "USD Environment configured successfully!" -ForegroundColor Green
Write-Host "PYTHONPATH: $env:PYTHONPATH" -ForegroundColor Cyan
Write-Host "USD Root: $USD_ROOT" -ForegroundColor Cyan

# Check and install required Python packages
Write-Host "`nChecking required Python packages..." -ForegroundColor Yellow

# Check PyOpenGL
$pyopenglInstalled = & "$USD_ROOT\python\python.exe" -c "import PyOpenGL" 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "Installing PyOpenGL..." -ForegroundColor Yellow
    & "$USD_ROOT\python\python.exe" -m pip install PyOpenGL --quiet
    Write-Host "PyOpenGL installed successfully!" -ForegroundColor Green
} else {
    Write-Host "PyOpenGL is already installed" -ForegroundColor Green
}

# Check PyOpenGL_accelerate
$pyopenglAccelInstalled = & "$USD_ROOT\python\python.exe" -c "import PyOpenGL_accelerate" 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "Installing PyOpenGL_accelerate..." -ForegroundColor Yellow
    & "$USD_ROOT\python\python.exe" -m pip install PyOpenGL_accelerate --quiet
    Write-Host "PyOpenGL_accelerate installed successfully!" -ForegroundColor Green
} else {
    Write-Host "PyOpenGL_accelerate is already installed" -ForegroundColor Green
}

# Test USD import
Write-Host "`nTesting USD import..." -ForegroundColor Yellow
& "$USD_ROOT\python\python.exe" -c "from pxr import Usd; print('USD version:', Usd.GetVersion())"

# Verify Python version
Write-Host "`nVerifying Python version..." -ForegroundColor Yellow
$pythonVersion = python --version 2>&1
Write-Host "Current 'python' points to: $pythonVersion" -ForegroundColor Cyan
if ($pythonVersion -notlike "*3.11.11*") {
    Write-Host "WARNING: 'python' command is not using USD's Python 3.11.11!" -ForegroundColor Red
    Write-Host "This script may not have been sourced correctly." -ForegroundColor Yellow
    Write-Host "Please run with: . .\env\setup_usd_env.ps1 (note the leading dot)" -ForegroundColor Yellow
} else {
    Write-Host "Python environment is correctly configured!" -ForegroundColor Green
}
