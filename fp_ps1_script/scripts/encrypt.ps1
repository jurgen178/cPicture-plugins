
# Encrypt the picture
# Note: change the password and the secret phrases


# plugin variables

# console=true displays a console, use this option for scripts with text output
# Do not remove the # on the following line:
#[console=false]

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



# needs to match with 'decrypt.ps1':

# change the following two secret phrases
$salt = "secretSentence"
$init = "anotherSecretSentence"

# supply a strong password
$password = "mySecretPassword!"


function Encrypt-File($fileName, $encryptedFile)
{
    if($encryptedFile -eq $null)
    {
        $encryptedFile = $fileName
    }

    $rijndaelCSP = New-Object System.Security.Cryptography.RijndaelManaged
    $pass = [Text.Encoding]::UTF8.GetBytes($password)
    $salt = [Text.Encoding]::UTF8.GetBytes($salt)
 
    $rijndaelCSP.Key = (New-Object Security.Cryptography.PasswordDeriveBytes $pass, $salt, "SHA1", 5).GetBytes(32) #256/8
    $rijndaelCSP.IV = (New-Object Security.Cryptography.SHA1Managed).ComputeHash( [Text.Encoding]::UTF8.GetBytes($init) )[0..15]
   
	$encryptor = $rijndaelCSP.CreateEncryptor()

    $inputFileStream = New-Object System.IO.FileStream($fileName, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read)
    [int]$dataLen = $inputFileStream.Length
    [byte[]]$inputFileData = New-Object byte[] $dataLen
    [void]$inputFileStream.Read($inputFileData, 0, $dataLen)
    $inputFileStream.Close()

    $outputFileStream = New-Object System.IO.FileStream($encryptedFile, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
	
    $encryptStream = New-Object Security.Cryptography.CryptoStream $outputFileStream, $encryptor, "Write"
    $encryptStream.Write($inputFileData, 0, $dataLen)
	$encryptStream.Close()

    $outputFileStream.Close()
	$rijndaelCSP.Clear()
}

#"Encrypt picture '{0}'" -f $name

Encrypt-File $name
