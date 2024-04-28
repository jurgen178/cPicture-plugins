<#
.DESCRIPTION
    Decrypt the pictures.
    Note: change the password and the secret phrases
#>

# plugin variables
# Do not remove the leading # of the variable #[...]:

# Optional description.
#[desc=Decrypt the pictures]

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


# needs to match with 'encrypt.ps1':

# change the following two secret phrases
$salt = "secretSentence"
$init = "anotherSecretSentence"

# supply a strong password
$password = "mySecretPassword!"


function Decrypt-File($encryptedFile, $decryptedFile) {
    if ($decryptedFile -eq $null) {
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
    try {
        $decryptLength = $decryptStream.Read($inputFileData, 0, $dataLen)
        $decryptStream.Close()
        $inputFileStream.Close()

        $outputFileStream = New-Object System.IO.FileStream($decryptedFile, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
        $outputFileStream.Write($inputFileData, 0, $decryptLength)
        $outputFileStream.Close()

        $rijndaelCSP.Clear()
    }
    catch [System.Exception] {
    }
    finally {
    }
}

# Decrypt the pictures.
foreach ($picture_data in $picture_data_set) {
    "Decrypt '$($picture_data.file)'"
    Decrypt-File $picture_data.file
}


# Use this to pause the console when using the #[console=true] option.
# Do not use when #[console=false] as the console is not displayed.
Write-Host "Press any key to continue ..."
[void]$host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
