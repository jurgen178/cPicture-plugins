<#
.DESCRIPTION
    Conversion script from Raw to Tiff via DNG.

    Requires
    Adobe DNG converter 
      https://helpx.adobe.com/camera-raw/using/adobe-dng-converter.html
    ImageMagick
      https://imagemagick.org/script/download.php#windows
#>

# plugin variables

# console=true (default) displays a console, use this option for scripts with text output.
# Do not remove the # on the following line:
#[console=true]

# noexit=true keeps the console open, set to 'false' (default) to have the console closed when processing is done.
# Only used when #[console=true].
# Do not remove the # on the following line:
#[noexit=false]

param
(
    [Parameter(Mandatory = $true)]
    [string]$picture_data_json
)

# Get the picture data.
$picture_data_set = ConvertFrom-Json -InputObject $picture_data_json

# Print the number of pictures.
[int]$size = $picture_data_set.length
Write-Host "Converting $size Raw picture(s):" -ForegroundColor White
Write-Host ("-" * 30)

<#
Command Line Options
The Adobe DNG Converter supports the following command line options:
-c Output compressed DNG files (default).
-u Output uncompressed DNG files.
-l Output linear DNG files.
-e Embed original raw file inside DNG files.
-p0 Set JPEG preview size to none.
-p1 Set JPEG preview size to medium size (default).
-p2 Set JPEG preview size to full size.
-cr2.4 Set Camera Raw compatibility to 2.4 and later
-cr4.1 Set Camera Raw compatibility to 4.1 and later
-cr4.6 Set Camera Raw compatibility to 4.6 and later
-cr5.4 Set Camera Raw compatibility to 5.4 and later
-dng1.1 Set DNG backward version to 1.1
-dng1.3 Set DNG backward version to 1.3
-d <directory> Output converted files to the specified directory.
Default is the same directory as the input file.
-o <filename> Specify the name of the output DNG file.
Default is the name of the input file with the extension
changed to .dng.
#>

# Convert the pictures.
foreach ($picture_data in $picture_data_set) {

    # DNG
    [string]$dng = Join-Path $picture_data.dir (([io.fileinfo]$picture_data.file).basename + ".dng")
    "Konvertieren von '$($picture_data.file)' nach '$dng'"
    & "C:\Program Files\Adobe\Adobe DNG Converter\Adobe DNG Converter.exe" -c -p2 $picture_data.file | Out-Null # | Out-Null  is to wait until finished

    # TIFF
    [string]$tiff = Join-Path $picture_data.dir (([io.fileinfo]$picture_data.file).basename + ".tiff")
    "Konvertieren von '$dng' nach '$tiff'"
    & "C:\Program Files\ImageMagick-7.1.1-Q16\magick " $dng $tiff | Out-Null

    # Remove-Item $dng
}


# Use this to pause the console when using the #[console=true] option.
# Do not use when #[console=false] as the console is not displayed.
Write-Host "Press any key to continue ..."
[void]$host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")