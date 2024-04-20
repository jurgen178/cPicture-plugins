
$max = 255

$h_file = "GetInstance.h"
0..$max | % { "static CFunctionPlugin* __stdcall GetInstance$_() { return new CFunctionPluginScript(Scripts[$_]); };" } | Out-File -FilePath $h_file

"" | Out-File -FilePath $h_file -Append
"static lpfnFunctionGetInstanceProc GetInstanceList[] = " | Out-File -FilePath $h_file -Append
"{" | Out-File -FilePath $h_file -Append
0..$max | % { "GetInstance$_," } | Out-File -FilePath $h_file -Append
"};" | Out-File -FilePath $h_file -Append

