#include "pch.h"

// Plugin is a library with explicit exports and cannot be used as a dependent .lib for the test framework.
#include "..\fp_ps1_script\plugin.cpp"

#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;


namespace unittest
{
	TEST_CLASS(unittest)
	{
	public:

		TEST_METHOD(TestScanBoolVar)
		{
#pragma warning (push)
#pragma warning(disable : 4996)

			const CString consoleSearchTextTemplate("\n#[console=%s]");
			const CString noexitSearchTextTemplate("\n#[noexit=%s]");

			const int textSize(1024);
			char TextA[textSize] = { 0 };
			WCHAR TextW[textSize] = { 0 };

			// console=true
			strcpy(TextA, "abc \n# [ console=true] def");
			Assert::IsTrue(scanBoolVar(TextA, consoleSearchTextTemplate, true));

			// console=true, var is not at the beginning of the line.
			strcpy(TextA, "abc #[console = false] def");
			Assert::IsTrue(scanBoolVar(TextA, consoleSearchTextTemplate, true));

			// console=false
			strcpy(TextA, "abc \n\r#[console = false] def");
			Assert::IsFalse(scanBoolVar(TextA, consoleSearchTextTemplate, true));

			// console=false, \t added
			strcpy(TextA, "abc \n\r\t#[console = false] def");
			Assert::IsTrue(scanBoolVar(TextA, consoleSearchTextTemplate, true));

			// console default
			strcpy(TextA, "abc no variable defined def");
			Assert::IsTrue(scanBoolVar(TextA, consoleSearchTextTemplate, true));

			// noexit=true
			wcscpy(TextW, L"!abc \n#[noexit=true ] def");
			// Add Unicode byte order mark.
			TextW[0] = 0xfeff;
			Assert::IsTrue(scanBoolVar((char*)TextW, noexitSearchTextTemplate, false));

			// noexit=false
			wcscpy(TextW, L"!abc \n\r#[noexit=false] def");
			// Add Unicode byte order mark.
			TextW[0] = 0xfeff;
			Assert::IsFalse(scanBoolVar((char*)TextW, noexitSearchTextTemplate, false));

			// noexit default
			wcscpy(TextW, L"!abc no variable defined def");
			// Add Unicode byte order mark.
			TextW[0] = 0xfeff;
			Assert::IsFalse(scanBoolVar((char*)TextW, noexitSearchTextTemplate, false));

#pragma warning (pop)
		}

		TEST_METHOD(TestEscapeCmdLineJsonData1)
		{
			// [{"key'1","value"a","key2","value\b\"}]
			CString text(L"[{\"key'1\",\"value\"a\",\"key2\",\"value\\b\\\"}]");

			CString escapedJson = escapeCmdLineJsonData(text);

			// [{\\""key''1\\"",\\""value\\\\\\""a\\"",\\""key2\\"",\\""value\\\\b\\\\\\\\\\""}]
			Assert::AreEqual(L"[{\\\\\"\"key''1\\\\\"\",\\\\\"\"value\\\\\\\\\\\\\"\"a\\\\\"\",\\\\\"\"key2\\\\\"\",\\\\\"\"value\\\\\\\\b\\\\\\\\\\\\\\\\\\\\\"\"}]", escapedJson);
		}

		TEST_METHOD(TestEscapeCmdLineJsonData2)
		{
			// ab"c'1\ 
			CString text(L"ab\"c'\\1\\");

			CString escapedText = escapeCmdLineJsonData(text);

			// ab\\""c''\\1\\\\ 
			Assert::AreEqual(L"ab\\\\\"\"c''\\\\1\\\\\\\\", escapedText);
		}
	};
}