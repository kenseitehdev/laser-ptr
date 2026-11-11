param(
    [ValidateSet('arm64', 'x86', 'x64')]
    [string]$Arch = 'x64'
)
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    $targetArch = $env:VSCMD_ARG_TGT_ARCH
    if (-not $targetArch) {
        throw "Open a Visual Studio Native Tools PowerShell configured for $Arch before building."
    }
    if ($targetArch -ne $Arch) {
        throw "This shell targets $targetArch; open a Visual Studio Native Tools shell for $Arch."
    }
    $outputDirectory = Join-Path $PSScriptRoot "build/$Arch"
    New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
    $source = Join-Path $PSScriptRoot 'main.c'
    $resource = Join-Path $outputDirectory 'laser-ptr.res'
    $resourceScript = Join-Path $PSScriptRoot 'app.rc'
    $output = Join-Path $outputDirectory 'laser-ptr.exe'
    & rc.exe /nologo "/fo$resource" $resourceScript
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    & cl.exe /nologo /O2 /W4 /DWIN32_LEAN_AND_MEAN $source $resource "/Fe:$output" /link /SUBSYSTEM:WINDOWS user32.lib shell32.lib
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} finally {
    Pop-Location
}
