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
mobil: 1300px (width) X 2000px (height).
shirt:  3300px (width) X 5100px (height).
all over shirt 6000px (width) X 6000px (height)
pillow: 3500px (width) X 3500px (height); 
clock 3500px (width) X 3500px 
rug: 1:2 aspect, 10k width max
mug 4600px (width) X 2000px (height).
#>

function crop([int]$z,[int]$n)
{
    if($height -gt $width)
    {
        [int]$w = $width
        [int]$h = $width*$z/$n
        if($h -gt $height)
        {
            $w = $w*$height/$h
            $h = $height 
        }
    }
    else
    {
        [int]$h = $height
        [int]$w = $height*$z/$n
        if($w -gt $width)
        {
            $h = $h*$width/$w
            $w = $width 
        }
    }

    "{0}x{1}+0+0" -f $w,$h
}


cd "C:\Program Files\ImageMagick-6.9.2-Q8"
[string]$out = "C:\Bilder\Sizing\"

if(!(Test-Path $out))
{
    [void](New-Item -Path $out -ItemType Directory)
}

[int]$p = ($i+1) * 100 / $n

"$p%"

Copy-Item -Path $name $out


[int]$k = $width
if($height -lt $width)
{
    $k = $height
}
[string]$crop = "{0}x{0}+0+0" -f $k

"Shirt_Shower"
.\convert.exe $name -crop $crop +repage -resize 6000x6000! -gravity center ($out+$file+"_shirt_shower.jpg")

"Clock"
.\convert.exe $name -crop $crop +repage -resize 3500x3500! -gravity center ($out+$file+"_clock.jpg")

#"Print-Shirt"
#.\convert.exe $name -crop $crop +repage -resize 6000x6000! -gravity center ($out+$file+"_PrintShirt.jpg")

"Leggings"
if($height -gt $width)
{
    $crop = crop 90 75
}
else
{
    $crop = crop 75 90
}
.\convert.exe $name -crop $crop +repage -resize 7500x9000! -gravity center ($out+$file+"_Leggings.jpg")

$crop = crop 3 2
"Rugs"
if($height -gt $width)
{
    $size = "{0}x{1}^" -f 3000,4500
}
else
{
    $size = "{0}x{1}^" -f 4500,3000
}

.\convert.exe $name -crop $crop +repage -gravity center -resize $size ($out+$file+"_Rugs.jpg")

"Blankets"
$crop = crop 6500 5525
$crop2 = crop 7400 3700
if($height -gt $width)
{
    $size = "{0}x{1}!" -f 5525,6500
    $size2 = "{0}x{1}!" -f 5525,7400
}
else
{
    $size = "{0}x{1}!" -f 6500,5525
    $size2 = "{0}x{1}!" -f 7400,3700
}

.\convert.exe $name -crop $crop +repage -gravity center -resize $size ($out+$file+"_Blankets.jpg")

"Handtuch"
.\convert.exe $name -crop $crop2 +repage -gravity center -resize $size2 ($out+$file+"_Handtuch.jpg")


"Mobile"
.\convert.exe $name -resize 1300x2000 -gravity center -extent 1300x2000 ($out+$file+"_mobile.jpg")

"Shirt"
.\convert.exe $name -resize 3300x5100 -gravity center -extent 3300x5100 ($out+$file+"_shirt.jpg")

"Mug"
.\convert.exe $name -resize 4600x2000 -gravity center -extent 4600x2000 ($out+$file+"_mug.jpg")

"TravelMug"
.\convert.exe $name -resize 2697x1518 -gravity center -extent 2697x1518 ($out+$file+"_TravelMug.jpg")

"Laptop"
.\convert.exe $name -resize 4600x3000 -gravity center -extent 4600x3000 ($out+$file+"_laptop.jpg")


#Write-Host "Press any key to continue ..."
#[void]$host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
