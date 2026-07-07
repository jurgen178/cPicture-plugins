# cPicture-plugins
Plugins for the cPicture App

[cPicture](https://bitfabrik.io/cpicture/) supports function plug-ins to process/display selected pictures.  
The Plug-In is a DLL-File located in the same folder as cPicture and starts with 'cpp_'. For example 'cpp_fp1.dll'.  
Click the function plug-ins button in the menu ribbon to display all installed plug-ins.

![](doc/fp-menu.png)   

<br>Function plugins
---------------------

### Sample 1

Demonstrates the basic function plugin flow with `begin()`, `process_picture()` and `end()`.

![](doc/fp1a.png)   
![](doc/fp1b.png)   
![](doc/fp1c.png)   

### Sample 2

Displays a picture preview dialog for the selected pictures.

![](doc/fp2.png)   

### Sample 3

Shows a simple picture order form.

![](doc/fp3.png)   

### Sample 4

Creates modified copies of the selected pictures by scaling them down and inverting colors.

![](doc/fp4.png)   

### Sample 5

Creates an index print from exactly two selected pictures.

![](doc/fp5a.png)   
![](doc/fp5b.png)   

### Script Plugin

Runs PowerShell, Batch and Python scripts from cPicture. Script files are packaged separately and executed through `cpp_script.dll`.

![](doc/fp_script.png)   
![](doc/fp_script_powershell.png)   
![](doc/fp_script_bat.png)   
![](doc/fp_script_python.png)   

### HDR enfuse

Uses the [enfuse tool](https://wiki.panotools.org/Enfuse) to create an HDR picture from an exposure series.

![](doc/fp_hdr.png)   

### Exposure Difference

Calculates exposure differences (EV) for the selected pictures relative to the first selected picture.

![](doc/fp_ev.png)   

### ASCII Art

Converts a picture into ASCII art.

![](doc/fp_ascii_art.png)   

### Clipboard

Copies the selected picture to the Windows clipboard.

### QR Code

Embeds a configurable QR code into the selected pictures.

![](doc/fp_qrcode.png)   

### OCR - Text Recognition

Recognizes text in selected images using the built-in Windows OCR engine. No API key is required.

### Time Capsule

Builds a shareable story poster from the current selection with route, places and thumbnails.

![](doc/fp_timecapsule.jpg)   

### X-Ray

Creates an analysis board for each selected picture with edge, block-boundary and heatmap views.

![](doc/fp_xray.jpg)   

### Motion Composer

Merges a sequence into a single motion-heavy composite image with colored trails.

![](doc/fp_motion_composer.jpg)   

### Postage

Creates a picture that looks like a simple postage stamp with a perforated border, value text and optional stamp overlay.

![](doc/fp_postage1.png)   
![](doc/fp_postage2.jpg)   

### FilmReel

Creates an animated WebP film reel from multiple selected images with configurable frame delay, loop count and quality.

<br>Format plugins
---------------------

### TIFF and PNG Pictures

Adds support for opening TIFF and PNG pictures.

### PDF Format

Adds support for opening PDF documents. The plugin uses PDFium as a delay-loaded dependency.

![](doc/pdf.png)   

### AVIF Format

Adds support for opening and saving AVIF pictures.

### ICM/ICC Color Profile

Adds support for opening ICC and ICM color profile files.

![](doc/icm.PNG)   

### WebP Format

Adds support for opening and saving WebP pictures, including animated WebP playback.

### GIF Format

Adds support for opening GIF pictures, including animated GIF playback.


<br>Structure and details
---------------------

A function plug-ins will be executed in 3 steps.
 The following example displays a message box for each step.

### Step 1

The function start(...) will be called with a list of all selected pictures.
 In this first sample, all picture names will displayed in the message box:

![](doc/fp-code1.png)   


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

### Linker base addresses

Current linker base address state:  

- Plug-ins use fixed linker base addresses with ASLR disabled.
- The address ranges that matter at runtime are PE image ranges: `[ImageBase, ImageBase + SizeOfImage)`.
- The upload script `cPicture\build\build-upload-plugins.ps1` verifies the Release DLL image ranges before creating ZIP files or uploading anything.
- Format plug-ins are spaced 64 MB apart.
- Function plug-ins are spaced 32 MB apart.

Currently assigned base addresses:  

cpf_tiff_png:        0x180000000  
cpf_pdf:             0x184000000  
cpf_avif:            0x188000000  
cpf_icm:             0x18C000000  
cpf_webp:            0x190000000  
cpf_gif:             0x194000000  
cpp_fp1:             0x200000000  
cpp_fp2:             0x202000000  
cpp_fp3:             0x204000000  
cpp_fp4:             0x206000000  
cpp_fp5:             0x208000000  
cpp_ocr:             0x20A000000  
cpp_script:          0x20C000000  
cpp_fp_hdr:          0x20E000000  
cpp_fp_ev:           0x210000000  
cpp_ascii_art:       0x212000000  
cpp_copy_cb:         0x214000000  
cpp_qrcode:          0x216000000  
cpp_timecapsule:     0x218000000  
cpp_xray:            0x21A000000  
cpp_motion_composer: 0x21C000000  
cpp_postage:         0x21E000000  
cpp_filmreel:        0x220000000  


