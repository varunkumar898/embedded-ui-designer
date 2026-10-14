# Embedded UI Designer: Windows Cross-Compile to ARM (PowerShell)
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  Embedded UI Designer: Windows Cross-Compile to ARM" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan

# Verify GNU Arm Embedded Toolchain
if (!(Get-Command arm-none-eabi-gcc -ErrorAction SilentlyContinue)) {
    Write-Host "[1/3] ARM GCC toolchain not found in PATH. Installing via choco..." -ForegroundColor Yellow
    if (Get-Command choco -ErrorAction SilentlyContinue) {
        choco install gcc-arm-embedded cmake ninja -y
    } else {
        Write-Host "Please download and install arm-none-eabi-gcc toolchain." -ForegroundColor Red
    }
} else {
    Write-Host "[1/3] Found arm-none-eabi-gcc" -ForegroundColor Green
    arm-none-eabi-gcc --version | Select-Object -First 1
}

Write-Host "[2/3] Configuring CMake with ARM toolchain..." -ForegroundColor Green
if (!(Test-Path -Path "build-arm")) {
    New-Item -ItemType Directory -Path "build-arm" | Out-Null
}
Set-Location build-arm
cmake -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/arm-toolchain.cmake -DPLATFORM=Embedded -DARCH=ARM ..

Write-Host "[3/3] Cross-compiling target firmware..." -ForegroundColor Green
cmake --build . --config Release

Write-Host "✓ Windows cross-compilation setup complete!" -ForegroundColor Green
Set-Location ..
