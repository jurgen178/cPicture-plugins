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

exiftool.exe -charset filename=Latin -AllDates-="0:0:8 0:0:0" $name
