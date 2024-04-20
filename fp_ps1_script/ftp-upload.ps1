# plugin variables

# console=true displays a console, use this option for scripts with text output
# Do not remove the # on the following line:
#[console=true]

# noexit=true keeps the console open, set to 'false' to have the console closed when processing is done
# Do not remove the # on the following line:
#[noexit=true]

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
    $name name
    $dir dir
    $file file
    $width PictureWidth
    $height PictureHeight
    $i sequence number
    $n number of files

    Example:
    $name c:\picture_folder\picture.jpg
    $dir c:\picture_folder\
    $file picture.jpg
    $width 1024
    $height 768
    $i 1
    $n 4
#>


# set the PASSWORD and the LOGINNAME for the $ftpServer:
$ftpServer = "ftp://PASSWORD:LOGINNAME@yourftpserver.com/"

# set the upload folder for the pictures
# Note: folder will be created if it not exist
$ftpUploadFolder = "test/upload/"


#

$script:FTPdirs = @()

Function MakeSurePathExist([string]$ftpUrl, [string]$filepath)
{
    $subDirs = $filepath -split '/'
    $subDirs = $subDirs[0..($subDirs.Length-2)]
    [string]$dir = ""

    foreach($subDir in $subDirs)
    {
        $dir += $subDir + "/"

        if(!$script:FTPdirs.Contains($dir))
        {
            try
            {
                # folder exist? 
                $request = [System.Net.FtpWebRequest]::Create($ftpUrl + $dir)
                $request.Method = [System.Net.WebRequestMethods+Ftp]::ListDirectory
                $response = [System.Net.FtpWebResponse]$request.GetResponse()
                $response.Close()
            }
            catch [System.Net.WebException]
            {
                # folder does not exist: create folder
                $request = [System.Net.FtpWebRequest]::Create($ftpUrl + $dir)
                $request.Method = [System.Net.WebRequestMethods+Ftp]::MakeDirectory
                $response = [System.Net.FtpWebResponse]$request.GetResponse()
                $response.Close()

                write-host "Folder '$dir' created"
            }
            catch
            {
                Write-Error $_.Exception.ToString()
            }

            $script:FTPdirs += ,$dir
        }
    }
}


Function Upload-File([string]$ftpUrl, [string]$from, [string]$to)
{
    MakeSurePathExist $ftpUrl $to

    Write-Host "Upload of '$from'"
    $webclient = New-Object System.Net.WebClient
    $uri = New-Object System.Uri($ftpUrl + $to)
    $webclient.UploadFile($uri, $from)
    $webclient = $null
}


Upload-File $ftpServer $name ($ftpUploadFolder + $file)