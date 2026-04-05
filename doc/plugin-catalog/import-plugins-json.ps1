Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$InputJsonPath = 'C:\Projects\cPicture\build\plugins2.json'
$SourceJsonPath = Join-Path $PSScriptRoot 'plugins.source.json'
$MarkdownRoot = Join-Path $PSScriptRoot 'markdown'

function Convert-InlineHtmlToMarkdown {
    param(
        [string]$Html
    )

    $text = $Html

    $text = [regex]::Replace($text, '(?is)<a\s+href=["'']([^"'']+)["'']\s*>(.*?)</a>', {
        param($match)
        $url = [System.Net.WebUtility]::HtmlDecode($match.Groups[1].Value)
        $label = Convert-InlineHtmlToMarkdown -Html $match.Groups[2].Value
        return "[$label]($url)"
    })

    $text = [regex]::Replace($text, '(?is)<strong>(.*?)</strong>', {
        param($match)
        $value = Convert-InlineHtmlToMarkdown -Html $match.Groups[1].Value
        return "**$value**"
    })

    $text = [regex]::Replace($text, '(?is)<em>(.*?)</em>', {
        param($match)
        $value = Convert-InlineHtmlToMarkdown -Html $match.Groups[1].Value
        return "*$value*"
    })

    $text = [regex]::Replace($text, '(?i)<br\s*/?>', "`n")
    $text = [regex]::Replace($text, '(?is)</?[^>]+>', '')
    $text = [System.Net.WebUtility]::HtmlDecode($text)

    return $text.Trim()
}

function Convert-HtmlFragmentToMarkdown {
    param(
        [string]$Html
    )

    $markdown = $Html -replace "`r", ''

    $markdown = [regex]::Replace($markdown, '(?is)<pre>\s*<code[^>]*>(.*?)</code>\s*</pre>', {
        param($match)
        $rawCode = $match.Groups[1].Value
        $hasTrailingBreak = [regex]::IsMatch($rawCode, '(?i)<br\s*/?>\s*$')
        $code = $rawCode
        $code = [regex]::Replace($code, '(?i)<br\s*/?>', "`n")
        $code = [System.Net.WebUtility]::HtmlDecode($code).TrimStart("`r", "`n").TrimEnd("`r", "`n")
        if ($hasTrailingBreak) {
            $code += "`n"
        }
        return (@('', '```text', $code, '```', '') -join "`n")
    })

    $markdown = [regex]::Replace($markdown, '(?is)<code[^>]*>(.*?)</code>', {
        param($match)
        $rawCode = $match.Groups[1].Value
        $hasTrailingBreak = [regex]::IsMatch($rawCode, '(?i)<br\s*/?>\s*$')
        $code = $rawCode
        $code = [regex]::Replace($code, '(?i)<br\s*/?>', "`n")
        $code = [System.Net.WebUtility]::HtmlDecode($code).TrimStart("`r", "`n").TrimEnd("`r", "`n")
        if ($hasTrailingBreak) {
            $code += "`n"
        }
        return (@('', '```text', $code, '```', '') -join "`n")
    })

    $markdown = [regex]::Replace($markdown, '(?is)<img\b[^>]*\bsrc=["'']([^"'']+)["''][^>]*>', {
        param($match)
        $url = [System.Net.WebUtility]::HtmlDecode($match.Groups[1].Value)
        return "`n![]($url)`n"
    })

    $markdown = [regex]::Replace($markdown, '(?is)<h[1-6][^>]*>(.*?)</h[1-6]>', {
        param($match)
        $title = Convert-InlineHtmlToMarkdown -Html $match.Groups[1].Value
        return "### $title`n`n"
    })

    $markdown = [regex]::Replace($markdown, '(?is)<p>(.*?)</p>', {
        param($match)
        $paragraph = Convert-InlineHtmlToMarkdown -Html $match.Groups[1].Value
        if ([string]::IsNullOrWhiteSpace($paragraph)) {
            return ''
        }

        return "$paragraph`n`n"
    })

    $markdown = [regex]::Replace($markdown, '(?i)<br\s*/?>', "`n")
    $markdown = [System.Net.WebUtility]::HtmlDecode($markdown)

    $lines = $markdown -split "`n"
    $cleanLines = foreach ($line in $lines) {
        $line.TrimEnd()
    }

    $markdown = ($cleanLines -join "`n").Trim()
    $markdown = [regex]::Replace($markdown, "(`n\s*){3,}", "`n`n")

    return $markdown + "`n"
}

function Get-MarkdownRelativePath {
    param(
        [string]$LanguageCode,
        [string]$PluginFileName
    )

    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($PluginFileName)
    return "markdown/$LanguageCode/$baseName.md"
}

function Convert-Entries {
    param(
        [object[]]$Entries,
        [string]$LanguageCode
    )

    $converted = @()

    foreach ($entry in $Entries) {
        $relativePath = Get-MarkdownRelativePath -LanguageCode $LanguageCode -PluginFileName $entry.file
        $markdownPath = Join-Path $PSScriptRoot $relativePath
        $markdownDir = Split-Path -Path $markdownPath -Parent
        if (-not (Test-Path -LiteralPath $markdownDir)) {
            [System.IO.Directory]::CreateDirectory($markdownDir) | Out-Null
        }

        $markdown = Convert-HtmlFragmentToMarkdown -Html ([string]$entry.info)
        [System.IO.File]::WriteAllText($markdownPath, $markdown, $utf8NoBom)

        $output = [ordered]@{}
        foreach ($property in $entry.PSObject.Properties) {
            if ($property.Name -eq 'info') {
                $output['infoFileMd'] = $relativePath
                continue
            }

            $output[$property.Name] = $property.Value
        }

        $converted += [pscustomobject]$output
    }

    return $converted
}

if (-not (Test-Path -LiteralPath $InputJsonPath)) {
    throw "Input JSON not found: $InputJsonPath"
}

$inputJson = Get-Content -LiteralPath $InputJsonPath -Raw | ConvertFrom-Json -Depth 100
$catalog = $inputJson.'cPicture Plugins'

$resultRoot = [ordered]@{}
$resultCatalog = [ordered]@{}

foreach ($property in $catalog.PSObject.Properties) {
    switch ($property.Name) {
        'plugins-de' {
            $resultCatalog[$property.Name] = @(Convert-Entries -Entries $property.Value -LanguageCode 'de')
        }
        'plugins-en' {
            $resultCatalog[$property.Name] = @(Convert-Entries -Entries $property.Value -LanguageCode 'en')
        }
        default {
            $resultCatalog[$property.Name] = $property.Value
        }
    }
}

$resultRoot['cPicture Plugins'] = [pscustomobject]$resultCatalog

$json = ([pscustomobject]$resultRoot | ConvertTo-Json -Depth 100)
[System.IO.File]::WriteAllText($SourceJsonPath, $json, $utf8NoBom)

Write-Host "Imported: $InputJsonPath"
Write-Host "Written source JSON: $SourceJsonPath"