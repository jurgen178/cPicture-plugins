#include "stdafx.h"

#include <vector>
#include <io.h>
#include <regex>
using namespace std;


bool CheckFile(const WCHAR* pFile);
bool scanBoolVar(const char* Text, const CString& SearchTextTemplate, bool def);
std::wregex GetPS1DescriptionRegex();
CString scanPS1Description(char* Text, std::wregex& descriptionRegex);
std::wregex GetPyDescriptionRegex();
CString scanPyDescription(char* Text, std::wregex& descriptionRegex);
CString escapeCmdLineJsonData(CString text);
CString escapeCmdLineJsonData(bool expr);
CString escapeCmdLineJsonData(float value);
CString escapeCmdLineJsonData(__int64 value);
#ifdef DEBUG
CString GetLastErrorStr();
#endif
