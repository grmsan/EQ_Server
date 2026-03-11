<#
Installs LuaJIT + Dear ImGui (DX9 + Win32 backends) for eq-core-dll Win32 builds using vcpkg.

Usage:
  powershell -ExecutionPolicy Bypass -File .\extras\eq-core-dll-main\install_lua_imgui_deps.ps1
  powershell -ExecutionPolicy Bypass -File .\extras\eq-core-dll-main\install_lua_imgui_deps.ps1 -Triplet x86-windows
#>
param(
    [string]$Triplet = "x86-windows-static-md",
    [string]$RepoVcpkgDir = "$PSScriptRoot\..\..\vcpkg\vcpkg-tool"
)

$ErrorActionPreference = "Stop"

function Write-Step([string]$message) {
    Write-Host "[lua-imgui-install] $message" -ForegroundColor Cyan
}

function Ensure-GitSafeDirectory([string]$path) {
    try {
        & git config --global --add safe.directory $path 2>$null | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "git config failed"
        }
    } catch {
        Write-Host "[lua-imgui-install] Warning: unable to set git safe.directory for $path" -ForegroundColor Yellow
    }
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$vcpkgDir = Resolve-Path -Path (Split-Path $RepoVcpkgDir -Parent) -ErrorAction SilentlyContinue
if (-not $vcpkgDir) {
    $vcpkgDir = Join-Path $repoRoot "vcpkg"
    if (-not (Test-Path $vcpkgDir)) {
        New-Item -ItemType Directory -Path $vcpkgDir | Out-Null
    }
}

if (-not (Test-Path $RepoVcpkgDir)) {
    Write-Step "Cloning vcpkg into $RepoVcpkgDir"
    git clone https://github.com/microsoft/vcpkg.git $RepoVcpkgDir
} else {
    Write-Step "Using existing vcpkg checkout at $RepoVcpkgDir"
}

Ensure-GitSafeDirectory $RepoVcpkgDir

$bootstrapBat = Join-Path $RepoVcpkgDir "bootstrap-vcpkg.bat"
$vcpkgExe = Join-Path $RepoVcpkgDir "vcpkg.exe"

if (-not (Test-Path $vcpkgExe)) {
    Write-Step "Bootstrapping vcpkg"
    & $bootstrapBat
}

Write-Step "Installing luajit + imgui for triplet: $Triplet"
& $vcpkgExe install `
    "luajit:$Triplet" `
    "imgui[dx9-binding,win32-binding]:$Triplet" `
    --clean-after-build
if ($LASTEXITCODE -ne 0) {
    throw "vcpkg install failed with exit code $LASTEXITCODE"
}

$installedRoot = Join-Path $RepoVcpkgDir "installed\$Triplet"
$includeDir = Join-Path $installedRoot "include"
$libDir = Join-Path $installedRoot "lib"

if (Test-Path $includeDir) { $includeDir = (Resolve-Path $includeDir).Path }
if (Test-Path $libDir) { $libDir = (Resolve-Path $libDir).Path }

Write-Step "Install complete."
Write-Host ""
Write-Host "Use these paths in eq-core-dll project settings:" -ForegroundColor Green
Write-Host "  Include: $includeDir"
Write-Host "  Lib:     $libDir"
Write-Host "  Libs:    lua51.lib;imgui.lib"
Write-Host ""
Write-Host "Expected headers:"
Write-Host "  $includeDir\luajit\lua.hpp"
Write-Host "  $includeDir\imgui.h"
Write-Host "  $includeDir\imgui_impl_dx9.h"
Write-Host "  $includeDir\imgui_impl_win32.h"
