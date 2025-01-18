#include "stdafx.h"

#include <vector>
#include <io.h>
#include <regex>
using namespace std;


bool CheckFile(const WCHAR* pFile);
bool scanBoolVar(const char* Text, const CString& SearchTextTemplate, bool def);
std::wregex GetDescriptionRegex();
CString scanDescription(char* Text, std::wregex& descriptionRegex);
CString escapeCmdLineJsonData(CString text);
CString escapeCmdLineJsonData(bool expr);
CString escapeCmdLineJsonData(float value);
CString escapeCmdLineJsonData(__int64 value);
