### Bilderverschlüsselung

Ein PowerShell-Skript zum Verschlüsseln Ihrer Bilder mit AES-256-Verschlüsselung. Verschlüsselte Bilder können nur mit dem passenden Skript *decrypt.ps1* und dem richtigen Passwort entschlüsselt werden.

**Hinweis:** Bearbeiten Sie die Skriptdatei *encrypt.ps1* mit einem Texteditor und ändern Sie folgende Zeilen:

```text
# needs to match with 'decrypt.ps1':

# change the following two secret phrases
$salt = 'secretSentence'
$init = 'anotherSecretSentence'

# supply a strong password
$password = 'mySecretPassword!'
```
