// qrcode.h
// C++ port of github.com/AlexEidt/qr (pure-Go QR code library).

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
    if (ws.IsEmpty()) 
        return {};
    const int len = WideCharToMultiByte(CP_UTF8, 0, ws, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) 
        return {};
    std::string result(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws, -1, &result[0], len, nullptr, nullptr);
    return result;
}

// Calculate the top-left pixel position of a QR code.
// corner: 0=Top-Left, 1=Top-Right, 2=Bottom-Left, 3=Bottom-Right
inline void CalcQRPosition(int corner, int img_width, int img_height,
                            int drawSize, int margin,
                            int& out_x, int& out_y)
{
    switch (corner)
    {
    case 0: // Top-Left
        out_x = margin;
        out_y = margin;
        break;
    case 1: // Top-Right
        out_x = img_width - drawSize - margin;
        out_y = margin;
        break;
    case 2: // Bottom-Left
        out_x = margin;
        out_y = img_height - drawSize - margin;
        break;
    default: // Bottom-Right
        out_x = img_width - drawSize - margin;
        out_y = img_height - drawSize - margin;
        break;
    }
}
