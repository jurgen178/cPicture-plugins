#include "stdafx.h"
#include "plugin.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

extern bool scanBoolVar(CString Text, const CString SearchTextTemplate, bool def);


namespace cppps1scripttest
{
	TEST_CLASS(cppps1scripttest)
	{
	public:

		TEST_METHOD(TestScanBoolVar)
		{
			const CString consoleSearchTextTemplate("#[console=%s]");
			const CString noexitSearchTextTemplate("#[noexit=%s]");

			const int textSize(1024);
			char TextA[textSize] = { 0 };
			WCHAR TextW[textSize] = { 0 };

			// console=true
			strcpy(TextA, "abc # [ console=true] def");
			Assert::IsTrue(scanBoolVar(TextA, consoleSearchTextTemplate, true));

			// console=false
			strcpy(TextA, "abc #[console = false] def");
			Assert::IsFalse(scanBoolVar(TextA, consoleSearchTextTemplate, true));

			// console default
			strcpy(TextA, "abc no variable defined def");
			Assert::IsTrue(scanBoolVar(TextA, consoleSearchTextTemplate, true));

			// noexit=true
			wcscpy(TextW, L"abc #[noexit=true ] def");
			Assert::IsTrue(scanBoolVar(TextW, noexitSearchTextTemplate, false));

			// noexit=false
			wcscpy(TextW, L"abc #[noexit=false] def");
			Assert::IsFalse(scanBoolVar(TextW, noexitSearchTextTemplate, false));

			// noexit default
			wcscpy(TextW, L"abc no variable defined def");
			Assert::IsFalse(scanBoolVar(TextW, noexitSearchTextTemplate, false));
		}
	};
}
