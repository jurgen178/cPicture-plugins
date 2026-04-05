### Bilderentschlüsselung

Ein PowerShell-Skript zum Entschlüsseln von Bildern, die mit dem Skript *encrypt.ps1* verschlüsselt wurden. Passwort und Geheimphrasen müssen mit denen der Verschlüsselung übereinstimmen.

**Hinweis:** Bearbeiten Sie die Skriptdatei *decrypt.ps1* mit einem Texteditor und ändern Sie folgende Zeilen:

```text
# needs to match with 'encrypt.ps1':

# change the following two secret phrases
$salt = 'secretSentence'
$init = 'anotherSecretSentence'

# supply a strong password
$password = 'mySecretPassword!'
```
