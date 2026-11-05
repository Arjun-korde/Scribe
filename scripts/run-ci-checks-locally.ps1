<#
.SYNOPSIS
    Runs the same checks as CI (.github/workflows/ci.yml) on your machine.

.DESCRIPTION
    Full run: for debug and release, configure + build + test; then clang-format and clang-tidy.
    Stops at the first failing step and exits with a non-zero code.

.PARAMETER Quick
    Only the fast checks (clang-format and clang-tidy). The pre-push hook uses this.
    clang-tidy needs build/debug/compile_commands.json, so configure debug once first.
#>
param([switch]$Quick)

$ErrorActionPreference = 'Stop'
Set-Location (git rev-parse --show-toplevel)

# clang-tidy and the build need the compiler environment (see docs/04-development-workflow.md).
function Import-MsvcEnvironment {
    if (Get-Command cl.exe -ErrorAction SilentlyContinue) { return }
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vs = & $vswhere -latest -property installationPath
    $vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
    # vcvars prints a harmless "vswhere.exe is not recognized" on stderr; keep it away from PowerShell.
    cmd /c "`"$vcvars`" >nul 2>&1 && set" | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') {
            [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process')
        }
    }
}

function Invoke-Step([string]$Name, [scriptblock]$Command) {
    Write-Host "==> $Name" -ForegroundColor Cyan
    # Tools write progress to stderr; only the exit code decides success.
    $ErrorActionPreference = 'Continue'
    & $Command
    $ErrorActionPreference = 'Stop'
    if ($LASTEXITCODE -ne 0) { throw "$Name failed (exit code $LASTEXITCODE)" }
}

try {
    Import-MsvcEnvironment

    $qtRoot = if ($env:QT_ROOT_DIR) { $env:QT_ROOT_DIR } else { 'C:\Qt\6.8.3\msvc2022_64' }
    $env:CMAKE_PREFIX_PATH = $qtRoot
    $env:PATH = "$qtRoot\bin;$env:PATH"

    if (-not $Quick) {
        foreach ($preset in 'debug', 'release') {
            Invoke-Step "Configure ($preset)" { cmake --preset $preset }
            Invoke-Step "Build ($preset)" { cmake --build --preset $preset }
            Invoke-Step "Test ($preset)" { ctest --preset $preset }
        }
    }

    $format = git ls-files 'src/*.cpp' 'src/*.h' 'tests/*.cpp' 'tests/*.h'
    Invoke-Step 'clang-format' { clang-format --dry-run -Werror $format }

    $tidy = git ls-files 'src/*.cpp' 'tests/*.cpp'
    Invoke-Step 'clang-tidy' { clang-tidy -p build/debug --quiet $tidy }

    Write-Host 'All checks passed.' -ForegroundColor Green
}
catch {
    Write-Host "FAILED: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
