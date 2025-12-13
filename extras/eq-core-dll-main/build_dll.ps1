<#
Build script for eq-core-dll-main DLLs.

Usage examples:
    # Default uses eq-core-dll-visualstudio2022.sln, Release, Win32
    ./build_dll.ps1

    # Custom config and platform
    ./build_dll.ps1 -Configuration Debug -Platform x86

    # Custom MSBuild path
    ./build_dll.ps1 -MSBuildPath "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"

#>
param(
    [string]$Solution = "$PSScriptRoot\eq-core-dll-visualstudio2022.sln",
    [string]$Configuration = "Release",
    [string]$Platform = "Win32",
    [string]$PlatformToolset = "",
    [string]$MSBuildPath = $env:MSBUILD_PATH,
    [switch]$Verbose
)

function Write-ErrorAndExit {
    Write-Error $args[0]
    exit 1
}

# Resolve MSBuild path
if (-not $MSBuildPath) {
    # default Visual Studio 2022 Community location
    $candidates = @(
        'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe',
        'C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe',
        'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe'
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { $MSBuildPath = $c; break }
    }
    if (-not $MSBuildPath) {
        # Try to find msbuild in PATH
        $ms = Get-Command msbuild -ErrorAction SilentlyContinue
        if ($ms) { $MSBuildPath = $ms.Path }
    }
}
    # Check for vswhere to locate Visual Studio if MSBuild still not found
    if (-not $MSBuildPath) {
        $vswhere = "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe"
        if (Test-Path $vswhere) {
            try {
                $installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
                if ($installPath) {
                    $candidate = Join-Path $installPath 'MSBuild\Current\Bin\MSBuild.exe'
                    if (Test-Path $candidate) { $MSBuildPath = $candidate }
                }
            } catch {}
        }
    }

if (-not (Test-Path $MSBuildPath)) {
    Write-Host "Could not find MSBuild.exe. Please specify -MSBuildPath or install Visual Studio and ensure MSBuild is available." -ForegroundColor Yellow
    Write-Host "Candidates tried:" -ForegroundColor DarkGray
    $candidates | ForEach-Object { Write-Host "  $_" }
    exit 1
}

if (-not (Test-Path $Solution)) {
    Write-Host "Solution file not found at: $Solution" -ForegroundColor Red
    exit 1
}

Write-Host "Building solution: $Solution" -ForegroundColor Cyan
Write-Host "MSBuild path: $MSBuildPath" -ForegroundColor Gray
Write-Host "Configuration: $Configuration, Platform: $Platform" -ForegroundColor Gray
if ($PlatformToolset) { Write-Host "PlatformToolset: $PlatformToolset" -ForegroundColor Gray }

$msbuildArgs = @(
    $Solution,
    "/p:Configuration=$Configuration",
    "/p:Platform=$Platform",
    "/m"
)
if ($PlatformToolset) { $msbuildArgs += "/p:PlatformToolset=$PlatformToolset" }

# Write MSBuild args for debug when requested
if ($Verbose) { Write-Host "MSBuild Args: $($msbuildArgs -join ' ')" -ForegroundColor DarkGray }

# Capture build output to log file
$buildLog = "$PSScriptRoot\msbuild_output.log"
if (Test-Path $buildLog) { Remove-Item $buildLog -Force }

# Invoke msbuild directly so we can capture output
$invokeArgs = $msbuildArgs | ForEach-Object { $_ }
$exitCode = 0
try {
    & $MSBuildPath @invokeArgs 2>&1 | Tee-Object -FilePath $buildLog
    $exitCode = $LASTEXITCODE
} catch {
    Write-Host "MSBuild failed to start: $_" -ForegroundColor Red
    exit 1
}
if ($exitCode -ne 0) {
    Write-Host "Build failed with exit code $exitCode. See $buildLog for details." -ForegroundColor Red
    # Tail last 100 lines of log for convenience
    if (Test-Path $buildLog) { Get-Content $buildLog -Tail 100 }
    exit $exitCode
}

Write-Host "Build completed successfully." -ForegroundColor Green

Write-Host "Build completed successfully." -ForegroundColor Green

# Try to find generated DLLs and copy to ./bin
$targetDir = "$PSScriptRoot\bin"
if (-not (Test-Path $targetDir)) { New-Item -ItemType Directory -Path $targetDir | Out-Null }
#$genDlls = Get-ChildItem -Recurse -Filter "*.dll" -Path $PSScriptRoot\src | Where-Object { $_.FullName -match "\\$Configuration\\" }
# Try to find dlls in output folders under src and common common output directories
$searchPaths = @(Join-Path $PSScriptRoot "src"),
               (Join-Path $PSScriptRoot "src\Release"),
               (Join-Path $PSScriptRoot "src\$Configuration"),
               (Join-Path $PSScriptRoot "src\x86\$Configuration"),
               (Join-Path $PSScriptRoot "src\x64\$Configuration"),
               (Join-Path $PSScriptRoot "Release"),
               (Join-Path $PSScriptRoot "bin"),
               (Join-Path $PSScriptRoot "bin\$Configuration")

$genDlls = @()
foreach ($sp in $searchPaths) {
    if (Test-Path $sp) {
        $files = Get-ChildItem -Recurse -Filter "*.dll" -Path $sp -ErrorAction SilentlyContinue
        if ($files) { $genDlls += $files }
    }
}

if ($genDlls -and $genDlls.Count -gt 0) {
    foreach ($dll in $genDlls) {
        $destPath = Join-Path $targetDir $dll.Name
        if ($dll.FullName -ieq $destPath) { continue }

        Copy-Item -Path $dll.FullName -Destination $targetDir -Force
        Write-Host "Copied $($dll.Name) to $targetDir" -ForegroundColor Green
    }
} else {
    Write-Host "No built DLLs found in src/ under $Configuration. Check the project output paths." -ForegroundColor Yellow
}

Write-Host "Done." -ForegroundColor Magenta
