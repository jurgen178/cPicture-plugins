#pragma once
#include <string>
#include <vector>
#include <windows.h>

// Error correction levels: L = ~7 %, M = ~15 %, Q = ~25 %, H = ~30 %
enum class QRErrorLevel { L = 0, M = 1, Q = 2, H = 3 };

// Generate a QR code for the given UTF-8 data string.
// Returns true on success.
// outBitmap : row-major boolean array; outBitmap[y * outSize + x] == true means dark module.
// outSize   : side length in modules (square, includes the 4-module quiet zone on all sides).
bool GenerateQRCode(const std::string& data,
                    QRErrorLevel level,
                    std::vector<bool>& outBitmap,
                    int& outSize);

// Convert a CString (UTF-16) to a UTF-8 std::string.
inline std::string CStringToUTF8(const CString& ws)
{
    if (ws.IsEmpty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, ws, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) return {};
    std::string result(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws, -1, &result[0], len, nullptr, nullptr);
    return result;
}
