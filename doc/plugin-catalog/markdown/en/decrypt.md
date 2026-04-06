### Decrypt

A PowerShell script to decrypt pictures that were encrypted with the *encrypt.ps1* script. The password and secret phrases must match those used during encryption.

**Note:** Open the script file *decrypt.ps1* with a text editor and change these lines:

```text
# needs to match with 'encrypt.ps1':

# change the following two secret phrases
$salt = 'secretSentence'
$init = 'anotherSecretSentence'

# supply a strong password
$password = 'mySecretPassword!'
```
