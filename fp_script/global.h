#include "stdafx.h"

#include <vector>
#include <io.h>
#include <regex>
using namespace std;


bool CheckFile(const WCHAR* pFile);
bool scanBoolVar(const char* Text, const CString& SearchTextTemplate, bool def);
CString scanPS1Description(char* Text);
CString scanPyDescription(char* Text);
CString escapeCmdLineJsonDataPS1Str(CString text);
CString escapeCmdLineJsonData(bool expr);
CString escapeCmdLineJsonData(float value);
CString escapeCmdLineJsonData(__int64 value);
CString escapeCmdLineJsonDataPyStr(CString text);
bool toUTF8(const CString& data, unsigned char*& utf8Buffer, int& utf8Length);
CString toBase64(const unsigned char* bin, const int len);
CString toBase64(const CString& data);

#ifdef DEBUG
CString GetLastErrorStr();
#endif
