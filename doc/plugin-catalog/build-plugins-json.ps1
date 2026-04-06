Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$SourceJsonPath = Join-Path $PSScriptRoot 'plugins.source.json'
$OutputJsonPath = Join-Path $PSScriptRoot 'plugins2.json'

function Resolve-RelativePath {
    param(
        [string]$BaseFile,
        [string]$RelativePath
    )

    $baseDir = Split-Path -Path $BaseFile -Parent
    $combined = Join-Path -Path $baseDir -ChildPath $RelativePath
    return [System.IO.Path]::GetFullPath($combined)
}

function Normalize-HtmlAttributeQuotes {
    param(
        [string]$Html
    )

    return [regex]::Replace($Html, '(?is)<[^>]+>', {
        param($tagMatch)
        $tag = [string]$tagMatch.Value
        $tag = [regex]::Replace($tag, '([A-Za-z_:][-A-Za-z0-9_:.]*)="([^"]*)"', {
            param($attributeMatch)
            $name = $attributeMatch.Groups[1].Value
            $value = $attributeMatch.Groups[2].Value.Replace("'", '&#39;')
            return $name + "='" + $value + "'"
        })

        return $tag.Replace(" alt=''", '')
    })
}

function Normalize-MarkdownForHtmlConversion {
    param(
        [string]$Markdown
    )

    $normalized = $Markdown.Replace("`r", '')

    # Rohes <br> direkt vor Markdown-Bildern verhindert, dass ConvertFrom-Markdown die Bilder rendert.
    $normalized = [regex]::Replace($normalized, '(?im)^[ \t]*<br\s*/?>[ \t]*\n(?=[ \t]*!\[[^\]]*\]\([^\r\n]+\))', '')

    return $normalized
}

function Convert-LiteralMarkdownImagesToHtml {
    param(
        [string]$Html
    )

    return [regex]::Replace($Html, '!\[([^\]]*)\]\((https?://[^\s)]+)\)', {
        param($match)
        $altText = [System.Net.WebUtility]::HtmlEncode($match.Groups[1].Value)
        $imageUrl = $match.Groups[2].Value

        return "<img src='$imageUrl' alt='$altText' />"
    })
}

function Convert-MarkdownFileToHtmlFragment {
    param(
        [string]$MarkdownPath
    )

    $markdown = Get-Content -LiteralPath $MarkdownPath -Raw
    $markdown = Normalize-MarkdownForHtmlConversion -Markdown $markdown

    $markdownInfo = ConvertFrom-Markdown -InputObject $markdown
    $html = [string]$markdownInfo.Html

    if ($html -match '(?is)<body[^>]*>(.*?)</body>') {
        $html = $Matches[1]
    }

    $html = [regex]::Replace($html, '(?is)<h([1-6])\b[^>]*>(.*?)</h\1>', {
        param($match)
        $level = $match.Groups[1].Value
        $content = $match.Groups[2].Value
        return '<h' + $level + '>' + $content + '</h' + $level + '>'
    })

    # Codeblöcke werden für das einzeilige info-Feld in <code> mit <br /> umgewandelt.
    $html = [regex]::Replace($html, '(?is)<pre>\s*<code[^>]*>(.*?)</code>\s*</pre>', {
        param($match)
        $code = [string]$match.Groups[1].Value
        $code = $code.Replace("`r", '')
        if ($code.EndsWith("`n")) {
            $code = $code.Substring(0, $code.Length - 1)
        }
        $code = $code.Replace("`n", '<br />')
        return '<code>' + $code + '</code>'
    })

    $html = Convert-LiteralMarkdownImagesToHtml -Html $html
    $html = [regex]::Replace($html, '(?is)<p>\s*(<img\b[^>]*>)\s*</p>', '$1')
    $html = [regex]::Replace($html, '(?is)(?:<br\s*/?>\s*)+(?=<img\b)', '')

    $html = Normalize-HtmlAttributeQuotes -Html $html
    $html = $html.Replace('&quot;', '"')
    $html = $html.Replace("`r", '').Replace("`n", '')

    return $html.Trim()
}

function Convert-PluginEntry {
    param(
        [pscustomobject]$Entry,
        [string]$SourceFile
    )

    if (-not $Entry.PSObject.Properties['infoFileMd']) {
        throw "Entry '$($Entry.name)' is missing 'infoFileMd'."
    }

    $markdownPath = Resolve-RelativePath -BaseFile $SourceFile -RelativePath $Entry.infoFileMd
    if (-not (Test-Path -LiteralPath $markdownPath)) {
        throw "Markdown file not found: $markdownPath"
    }

    $html = Convert-MarkdownFileToHtmlFragment -MarkdownPath $markdownPath

    $output = [ordered]@{}
    foreach ($property in $Entry.PSObject.Properties) {
        if ($property.Name -eq 'infoFileMd') {
            $output['info'] = $html
            continue
        }

        $output[$property.Name] = $property.Value
    }

    return [pscustomobject]$output
}

if (-not (Test-Path -LiteralPath $SourceJsonPath)) {
    throw "Source JSON not found: $SourceJsonPath"
}

$sourceJson = Get-Content -LiteralPath $SourceJsonPath -Raw | ConvertFrom-Json -Depth 100
if (-not $sourceJson.PSObject.Properties['cPicture Plugins']) {
    throw "Source JSON must contain the root property 'cPicture Plugins'."
}

$catalog = $sourceJson.'cPicture Plugins'
$resultRoot = [ordered]@{}
$resultCatalog = [ordered]@{}

foreach ($property in $catalog.PSObject.Properties) {
    $name = $property.Name
    $value = $property.Value

    if ($name -in @('plugins-de', 'plugins-en')) {
        $convertedEntries = @()
        foreach ($entry in $value) {
            $convertedEntries += Convert-PluginEntry -Entry $entry -SourceFile $SourceJsonPath
        }

        $resultCatalog[$name] = $convertedEntries
        continue
    }

    $resultCatalog[$name] = $value
}

$resultRoot['cPicture Plugins'] = [pscustomobject]$resultCatalog

$json = ([pscustomobject]$resultRoot | ConvertTo-Json -Depth 100)
if (-not $json.EndsWith("`r`n")) {
    $json += "`r`n"
}
$utf8Bom = New-Object System.Text.UTF8Encoding($true)
[System.IO.File]::WriteAllText($OutputJsonPath, $json, $utf8Bom)

Write-Host "Generated: $OutputJsonPath"
