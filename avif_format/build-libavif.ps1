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
$sourceDir = Join-Path $projectDirFull 'external\libavif'
$buildDir = Join-Path $projectDirFull ("external\build\libavif\{0}" -f $Configuration)

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
$git = Find-Executable -Name 'git'

$gitPerlDir = 'C:\Program Files\Git\usr\bin'
if (Test-Path -LiteralPath (Join-Path $gitPerlDir 'perl.exe')) {
    $env:PATH = "$gitPerlDir;$env:PATH"
}

if (-not (Test-Path -LiteralPath (Join-Path $sourceDir 'CMakeLists.txt'))) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $sourceDir) | Out-Null
    & $git clone --branch v1.4.2 --depth 1 https://github.com/AOMediaCodec/libavif.git $sourceDir
}

# Skip cmake configure+build if avif.lib already exists (PreBuildEvent runs on every build).
# Delete external\build\libavif to force a rebuild.
$avifLib = Join-Path $buildDir "$Configuration\avif.lib"
if (-not (Test-Path -LiteralPath $avifLib)) {
    $avifLib = Join-Path $buildDir 'avif.lib'   # flat layout fallback
}
if (Test-Path -LiteralPath $avifLib) {
    Write-Host "avif.lib already built - skipping cmake build."
    exit 0
}

$cacheFile = Join-Path $buildDir 'CMakeCache.txt'
if ((Test-Path -LiteralPath $cacheFile) -and (Select-String -LiteralPath $cacheFile -SimpleMatch '$RuntimeLibrary' -Quiet)) {
    Remove-Item -LiteralPath $buildDir -Recurse -Force
}

& $cmake -S $sourceDir -B $buildDir `
    -DBUILD_SHARED_LIBS=OFF `
    -DAVIF_CODEC_AOM=LOCAL `
    -DAVIF_LIBYUV=LOCAL `
    -DAVIF_LIBSHARPYUV=OFF `
    -DAVIF_ZLIBPNG=OFF `
    -DAVIF_JPEG=OFF `
    -DAVIF_BUILD_APPS=OFF `
    -DAVIF_BUILD_TESTS=OFF `
    -DAVIF_BUILD_EXAMPLES=OFF `
    -DAVIF_ENABLE_WERROR=OFF `
    -DAOM_TARGET_CPU=generic `
    "-DCMAKE_BUILD_TYPE=$Configuration" `
    "-DCMAKE_MSVC_RUNTIME_LIBRARY=$RuntimeLibrary"

& $cmake --build $buildDir --config $Configuration --target avif_static
