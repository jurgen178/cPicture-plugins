#include "stdafx.h"
#include "plugin.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

extern bool scanBoolVar(char* Text, const CString SearchTextTemplate, bool def);


namespace cppps1scripttest
{
	TEST_CLASS(cppps1scripttest)
	{
	public:

//		TEST_METHOD(TestScanBoolVar)
//		{
//#pragma warning (push)
//#pragma warning(disable : 4996)
//
//			const CString consoleSearchTextTemplate("#[console=%s]");
//			const CString noexitSearchTextTemplate("#[noexit=%s]");
//
//			const int textSize(1024);
//			char TextA[textSize] = { 0 };
//			WCHAR TextW[textSize] = { 0 };
//
//			// console=true
//			strcpy(TextA, "abc # [ console=true] def");
//			Assert::IsTrue(scanBoolVar(TextA, consoleSearchTextTemplate, true));
//
//			// console=false
//			strcpy(TextA, "abc #[console = false] def");
//			Assert::IsFalse(scanBoolVar(TextA, consoleSearchTextTemplate, true));
//
//			// console default
//			strcpy(TextA, "abc no variable defined def");
//			Assert::IsTrue(scanBoolVar(TextA, consoleSearchTextTemplate, true));
//
//			// noexit=true
//			wcscpy(TextW, L"!abc #[noexit=true ] def");
//			// Add Unicode byte order mark.
//			TextW[0] = 0xff;
//			TextW[1] = 0xfe;
//			Assert::IsTrue(scanBoolVar((char*)TextW, noexitSearchTextTemplate, false));
//
//			// noexit=false
//			wcscpy(TextW, L"!abc #[noexit=false] def");
//			// Add Unicode byte order mark.
//			TextW[0] = 0xff;
//			TextW[1] = 0xfe;
//			Assert::IsFalse(scanBoolVar((char*)TextW, noexitSearchTextTemplate, false));
//
//			// noexit default
//			wcscpy(TextW, L"!abc no variable defined def");
//			// Add Unicode byte order mark.
//			TextW[0] = 0xff;
//			TextW[1] = 0xfe;
//			Assert::IsFalse(scanBoolVar((char*)TextW, noexitSearchTextTemplate, false));
//#pragma warning (pop)
//		}
	};
}
