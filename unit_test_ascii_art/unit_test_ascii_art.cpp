#include "pch.h"
#include <map>

// Plugin is a library with explicit exports and cannot be used as a dependent .lib for the test framework.
#include "..\fp_ascii_art\FontSelectComboBox.cpp"
#include "..\fp_ascii_art\AsciiArtDlg.cpp"
#include "..\fp_ascii_art\plugin.cpp"

#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;


namespace unittest
{
	TEST_CLASS(unittest)
	{
	public:

		TEST_METHOD(TestMap)
		{
			// Create a map to store all the densities.
			std::map<double, WCHAR> densities;

			densities[0.000000] = L' ';
			densities[0.022569] = L'.';
			densities[0.023872] = L'`';
			densities[0.043837] = L'\'';
			densities[0.045139] = L':';
			densities[0.046875] = L',';
			densities[0.058594] = L'_';
			densities[0.069444] = L';';
			densities[0.074219] = L'^';
			densities[0.080295] = L'~';
			densities[0.087674] = L'"';
			densities[0.090278] = L'!';
			densities[0.106337] = L'+';
			densities[0.109375] = L'<';
			densities[0.116753] = L'>';
			densities[0.117188] = L'=';
			densities[0.118924] = L'\\';
			densities[0.124566] = L'*';
			densities[0.131076] = L'?';
			densities[0.132378] = L'v';
			densities[0.138455] = L'c';
			densities[0.139323] = L'x';
			densities[0.142361] = L'L';
			densities[0.143229] = L'|';
			densities[0.143663] = L'l';
			densities[0.146701] = L'T';
			densities[0.147569] = L'Y';
			densities[0.152778] = L'}';
			densities[0.153212] = L'{';
			densities[0.154514] = L'n';
			densities[0.155382] = L')';
			densities[0.156684] = L'z';
			densities[0.157118] = L'1';
			densities[0.157986] = L'r';
			densities[0.158420] = L'i';
			densities[0.159288] = L'f';
			densities[0.160590] = L']';
			densities[0.163628] = L's';
			densities[0.164497] = L't';
			densities[0.166233] = L'J';
			densities[0.167535] = L'j';
			densities[0.171441] = L'u';
			densities[0.172309] = L'C';
			densities[0.172743] = L'o';
			densities[0.176215] = L'y';
			densities[0.176649] = L'e';
			densities[0.177951] = L'V';
			densities[0.178819] = L'X';
			densities[0.180556] = L'h';
			densities[0.181424] = L'a';
			densities[0.181858] = L'Z';
			densities[0.183594] = L'2';
			densities[0.185330] = L'4';
			densities[0.186632] = L'U';
			densities[0.188802] = L'S';
			densities[0.195312] = L'H';
			densities[0.198351] = L'w';
			densities[0.199219] = L'k';
			densities[0.199653] = L'A';
			densities[0.204427] = L'5';
			densities[0.205729] = L'p';
			densities[0.208333] = L'P';
			densities[0.209201] = L'm';
			densities[0.210938] = L'%';
			densities[0.215278] = L'6';
			densities[0.215712] = L'q';
			densities[0.217448] = L'd';
			densities[0.218316] = L'E';
			densities[0.218750] = L'G';
			densities[0.224392] = L'O';
			densities[0.225694] = L'R';
			densities[0.231771] = L'#';
			densities[0.233507] = L'N';
			densities[0.240451] = L'$';
			densities[0.241753] = L'g';
			densities[0.243924] = L'0';
			densities[0.244792] = L'8';
			densities[0.246094] = L'&';
			densities[0.253906] = L'B';
			densities[0.255208] = L'M';
			densities[0.258681] = L'W';
			densities[0.264323] = L'Q';
			densities[0.279948] = L'@';

			// Create a map to map the normalized grey values 0..255 to the matching char.
			std::map<int, WCHAR> densities_index;

			// Largest density.
			const double largestDensity(densities.rbegin()->first);

			for (int i = 0; i < 256; ++i)
			{
				const double d(i / largestDensity / 255);

				// Find best matching density.
				for (const auto& pair : densities) {
					if (pair.first >= d)
					{
						densities_index[i] = pair.second;
						break;
					}
				}
			}

			Assert::AreEqual(L' ', densities_index[0]);
			Assert::AreEqual(L'@', densities_index[255]);
		}
	};
}