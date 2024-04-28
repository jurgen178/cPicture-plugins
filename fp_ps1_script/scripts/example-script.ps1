<#
.DESCRIPTION
    Example script to print the picture data.
#>

# plugin variables
# Do not remove the leading # of the variable #[...]:

# Optional description.
#[desc=Example script to print the picture data.
#This example is using the default cPicture cdata template
#which can be changed in the Settings (F9).]

# console=true (default) displays a console, use this option for scripts with text output.
#[console=true]

# noexit=true keeps the console open, set to 'false' (default) to have the console closed when processing is done. This variable is only used when #[console=true].
#[noexit=false]

param
(
    [Parameter(Mandatory=$true)]
    [string]$picture_data_json
)

# Get the picture data.
$picture_data_set = ConvertFrom-Json -InputObject $picture_data_json

# Print the number of pictures.
[int]$size = $picture_data_set.length
Write-Host "$size picture(s):" -ForegroundColor White
Write-Host ("-" * 15)
Write-Host

# Print the picture data.
[int]$i = 1
foreach ($picture_data in $picture_data_set) {
    # Example:
    # $picture_data.file   : c:\Bilder\bild1.jpg
    # $picture_data.name   : bild1.jpg
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
    # name               : Pike-Place-Market-Kreuzung-360x180.jpg
    # dir                : C:\Bilder\
    # size               : 1624x812 Bildpunkte
    # model              : [NIKON Z 30]
    # settings           : 1/1250s ISO 100/21°
    # contains           : Kommentar, XMP, Farbprofil,
    # gps                : N 47° 37' 0,872498" W 122° 19' 32,021484"
    # file_size          : 835 KB (855189 Bytes)
    # file_create_date   : Dienstag, 19. März 2024 um 11:49:29 Uhr
    # file_modified_date : Dienstag, 19. März 2024 um 11:49:29 Uhr
    # exif_date          : Dienstag, 19. März 2024 um 11:49:29 Uhr

    [int]$MP = $picture_data.width * $picture_data.height / 1000000
    "Image '{0}' ({4} of {5}) with {1}x{2} pixel ({3}MP)" -f $picture_data.file, $picture_data.width, $picture_data.height, $MP, $i, $size
    "  name='$($picture_data.name)', dir='$($picture_data.dir)'`n"
    "picture_data:"

    <# 
        Use ConvertFrom-Json when cdata is a json array to access the data elements.
        Otherwise cdata is arbitrary text and use $picture_data.cdata as a string, for example: Write-Host $picture_data.cdata
    #>

    # The default setting for the data is a JSON array matching the tooltip data in cPicture.
    $cdata = ConvertFrom-Json -InputObject $picture_data.cdata
    $cdata

    # Example usage:

    # Use specific data field.
    Write-Host $cdata.model -ForegroundColor Yellow

    # Enumerate all data fields.
    Write-Host "$($cdata.psobject.properties.Value.Count) elements:"
    foreach ($p in $cdata.psobject.properties) {
        Write-Host "  $($p.Name):$($p.Value)" -ForegroundColor Blue
    }

    "-" * 70

    $i++
}


# Use this to pause the console when using the #[console=true] option.
# Do not use when #[console=false] as the console is not displayed.
Write-Host "Press any key to continue ..."
[void]$host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")