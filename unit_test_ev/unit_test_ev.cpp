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

		TEST_METHOD(TestMatchParameterLambda)
		{
#pragma warning (push)
#pragma warning(disable : 4996)

			vector <unsigned int> matchList;
			matchList.push_back(1);
			matchList.push_back(2);
			matchList.push_back(3);

			// toString lambda expression
			auto getText([](unsigned int id) { CString text; text.Format(L"%d", id); return text; });
			
			const CString matchParameter(aggregate<vector<unsigned int>>(matchList.begin(), matchList.end(), getText, L"(", L")", L", "));
			
			Assert::AreEqual(L"(1, 2, 3)", matchParameter);


#pragma warning (pop)
		}
	};
}