
# Decrypt the picture
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


# needs to match with 'encrypt.ps1':

# change the following two secret phrases
$salt = "secretSentence"
$init = "anotherSecretSentence"

# supply a strong password
$password = "mySecretPassword!"


function Decrypt-File($encryptedFile, $decryptedFile)
{
    if($decryptedFile -eq $null)
    {
        $decryptedFile = $encryptedFile
    }

	$rijndaelCSP = New-Object System.Security.Cryptography.RijndaelManaged
	$pass = [System.Text.Encoding]::UTF8.GetBytes($password)
	$salt = [System.Text.Encoding]::UTF8.GetBytes($salt)
	 
	$rijndaelCSP.Key = (New-Object Security.Cryptography.PasswordDeriveBytes $pass, $salt, "SHA1", 5).GetBytes(32) #256/8
	$rijndaelCSP.IV = (New-Object Security.Cryptography.SHA1Managed).ComputeHash( [Text.Encoding]::UTF8.GetBytes($init) )[0..15]
	 
	$decryptor = $rijndaelCSP.CreateDecryptor()

    $inputFileStream = New-Object System.IO.FileStream($encryptedFile, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read)
	$decryptStream = New-Object Security.Cryptography.CryptoStream $inputFileStream, $decryptor, "Read"
    
    [int]$dataLen = $inputFileStream.Length
    [byte[]]$inputFileData = New-Object byte[] $dataLen

    [int]$decryptLength = 0
    try
    {
        $decryptLength = $decryptStream.Read($inputFileData, 0, $dataLen)
        $decryptStream.Close()
        $inputFileStream.Close()

        $outputFileStream = New-Object System.IO.FileStream($decryptedFile, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
        $outputFileStream.Write($inputFileData, 0, $decryptLength)
        $outputFileStream.Close()

	    $rijndaelCSP.Clear()
    }
    catch [System.Exception]
    {
    }
    finally
    {
    }
}

#"Decrypt picture '{0}'" -f $name

Decrypt-File $name
