# plugin variables

# console=true displays a console, use this option for scripts with text output
# Do not remove the # on the following line:
#[console=true]

# noexit=true keeps the console open, set to 'false' to have the console closed when processing is done
# Do not remove the # on the following line:
#[noexit=false]

param (
    [string]$name,
    [string]$dir,
    [string]$file,
    [int]$width,
    [int]$height,
    [int]$i,
    [int]$n
     )

<#
    -name name
    -file file
    -dir dir
    -width PictureWidth
    -height PictureHeight
    -i sequence number
    -n number of files

    Example:
    -name c:\picture_folder\picture.jpg
    -file picture.jpg
    -dir c:\picture_folder\
    -width 1024
    -height 768
    -i 1
    -n 4
#>

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
changed to “.dng”.
#>

# DNG
$dng = Join-Path $dir (([io.fileinfo]$file).basename + ".dng")
"Konvertieren von '{0}' nach '{1}'" -f $name, $dng
& "C:\Program Files (x86)\Adobe\Adobe DNG Converter.exe" -c -p2 $name | Out-Null # | Out-Null  is to wait until finished

# TIFF
$tiff = Join-Path $dir (([io.fileinfo]$file).basename + ".tiff")
"Konvertieren von '{0}' nach '{1}'" -f $dng, $tiff
& "C:\Users\jurgen\Programme\ImageMagick-6.8.8-Q16\convert" ("dng:"+$dng) $tiff | Out-Null

# 
Remove-Item $dng

<#
Write-Host "Press any key to continue ..."
[void]$host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
#>
