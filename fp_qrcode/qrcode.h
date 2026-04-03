#pragma once
#include <string>
#include <vector>

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
