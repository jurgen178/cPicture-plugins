<#
.DESCRIPTION
    Resize and crop pictures to different sizes using ImageMagick.
#>

# plugin variables
# Do not remove the leading # of the variable #[...]:

# Optional description.
#[desc=Resize and crop pictures to different sizes using ImageMagick]

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

<#
mobil: 1300px (width) X 2000px (height).
shirt:  3300px (width) X 5100px (height).
all over shirt 6000px (width) X 6000px (height)
pillow: 3500px (width) X 3500px (height); 
clock 3500px (width) X 3500px 
rug: 1:2 aspect, 10k width max
mug 4600px (width) X 2000px (height).
#>

function crop([int]$z, [int]$n, $picture_data) {
    if ($picture_data.height -gt $picture_data.width) {
        [int]$w = $picture_data.width
        [int]$h = $picture_data.width * $z / $n
        if ($h -gt $picture_data.height) {
            $w = $w * $picture_data.height / $h
            $h = $picture_data.height 
        }
    }
    else {
        [int]$h = $picture_data.height
        [int]$w = $picture_data.height * $z / $n
        if ($w -gt $picture_data.width) {
            $h = $h * $picture_data.width / $w
            $w = $picture_data.width 
        }
    }

    "{0}x{1}+0+0" -f $w, $h
}


[string]$out = "C:\Bilder\Sizing\"
[string]$ImageMagick = "C:\Program Files\ImageMagick-7.1.1-Q16\magick "

if (!(Test-Path $out)) {
    [void](New-Item -Path $out -ItemType Directory)
}

[int]$n = $picture_data_set.length
[int]$i = 1

# Resize the pictures.
foreach ($picture_data in $picture_data_set) {

    [int]$p = $i * 100 / $n
    "$p%"

    Copy-Item -Path $picture_data.file $out

    [int]$k = $picture_data.width
    if ($picture_data.height -lt $picture_data.width) {
        $k = $picture_data.height
    }

    [string]$name = ([io.fileinfo]$picture_data.file).basename
    [string]$crop = "{0}x{0}+0+0" -f $k

    "Shirt_Shower"
    & $ImageMagick $picture_data.file -crop $crop +repage -resize 6000x6000! -gravity center ($out + $name + "_shirt_shower.jpg")

    "Clock"
    & $ImageMagick $picture_data.file -crop $crop +repage -resize 3500x3500! -gravity center ($out + $name + "_clock.jpg")

    #"Print-Shirt"
    #& $ImageMagick $picture_data.file -crop $crop +repage -resize 6000x6000! -gravity center ($out+$name+"_PrintShirt.jpg")

    "Leggings"
    if ($picture_data.height -gt $picture_data.width) {
        $crop = crop 90 75 $picture_data
    }
    else {
        $crop = crop 75 90 $picture_data
    }
    & $ImageMagick $picture_data.file -crop $crop +repage -resize 7500x9000! -gravity center ($out + $name + "_Leggings.jpg")

    $crop = crop 3 2 $picture_data
    "Rugs"
    if ($picture_data.height -gt $picture_data.width) {
        $size = "{0}x{1}^" -f 3000, 4500
    }
    else {
        $size = "{0}x{1}^" -f 4500, 3000
    }

    & $ImageMagick $picture_data.file -crop $crop +repage -gravity center -resize $size ($out + $name + "_Rugs.jpg")

    "Blankets"
    $crop = crop 6500 5525 $picture_data
    $crop2 = crop 7400 3700 $picture_data
    if ($picture_data.height -gt $picture_data.width) {
        $size = "{0}x{1}!" -f 5525, 6500
        $size2 = "{0}x{1}!" -f 5525, 7400
    }
    else {
        $size = "{0}x{1}!" -f 6500, 5525
        $size2 = "{0}x{1}!" -f 7400, 3700
    }

    & $ImageMagick $picture_data.file -crop $crop +repage -gravity center -resize $size ($out + $name + "_Blankets.jpg")

    "Handtuch"
    & $ImageMagick $picture_data.file -crop $crop2 +repage -gravity center -resize $size2 ($out + $name + "_Handtuch.jpg")


    "Mobile"
    & $ImageMagick $picture_data.file -resize 1300x2000 -gravity center -extent 1300x2000 ($out + $name + "_mobile.jpg")

    "Shirt"
    & $ImageMagick $picture_data.file -resize 3300x5100 -gravity center -extent 3300x5100 ($out + $name + "_shirt.jpg")

    "Mug"
    & $ImageMagick $picture_data.file -resize 4600x2000 -gravity center -extent 4600x2000 ($out + $name + "_mug.jpg")

    "TravelMug"
    & $ImageMagick $picture_data.file -resize 2697x1518 -gravity center -extent 2697x1518 ($out + $name + "_TravelMug.jpg")

    "Laptop"
    & $ImageMagick $picture_data.file -resize 4600x3000 -gravity center -extent 4600x3000 ($out + $name + "_laptop.jpg")
}


# Use this to pause the console when using the #[console=true] option.
# Do not use when #[console=false] as the console is not displayed.
Write-Host "Press any key to continue ..."
[void]$host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
