<#
.DESCRIPTION
    Example script to print the picture data.
#>

# plugin variables

# console=true displays a console, use this option for scripts with text output.
# Do not remove the # on the following line:
#[console=true]

# noexit=true keeps the console open, set to 'false' to have the console closed when processing is done.
# Do not remove the # on the following line:
#[noexit=false]

param
(
    [Parameter(Mandatory=$true)]
    [string]$picture_data_json
)

# Get the picture data.
$picture_data_set = ConvertFrom-Json -InputObject $picture_data_json

# Number of pictures.
[int]$size = $picture_data_set.length
Write-Host "$size picture(s):" -ForegroundColor White
Write-Host ("-" * 15)
Write-Host

# Print the picture data.
[int]$i = 1
foreach($picture_data in $picture_data_set)
{
    # Example:
    # $picture_data.name   : c:\Bilder\bild1.jpg
    # $picture_data.file   : bild1.jpg
    # $picture_data.dir    : c:\Bilder\
    # $picture_data.width  : 3712
    # $picture_data.height : 5568

    # $picture_data.audio        : false
    # $picture_data.video        : false
    # $picture_data.colorprofile : false
    # $picture_data.gps          : N 47° 37' 0,872498" W 122° 19' 32,021484"
    # $picture_data.aperture     : 5.6   # f/5.6
    # $picture_data.shutterspeed : 1250  # 1/1250s
    # $picture_data.iso          : 100
    # $picture_data.exifdate     : 133553225690000000
    # $picture_data.exifdate_str : 19.03.2024 11:49:29
    # $picture_data.model        : NIKON Z 30 
    # $picture_data.lens         : 16-50mm f/3,5-6,3 VR f=44mm/66mm
    # $picture_data.cdata        :  # Configurable with F9
                # Bildname:       Pike-Place-Market-Kreuzung-360x180.jpg
                # Bilderordner:   C:\Bilder\
                # Bildgröße:      1624x812 Bildpunkte (1,3MP) (2:1)
                # Modell:         [NIKON Z 30]
                # Einstellungen:  1/1250s ISO 100/21°
                # Enthält:                Kommentar, XMP, Farbprofil
                # GPS:            N 47° 37' 0,872498 W 122° 19' 32,021484
                # Dateigröße:     835 KB (855189 Bytes)
                # Datei erstellt am:      Dienstag, 19. März 2024 um 11:49:29 Uhr
                # Datei geändert am:      Dienstag, 19. März 2024 um 11:49:29 Uhr
                # Aufgen. am:     Dienstag, 19. März 2024 um 11:49:29 Uhr

    [int]$MP = $picture_data.width * $picture_data.height / 1000000
    Write-Host ("Image '{0}' ({4} of {5}) with {1}x{2} pixel ({3}MP)" -f $picture_data.file, $picture_data.width, $picture_data.height, $MP, $i, $size)
    Write-Host "  name='$($picture_data.name)', dir='$($picture_data.dir)'`n"
    Write-Host "picture_data:"
    Write-Host $picture_data
    Write-Host ("-" * 70)

    $i++
}


# Use this to pause the console when using the #[console=true] option.
Write-Host "Press any key to continue ..."
[void]$host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
