param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectDir,

    [Parameter(Mandatory = $true)]
    [string]$Configuration,

    [Parameter(Mandatory = $true)]
    [string]$RuntimeLibrary
)

$ErrorActionPreference = 'Stop'

function Find-Executable {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,
        [string[]]$Candidates = @()
    )

    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate)) {
            return $candidate
        }
    }

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    throw "$Name was not found. Install it or add it to PATH."
}

$projectDirFull = [System.IO.Path]::GetFullPath($ProjectDir)
$sourceDir = Join-Path $projectDirFull 'external\libwebp'
$buildDir = Join-Path $projectDirFull ("external\build\libwebp\{0}" -f $Configuration)

$programFilesX86 = ${env:ProgramFiles(x86)}
$vswhere = if ($programFilesX86) { Join-Path $programFilesX86 'Microsoft Visual Studio\Installer\vswhere.exe' } else { $null }
$vsCMakeCandidates = @(
    'C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
    'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
    'C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
    'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
    'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
)

if ($vswhere -and (Test-Path -LiteralPath $vswhere)) {
    $installations = & $vswhere -all -products * -requires Microsoft.Component.MSBuild -property installationPath
    foreach ($installation in $installations) {
        $vsCMakeCandidates += Join-Path $installation 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    }
}

$cmake = Find-Executable -Name 'cmake' -Candidates $vsCMakeCandidates
$gitCandidates = @(
    'C:\Program Files\Git\cmd\git.exe',
    'C:\Program Files\Git\bin\git.exe'
)
$git = Find-Executable -Name 'git' -Candidates $gitCandidates

if (-not (Test-Path -LiteralPath (Join-Path $sourceDir 'CMakeLists.txt'))) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $sourceDir) | Out-Null
    & $git clone --branch v1.5.0 --depth 1 https://github.com/webmproject/libwebp.git $sourceDir
}

# Skip cmake configure+build if libwebp.lib already exists (PreBuildEvent runs on every build).
# Delete external\build\libwebp to force a rebuild.
$webpLib = Join-Path $buildDir "$Configuration\libwebp.lib"
if (-not (Test-Path -LiteralPath $webpLib)) {
    $webpLib = Join-Path $buildDir 'libwebp.lib'
}
if (Test-Path -LiteralPath $webpLib) {
    Write-Host "libwebp.lib already built - skipping cmake build."
    exit 0
}

$cacheFile = Join-Path $buildDir 'CMakeCache.txt'
if ((Test-Path -LiteralPath $cacheFile) -and (Select-String -LiteralPath $cacheFile -SimpleMatch '$RuntimeLibrary' -Quiet)) {
    Remove-Item -LiteralPath $buildDir -Recurse -Force
}

& $cmake -S $sourceDir -B $buildDir `
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW `
    -DBUILD_SHARED_LIBS=OFF `
    -DWEBP_BUILD_ANIM_UTILS=OFF `
    -DWEBP_BUILD_CWEBP=OFF `
    -DWEBP_BUILD_DWEBP=OFF `
    -DWEBP_BUILD_GIF2WEBP=OFF `
    -DWEBP_BUILD_IMG2WEBP=OFF `
    -DWEBP_BUILD_VWEBP=OFF `
    -DWEBP_BUILD_WEBPINFO=OFF `
    -DWEBP_BUILD_WEBPMUX=OFF `
    -DWEBP_BUILD_EXTRAS=OFF `
    -DWEBP_BUILD_WEBP_JS=OFF `
    "-DCMAKE_BUILD_TYPE=$Configuration" `
    "-DCMAKE_MSVC_RUNTIME_LIBRARY=$RuntimeLibrary"

& $cmake --build $buildDir --config $Configuration
