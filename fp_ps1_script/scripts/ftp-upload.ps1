<#
.DESCRIPTION
    Example script to upload pictures to a FTP server
#>

# plugin variables
# Do not remove the leading # of the variable #[...]:

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

# set the PASSWORD and the LOGINNAME for the $ftpServer:
[string]$ftpServer = "ftp://PASSWORD:LOGINNAME@yourftpserver.com/"

# set the upload folder for the pictures
# Note: folder will be created if it not exist
$ftpUploadFolder = "test/upload/"


$script:FTPdirs = @()

Function MakeSurePathExist([string]$ftpUrl, [string]$filepath) {
    $subDirs = $filepath -split '/'
    $subDirs = $subDirs[0..($subDirs.Length - 2)]
    [string]$dir = ""

    foreach ($subDir in $subDirs) {
        $dir += $subDir + "/"

        if (!$script:FTPdirs.Contains($dir)) {
            try {
                # folder exist? 
                $request = [System.Net.FtpWebRequest]::Create($ftpUrl + $dir)
                $request.Method = [System.Net.WebRequestMethods+Ftp]::ListDirectory
                $response = [System.Net.FtpWebResponse]$request.GetResponse()
                $response.Close()
            }
            catch [System.Net.WebException] {
                # folder does not exist: create folder
                $request = [System.Net.FtpWebRequest]::Create($ftpUrl + $dir)
                $request.Method = [System.Net.WebRequestMethods+Ftp]::MakeDirectory
                $response = [System.Net.FtpWebResponse]$request.GetResponse()
                $response.Close()

                write-host "Folder '$dir' created"
            }
            catch {
                Write-Error $_.Exception.ToString()
            }

            $script:FTPdirs += , $dir
        }
    }
}
Function Upload-File([string]$ftpUrl, [string]$from, [string]$to) {
    MakeSurePathExist $ftpUrl $to

    $webclient = New-Object System.Net.WebClient
    $uri = New-Object System.Uri($ftpUrl + $to)
    $webclient.UploadFile($uri, $from)
    $webclient = $null
}

# Upload the pictures.
[int]$i = 1
foreach ($picture_data in $picture_data_set) {
    "Upload '$($picture_data.name)' ($i of $size)"
    Upload-File $ftpServer $picture_data.file ($ftpUploadFolder + $picture_data.name)
    $i++
}

# Use this to pause the console when using the #[console=true] option.
# Do not use when #[console=false] as the console is not displayed.
Write-Host "Press any key to continue ..."
[void]$host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")