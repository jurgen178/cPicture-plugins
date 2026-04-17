[CmdletBinding()]
param(
    [string]$CatalogPath = (Join-Path $PSScriptRoot 'plugins.source.json'),
    [switch]$FailOnMismatch
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-RepoRoot {
    return [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
}

function Get-VersionMap {
    param(
        [object[]]$Entries
    )

    $map = @{}
    foreach ($entry in $Entries) {
        if (-not $entry.PSObject.Properties['version']) {
            continue
        }

        $map[[string]$entry.file] = [string]$entry.version
    }

    return $map
}

function Get-PluginSourcePath {
    param(
        [string]$RepoRoot,
        [string]$PluginFile
    )

    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($PluginFile)
    if ($baseName.StartsWith('cpp_')) {
        $pluginName = $baseName.Substring(4)
        $folderName = if ($pluginName -match '^fp[1-5]$' -or $pluginName.StartsWith('fp_')) { $pluginName } else { 'fp_' + $pluginName }

        return Join-Path $RepoRoot ($folderName + '\Plugin.cpp')
    }

    if ($baseName.StartsWith('cpf_')) {
        $formatName = $baseName.Substring(4)
        return Join-Path $RepoRoot ($formatName + '_format\PluginFormat.cpp')
    }

    return $null
}

function Get-SourceVersion {
    param(
        [string]$SourcePath
    )

    if ([string]::IsNullOrWhiteSpace($SourcePath) -or -not (Test-Path -LiteralPath $SourcePath)) {
        return '<missing source>'
    }

    $content = Get-Content -LiteralPath $SourcePath -Raw
    $match = [regex]::Match(
        $content,
        'GetPluginVersion\s*\([^)]*\)\s*\{.*?return\s+(?:_T\(|L")(?<ver>[0-9]+\.[0-9]+)"\)?\s*;',
        [System.Text.RegularExpressions.RegexOptions]::Singleline
    )

    if (-not $match.Success) {
        return '<not found>'
    }

    return $match.Groups['ver'].Value
}

if (-not (Test-Path -LiteralPath $CatalogPath)) {
    throw "Catalog JSON not found: $CatalogPath"
}

$repoRoot = Get-RepoRoot
$catalogJson = Get-Content -LiteralPath $CatalogPath -Raw | ConvertFrom-Json -Depth 100
$catalog = $catalogJson.'cPicture Plugins'

if ($null -eq $catalog) {
    throw "Catalog JSON must contain the root property 'cPicture Plugins'."
}

$deVersions = Get-VersionMap -Entries $catalog.'plugins-de'
$enVersions = Get-VersionMap -Entries $catalog.'plugins-en'

$allFiles = @($deVersions.Keys + $enVersions.Keys | Sort-Object -Unique)
$results = foreach ($pluginFile in $allFiles) {
    $sourcePath = Get-PluginSourcePath -RepoRoot $repoRoot -PluginFile $pluginFile
    $sourceVersion = Get-SourceVersion -SourcePath $sourcePath
    $deVersion = if ($deVersions.ContainsKey($pluginFile)) { $deVersions[$pluginFile] } else { '<missing>' }
    $enVersion = if ($enVersions.ContainsKey($pluginFile)) { $enVersions[$pluginFile] } else { '<missing>' }
    $deEnMatch = ($deVersion -eq $enVersion)
    $catalogSourceMatch = ($deVersion -eq $sourceVersion -and $enVersion -eq $sourceVersion)

    [pscustomobject]@{
        File = $pluginFile
        DE = $deVersion
        EN = $enVersion
        Source = $sourceVersion
        DeEnMatch = $deEnMatch
        CatalogSourceMatch = $catalogSourceMatch
        SourceFile = if ($null -ne $sourcePath) { $sourcePath.Substring($repoRoot.Length).TrimStart('\') } else { '<unknown>' }
    }
}

$deEnMismatches = @($results | Where-Object { -not $_.DeEnMatch })
$catalogSourceMismatches = @($results | Where-Object { -not $_.CatalogSourceMatch })

Write-Host "Catalog: $CatalogPath"
Write-Host "Checked plugins: $($results.Count)"
Write-Host "DE/EN mismatches: $($deEnMismatches.Count)"
Write-Host "Catalog/source mismatches: $($catalogSourceMismatches.Count)"
Write-Host ''

$results |
    Sort-Object File |
    Select-Object File, DE, EN, Source, DeEnMatch, CatalogSourceMatch, SourceFile |
    Format-Table -AutoSize |
    Out-Host

if ($deEnMismatches.Count -gt 0 -or $catalogSourceMismatches.Count -gt 0) {
    Write-Host ''
    Write-Host 'Mismatches:'
    $results |
        Where-Object { -not $_.DeEnMatch -or -not $_.CatalogSourceMatch } |
        Sort-Object File |
        Select-Object File, DE, EN, Source, SourceFile |
        Format-Table -AutoSize |
        Out-Host
}
else {
    Write-Host 'No mismatches found.'
}

if ($FailOnMismatch -and ($deEnMismatches.Count -gt 0 -or $catalogSourceMismatches.Count -gt 0)) {
    exit 1
}