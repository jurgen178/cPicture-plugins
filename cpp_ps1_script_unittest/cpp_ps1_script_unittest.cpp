#include "pch.h"
#include "..\fp_ps1_script\stdafx.h"
#include "..\fp_ps1_script\plugin.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

extern bool scanVar(char* Text, char* TxtANSI, WCHAR* TxtUnicode, bool def);

namespace cppps1scriptunittest
{
	TEST_CLASS(cppps1scriptunittest)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			char* consoleTxtANSI = "#[console=true]";
			char* noexitTxtANSI = "#[noexit=true]";
			WCHAR* consoleTxtUnicode = L"#[console=true]";
			WCHAR* noexitTxtUnicode = L"#[noexit=true]";

			// Read the first 1024 chars to get the console and noexit flags.
			const int textSize(1024);
			unsigned char Text[textSize] = { 0 };

			bool console = scanVar((char*)Text, consoleTxtANSI, consoleTxtUnicode, true);

			Assert::IsTrue(console);
		}
	};
}
