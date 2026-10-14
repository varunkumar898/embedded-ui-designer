# Embedded UI Designer: Windows x86_64 PowerShell Environment Setup
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  Embedded UI Designer: Windows x86_64 Setup" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan

# Check for Chocolatey or winget
if (Get-Command choco -ErrorAction SilentlyContinue) {
    Write-Host "[1/4] Installing dependencies via Chocolatey..." -ForegroundColor Green
    choco install cmake ninja llvm nodejs -y
} elseif (Get-Command winget -ErrorAction SilentlyContinue) {
    Write-Host "[1/4] Installing dependencies via winget..." -ForegroundColor Green
    winget install Kitware.CMake Ninja-build.Ninja LLVM.LLVM OpenJS.NodeJS --accept-source-agreements --accept-package-agreements
} else {
    Write-Host "[1/4] Please install CMake, Ninja, and LLVM/MSVC manually." -ForegroundColor Yellow
}

Write-Host "[2/4] Verifying CMake installation..." -ForegroundColor Green
cmake --version

Write-Host "[3/4] Configuring build directory..." -ForegroundColor Green
if (!(Test-Path -Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}
Set-Location build
cmake -G "Ninja" -DPLATFORM=Windows -DARCH=x86_64 -DENABLE_TESTS=ON ..

Write-Host "[4/4] Building test binaries..." -ForegroundColor Green
cmake --build . --config Release --target test_project_model test_codegen_multi_language test_security_performance test_compliance_observability

Write-Host "✓ Windows x86_64 setup complete!" -ForegroundColor Green
Set-Location ..
