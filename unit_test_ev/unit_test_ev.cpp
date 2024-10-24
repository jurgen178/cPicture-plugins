#include "pch.h"

// Plugin is a library with explicit exports and cannot be used as a dependent .lib for the test framework.
#include "..\fp_ev\plugin.cpp"

#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;


namespace unittest
{
	TEST_CLASS(unittest)
	{
	public:

		TEST_METHOD(TestIntDataLambda)
		{
			// int data
			vector <unsigned int> matchList = { 1, 2, 3 };

			// toString lambda expression
			auto getText([](auto id) { CString text; text.Format(L"%d", id); return text; });

			const CString matchParameter(JoinString<unsigned int>(matchList.begin(), matchList.end(), getText, L"(", L")", L", "));

			Assert::AreEqual(L"(1, 2, 3)", matchParameter);
		}

		TEST_METHOD(TestStringDataLambda)
		{
			// CString data
			vector <CString> strList = { L"a", L"b", L"c" };

			const CString combinedText(JoinString<CString>(strList.begin(), strList.end(), [](auto text) { return text; }, L"[", L"]", L"+"));

			Assert::AreEqual(L"[a+b+c]", combinedText);
		}
	};
}