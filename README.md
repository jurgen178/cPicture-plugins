# cPicture-plugins
Plugins for the cPicture App

cPicture supports function plug-ins to process/display selected pictures.  
The Plug-In is a DLL-File located in the same folder as cPicture and starts with 'cpp_'. For example 'cpp_fp1.dll'.  
Click the function plug-ins button in the menu ribbon to display all installed plug-ins.

![](doc/fp-menu.png)   

<br>Sample 1
--------

![](doc/fp1-1.png)   

![](doc/fp1-2a.png)   

![](doc/fp1-2b.png)   

![](doc/fp1-2c.png)   

![](doc/fp1-3.png)   

<br>Sample 2
--------

![](doc/fp2.png)   

<br>Sample 3
--------

![](doc/fp3.png)   


<br>.bat Script
-----------

The script files need to be in the same folder. You can use max 10 scripts.  

cPicture.bat example file:


    @echo OFF

    echo name  =%1
    echo file  =%2
    echo path  =%3
    echo width =%4
    echo height=%5
    echo sequence number=%6 
    echo number of files=%7


    REM "Press any key to continue ..."
    pause

![](doc/fp-bat.png)   


<br>Powershell script
-----------------
  

cPicture.ps1 example file  


         param (
            [string]$name,
            [string]$file,
            [string]$path,
            [int]$width,
            [int]$height
            [int]$i,
            [int]$n
             )

        [int]$MP = $width * $height / 1000000
        "`nImage '{0}' ({4} of {5}) with {1}x{2} pixel ({3}MP)`n" -f $name, $width, $height, $MP, ($i+1), $n


        Write-Host "Press any key to continue ..."
        [void]$host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")


![](doc/fp-ps1.png)   


<br>Structure and details
---------------------

A function plug-ins will be executed in 3 steps.
 The following example displays a message box for each step.

### Step 1

The function start(...) will be called with a list of all selected pictures.
 In this first sample, all picture names will displayed in the message box:

![](doc/fp-code2.png)   


### Step 2

The function process_picture(...) will be called for each selected picture:

![](doc/fp-code2.png)   


### Step 3
The funktion end() will be called last:

![](doc/fp-code3.png)   

The return value determines which pictures were modified/deleted or added.
cPicture adjusts the display accordingly.


cPicture can supply each picture with additional picture data. This is controlled by the return value of start(...).
 With this you can easily create external display applications (example above) or simply call a script or an external program.
 The execution stops if the return value is set to 'false'.

