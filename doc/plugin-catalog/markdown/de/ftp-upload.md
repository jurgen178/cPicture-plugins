### FTP-Dateiübertragung

Ein PowerShell-Skript zum Hochladen der ausgewählten Bilder auf einen FTP-Server. Fehlende Unterordner werden automatisch angelegt.

**Hinweis:** Bearbeiten Sie die Skriptdatei *ftp-upload.ps1* mit einem Texteditor und ändern Sie folgende Zeilen:

```text
# set the PASSWORD and the LOGINNAME for the $ftpServer:

$ftpServer = "ftp://PASSWORD:LOGINNAME@yourftpserver.com/"
$ftpUploadFolder = "test/upload/"

```
