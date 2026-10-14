# Embedded UI Designer: Unified Windows Setup Dispatcher
$arch = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture

if ($arch -eq [System.Runtime.InteropServices.Architecture]::Arm64 -or $arch -eq [System.Runtime.InteropServices.Architecture]::Arm) {
    & "$PSScriptRoot\setup-windows-cross-arm.ps1"
} else {
    & "$PSScriptRoot\setup-windows-x86.ps1"
}
