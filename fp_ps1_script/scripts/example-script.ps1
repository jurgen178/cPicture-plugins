<#
.DESCRIPTION
    Example script to print the picture data.
#>

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
"$size pictures:"

# Print the picture data.
[int]$i = 1
foreach($picture_data in $picture_data_set)
{
    # Example:
    # $picture_data.name   : c:\Bilder\bild1.jpg
    # $picture_data.file   : bild1.jpg
    # $picture_data.dir    : c:\Bilder\
    # $picture_data.width  : 1200
    # $picture_data.height : 800

    [int]$MP = $picture_data.width * $picture_data.height / 1000000
    "Image '{0}' ({4} of {5}) with {1}x{2} pixel ({3}MP)" -f $picture_data.name, $picture_data.width, $picture_data.height, $MP, $i, $size
    "file='$($picture_data.file)', dir='$($picture_data.dir)'`n"

    $i++
}

# Use this to pause the console when using the #[console=true] option.
Write-Host "Press any key to continue ..."
[void]$host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
