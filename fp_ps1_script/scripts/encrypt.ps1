<#
.DESCRIPTION
    Encrypt the pictures.
    Note: change the password and the secret phrases
#>

# plugin variables

# console=true (default) displays a console, use this option for scripts with text output.
# Do not remove the # on the following line:
#[console=true]

# noexit=true keeps the console open, set to 'false' (default) to have the console closed when processing is done.
# Only used when #[console=true].
# Do not remove the # on the following line:
#[noexit=false]

param
(
    [Parameter(Mandatory = $true)]
    [string]$picture_data_json
)

# Get the picture data.
$picture_data_set = ConvertFrom-Json -InputObject $picture_data_json


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


foreach ($picture_data in $picture_data_set) {
    "Encrypt '$($picture_data.file)'"
    Encrypt-File $picture_data.file
}


# Use this to pause the console when using the #[console=true] option.
# Do not use when #[console=false] as the console is not displayed.
Write-Host "Press any key to continue ..."
[void]$host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
