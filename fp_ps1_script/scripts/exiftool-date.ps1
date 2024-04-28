<#
.DESCRIPTION
    Update Exif data using the exiftool.
#>

# plugin variables
# Do not remove the leading # of the variable #[...]:

# Optional description.
#[desc=Update Exif data using the exiftool]

# console=true (default) displays a console, use this option for scripts with text output.
#[console=true]

# noexit=true keeps the console open, set to 'false' (default) to have the console closed when processing is done. This variable is only used when #[console=true].
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
Write-Host "$size picture(s):" -ForegroundColor White
Write-Host ("-" * 15)
Write-Host

# Update the pictures.
[int]$i = 1
foreach ($picture_data in $picture_data_set) {
    "Change exif date of '$($picture_data.name)' ($i of $size)"

    # Move the date by 8h
    & ".\exiftool.exe" -charset filename=Latin -AllDates-="0:0:0 8:0:0" $picture_data.file
    
    $i++
}

# Use this to pause the console when using the #[console=true] option.
# Do not use when #[console=false] as the console is not displayed.
Write-Host "Press any key to continue ..."
[void]$host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
