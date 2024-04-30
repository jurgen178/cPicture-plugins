<#
.DESCRIPTION
    Create the GetInstance.h header file with the instance definitions.

.NOTES
    Both cpp_bat_script and cpp_ps1_script are using the same header file.
#>

cd $PSScriptRoot

# Limit to 256 entries 0..255.
[int]$max = 255

[string]$h_file = "GetInstance.h"
[string]$class = "CFunctionPluginScript"

0..$max | % { "static CFunctionPlugin* __stdcall GetInstance$_() { return new $class(Scripts[$_]); };" } | Out-File -FilePath $h_file -Encoding utf8BOM

"" | Out-File -FilePath $h_file -Append -Encoding utf8BOM
"static lpfnFunctionGetInstanceProc GetInstanceList[] = " | Out-File -FilePath $h_file -Append -Encoding utf8BOM
"{" | Out-File -FilePath $h_file -Append -Encoding utf8BOM
0..$max | % { "GetInstance$_," } | Out-File -FilePath $h_file -Append -Encoding utf8BOM
"};" | Out-File -FilePath $h_file -Append -Encoding utf8BOM

