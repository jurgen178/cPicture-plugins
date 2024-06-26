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
			wcscpy(TextW, L"abc \n#[noexit=true ] def");
			Assert::IsTrue(scanBoolVar((char*)TextW, noexitSearchTextTemplate, false));

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

		TEST_METHOD(TestScanDescription)
		{
#pragma warning (push)
#pragma warning(disable : 4996)

			const int textSize(1024);
			char TextA[textSize] = { 0 };
			WCHAR TextW[textSize] = { 0 };

			std::wregex descriptionRegex(GetDescriptionRegex());

			//strcpy(TextA, "<#\n    .description\n    This is a test description\n#>");
			strcpy(TextA, "<#\n    .description\n\t    This is a\ntest\tdescription\n#>");
			Assert::AreEqual(L"This is a\ntest\tdescription", scanDescription(TextA, descriptionRegex));

			strcpy(TextA, "<#\r\n    .description\r\n    This is a test description\r\n#>");
			Assert::AreEqual(L"This is a test description", scanDescription(TextA, descriptionRegex));

			strcpy(TextA, "abc no DESCRIPTION defined def");
			Assert::AreEqual(L"", scanDescription(TextA, descriptionRegex));

			strcpy(TextA, "<# .DESCRIPTION This is a test description #>");
			Assert::AreEqual(L"", scanDescription((char*)TextW, descriptionRegex));

			wcscpy(TextW, L"<#\n    .Description\n    This is a test description\n#>");
			Assert::AreEqual(L"This is a test description", scanDescription((char*)TextW, descriptionRegex));

			wcscpy(TextW, L"<#\n    .Description\n    This is a test description\n.NOTES notes#>");
			Assert::AreEqual(L"This is a test description", scanDescription((char*)TextW, descriptionRegex));

			wcscpy(TextW, L"<#\n.DESCRIPTION\n    Example script to print the picture data\n	\n.NOTES\n    This example is using the default cPicture custom data template\n    for the $picture_data.cdata value.\n    This value can be changed in the Settings (F9).\n    The .DESCRIPTION text is used for the menu tooltip.\n#>");
			Assert::AreEqual(L"Example script to print the picture data", scanDescription((char*)TextW, descriptionRegex));

			wcscpy(TextW, L"!<#\n    .DESCRIPTION\n    This is a test description\n.NOTES notes\n    .PARAMETER\n    parameter#>");
			// Add Unicode byte order mark.
			TextW[0] = 0xfeff;
			Assert::AreEqual(L"This is a test description", scanDescription((char*)TextW, descriptionRegex));

			wcscpy(TextW, L"abc no DESCRIPTION defined def");
			Assert::AreEqual(L"", scanDescription((char*)TextW, descriptionRegex));

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