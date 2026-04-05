### Encrypt

A PowerShell script to encrypt your pictures using AES-256 encryption. Encrypted pictures can only be decrypted with the matching script *decrypt.ps1* and the correct password.

**Note:** Open the script file *encrypt.ps1* with a text editor and change these lines:

```text
# needs to match with 'decrypt.ps1':

# change the following two secret phrases
$salt = 'secretSentence'
$init = 'anotherSecretSentence'

# supply a strong password
$password = 'mySecretPassword!'
```
