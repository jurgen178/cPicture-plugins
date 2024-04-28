<#
.DESCRIPTION
    Rename pictures using a custom logic.
#>

# plugin variables
# Do not remove the leading # of the variable #[...]:

# Optional description.
#[desc=Rename pictures using a custom logic]

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

# Update the pictures.
[int]$i = 1
foreach ($picture_data in $picture_data_set) {

    # Custom logic to rename the file.
    $picture = Get-Item $picture_data.file
    
    [string]$newFile = "{0:000}-name-2024{1}" -f $i, $picture.Extension
    [string]$newName = Join-Path $picture_data.dir $newFile

    "Rename from '$($picture_data.file)' to '$newName'"
    if (Test-Path $newName -PathType Leaf) {
        "The file '$newName' already exist!"
    }
    else {
        Rename-Item -Path $picture_data.file -NewName $newName
    }

    $i++
}


# Use this to pause the console when using the #[console=true] option.
# Do not use when #[console=false] as the console is not displayed.
Write-Host "Press any key to continue ..."
[void]$host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
