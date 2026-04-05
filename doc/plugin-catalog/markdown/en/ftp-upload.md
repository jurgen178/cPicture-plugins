### FTP Upload

A PowerShell script to upload the selected pictures to an FTP server. Missing subfolders are created automatically.

**Note:** Open the script file *ftp-upload.ps1* with a text editor and change these lines:

```text
# set the PASSWORD and the LOGINNAME for the $ftpServer:

$ftpServer = "ftp://PASSWORD:LOGINNAME@yourftpserver.com/"
$ftpUploadFolder = "test/upload/"

```
