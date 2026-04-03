// qrcode.cpp
// C++ port of github.com/AlexEidt/qr (pure-Go QR code library).

#include "stdafx.h"
#include "qrcode.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>

namespace {

// ====================================================================
// Constants  (ported from constants.go)
// ====================================================================

// Alignment pattern center coordinates for each version (version-1 indexed).
// Coord list for generating all (non-overlapping-with-finder) 5x5 alignment patterns.
struct AlignList { int pos[7]; int n; };
static const AlignList kAlignPos[40] = {
    {{},                   0}, // v1
    {{6,18},               2}, // v2
    {{6,22},               2}, // v3
    {{6,26},               2}, // v4
    {{6,30},               2}, // v5
    {{6,34},               2}, // v6
    {{6,22,38},            3}, // v7
    {{6,24,42},            3}, // v8
    {{6,26,46},            3}, // v9
    {{6,28,50},            3}, // v10
    {{6,30,54},            3}, // v11
    {{6,32,58},            3}, // v12
    {{6,34,62},            3}, // v13
    {{6,26,46,66},         4}, // v14
    {{6,26,48,70},         4}, // v15
    {{6,26,50,74},         4}, // v16
    {{6,30,54,78},         4}, // v17
    {{6,30,56,82},         4}, // v18
    {{6,30,58,86},         4}, // v19
    {{6,34,62,90},         4}, // v20
    {{6,28,50,72,94},      5}, // v21
    {{6,26,50,74,98},      5}, // v22
    {{6,30,54,78,102},     5}, // v23
    {{6,28,54,80,106},     5}, // v24
    {{6,32,58,84,110},     5}, // v25
    {{6,30,58,86,114},     5}, // v26
    {{6,34,62,90,118},     5}, // v27
    {{6,26,50,74,98,122},  6}, // v28
    {{6,30,54,78,102,126}, 6}, // v29
    {{6,26,52,78,104,130}, 6}, // v30
    {{6,30,56,82,108,134}, 6}, // v31
    {{6,34,60,86,112,138}, 6}, // v32
    {{6,30,58,86,114,142}, 6}, // v33
    {{6,34,62,90,118,146}, 6}, // v34
    {{6,30,54,78,102,126,150}, 7}, // v35
    {{6,24,50,76,102,128,154}, 7}, // v36
    {{6,28,54,80,106,132,158}, 7}, // v37
    {{6,32,58,84,110,136,162}, 7}, // v38
    {{6,26,54,82,110,138,166}, 7}, // v39
    {{6,30,58,86,114,142,170}, 7}, // v40
};

// Version information bits (0 for versions < 7).
static const int kVersionBits[41] = {
    0,0,0,0,0,0,0,
    0b111110010010100,
    0b1000010110111100,
    0b1001101010011001,
    0b1010010011010011,
    0b1011101111110110,
    0b1100011101100010,
    0b1101100001000111,
    0b1110011000001101,
    0b1111100100101000,
    0b10000101101111000,
    0b10001010001011101,
    0b10010101000010111,
    0b10011010100110010,
    0b10100100110100110,
    0b10101011010000011,
    0b10110100011001001,
    0b10111011111101100,
    0b11000111011000100,
    0b11001000111100001,
    0b11010111110101011,
    0b11011000010001110,
    0b11100110000011010,
    0b11101001100111111,
    0b11110110101110101,
    0b11111001001010000,
    0b100000100111010101,
    0b100001011011110000,
    0b100010100010111010,
    0b100011011110011111,
    0b100100101100001011,
    0b100101010000101110,
    0b100110101001100100,
    0b100111010101000001,
    0b101000110001101001,
};

// Format bits indexed by mask | (errMLHQ << 3).
// errMLHQ: M=0, L=1, H=2, Q=3
static const int kFormatBits[32] = {
    0b101010000010010,
    0b101000100100101,
    0b101111001111100,
    0b101101101001011,
    0b100010111111001,
    0b100000011001110,
    0b100111110010111,
    0b100101010100000,
    0b111011111000100,
    0b111001011110011,
    0b111110110101010,
    0b111100010011101,
    0b110011000101111,
    0b110001100011000,
    0b110110001000001,
    0b110100101110110,
    0b1011010001001,
    0b1001110111110,
    0b1110011100111,
    0b1100111010000,
    0b11101100010,
    0b1001010101,
    0b110100001100,
    0b100000111011,
    0b11010101011111,
    0b11000001101000,
    0b11111100110001,
    0b11101000000110,
    0b10010010110100,
    0b10000110000011,
    0b10111011011010,
    0b10101111101101,
};

// Data capacity in bits. Index = (version-1)*4 + errorLevel (L=0,M=1,Q=2,H=3).
static const int kCapacity[160] = {
    152,128,104,72,       // 1
    272,224,176,128,      // 2
    440,352,272,208,      // 3
    640,512,384,288,      // 4
    864,688,496,368,      // 5
    1088,864,608,480,     // 6
    1248,992,704,528,     // 7
    1552,1232,880,688,    // 8
    1856,1456,1056,800,   // 9
    2192,1728,1232,976,   // 10
    2592,2032,1440,1120,  // 11
    2960,2320,1648,1264,  // 12
    3424,2672,1952,1440,  // 13
    3688,2920,2088,1576,  // 14
    4184,3320,2360,1784,  // 15
    4712,3624,2600,2024,  // 16
    5176,4056,2936,2264,  // 17
    5768,4504,3176,2504,  // 18
    6360,5016,3560,2728,  // 19
    6888,5352,3880,3080,  // 20
    7456,5712,4096,3248,  // 21
    8048,6256,4544,3536,  // 22
    8752,6880,4912,3712,  // 23
    9392,7312,5312,4112,  // 24
    10208,8000,5744,4304, // 25
    10960,8496,6032,4768, // 26
    11744,9024,6464,5024, // 27
    12248,9544,6968,5288, // 28
    13048,10136,7288,5608,// 29
    13880,10984,7880,5960,// 30
    14744,11640,8264,6344,// 31
    15640,12328,8920,6760,// 32
    16568,13048,9368,7208,// 33
    17528,13800,9848,7688,// 34
    18448,14496,10288,7888,// 35
    19472,15312,10832,8432,// 36
    20528,15936,11408,8768,// 37
    21616,16816,12016,9136,// 38
    22496,17728,12656,9776,// 39
    23648,18672,13328,10208,// 40
};

// Block structure. Index = (version-1)*4 + errorLevel.
// Fields: n1, totalCW1, dataCW1, n2, totalCW2, dataCW2.
// n2=0 means single group.
struct BlockEntry { int n1,t1,d1, n2,t2,d2; };
static const BlockEntry kBlocks[160] = {
    {1,26,19,0,0,0},{1,26,16,0,0,0},{1,26,13,0,0,0},{1,26,9,0,0,0},     // 1
    {1,44,34,0,0,0},{1,44,28,0,0,0},{1,44,22,0,0,0},{1,44,16,0,0,0},     // 2
    {1,70,55,0,0,0},{1,70,44,0,0,0},{2,35,17,0,0,0},{2,35,13,0,0,0},     // 3
    {1,100,80,0,0,0},{2,50,32,0,0,0},{2,50,24,0,0,0},{4,25,9,0,0,0},     // 4
    {1,134,108,0,0,0},{2,67,43,0,0,0},{2,33,15,2,34,16},{2,33,11,2,34,12}, // 5
    {2,86,68,0,0,0},{4,43,27,0,0,0},{4,43,19,0,0,0},{4,43,15,0,0,0},     // 6
    {2,98,78,0,0,0},{4,49,31,0,0,0},{2,32,14,4,33,15},{4,39,13,1,40,14}, // 7
    {2,121,97,0,0,0},{2,60,38,2,61,39},{4,40,18,2,41,19},{4,40,14,2,41,15}, // 8
    {2,146,116,0,0,0},{3,58,36,2,59,37},{4,36,16,4,37,17},{4,36,12,4,37,13}, // 9
    {2,86,68,2,87,69},{4,69,43,1,70,44},{6,43,19,2,44,20},{6,43,15,2,44,16}, // 10
    {4,101,81,0,0,0},{1,80,50,4,81,51},{4,50,22,4,51,23},{3,36,12,8,37,13}, // 11
    {2,116,92,2,117,93},{6,58,36,2,59,37},{4,46,20,6,47,21},{7,42,14,4,43,15}, // 12
    {4,133,107,0,0,0},{8,59,37,1,60,38},{8,44,20,4,45,21},{12,33,11,4,34,12}, // 13
    {3,145,115,1,146,116},{4,64,40,5,65,41},{11,36,16,5,37,17},{11,36,12,5,37,13}, // 14
    {5,109,87,1,110,88},{5,65,41,5,66,42},{5,54,24,7,55,25},{11,36,12,7,37,13}, // 15
    {5,122,98,1,123,99},{7,73,45,3,74,46},{15,43,19,2,44,20},{3,45,15,13,46,16}, // 16
    {1,135,107,5,136,108},{10,74,46,1,75,47},{1,50,22,15,51,23},{2,42,14,17,43,15}, // 17
    {5,150,120,1,151,121},{9,69,43,4,70,44},{17,50,22,1,51,23},{2,42,14,19,43,15}, // 18
    {3,141,113,4,142,114},{3,70,44,11,71,45},{17,47,21,4,48,22},{9,39,13,16,40,14}, // 19
    {3,135,107,5,136,108},{3,67,41,13,68,42},{15,54,24,5,55,25},{15,43,15,10,44,16}, // 20
    {4,144,116,4,145,117},{17,68,42,0,0,0},{17,50,22,6,51,23},{19,46,16,6,47,17}, // 21
    {2,139,111,7,140,112},{17,74,46,0,0,0},{7,54,24,16,55,25},{34,37,13,0,0,0}, // 22
    {4,151,121,5,152,122},{4,75,47,14,76,48},{11,54,24,14,55,25},{16,45,15,14,46,16}, // 23
    {6,147,117,4,148,118},{6,73,45,14,74,46},{11,54,24,16,55,25},{30,46,16,2,47,17}, // 24
    {8,132,106,4,133,107},{8,75,47,13,76,48},{7,54,24,22,55,25},{22,45,15,13,46,16}, // 25
    {10,142,114,2,143,115},{19,74,46,4,75,47},{28,50,22,6,51,23},{33,46,16,4,47,17}, // 26
    {8,152,122,4,153,123},{22,73,45,3,74,46},{8,53,23,26,54,24},{12,45,15,28,46,16}, // 27
    {3,147,117,10,148,118},{3,73,45,23,74,46},{4,54,24,31,55,25},{11,45,15,31,46,16}, // 28
    {7,146,116,7,147,117},{21,73,45,7,74,46},{1,53,23,37,54,24},{19,45,15,26,46,16}, // 29
    {5,145,115,10,146,116},{19,75,47,10,76,48},{15,54,24,25,55,25},{23,45,15,25,46,16}, // 30
    {13,145,115,3,146,116},{2,74,46,29,75,47},{42,54,24,1,55,25},{23,45,15,28,46,16}, // 31
    {17,145,115,0,0,0},{10,74,46,23,75,47},{10,54,24,35,55,25},{19,45,15,35,46,16}, // 32
    {17,145,115,1,146,116},{14,74,46,21,75,47},{29,54,24,19,55,25},{11,45,15,46,46,16}, // 33
    {13,145,115,6,146,116},{14,74,46,23,75,47},{44,54,24,7,55,25},{59,46,16,1,47,17}, // 34
    {12,151,121,7,152,122},{12,75,47,26,76,48},{39,54,24,14,55,25},{22,45,15,41,46,16}, // 35
    {6,151,121,14,152,122},{6,75,47,34,76,48},{46,54,24,10,55,25},{2,45,15,64,46,16}, // 36
    {17,152,122,4,153,123},{29,74,46,14,75,47},{49,54,24,10,55,25},{24,45,15,46,46,16}, // 37
    {4,152,122,18,153,123},{13,74,46,32,75,47},{48,54,24,14,55,25},{42,45,15,32,46,16}, // 38
    {20,147,117,4,148,118},{40,75,47,7,76,48},{43,54,24,22,55,25},{10,45,15,67,46,16}, // 39
    {19,148,118,6,149,119},{18,75,47,31,76,48},{34,54,24,34,55,25},{20,45,15,61,46,16}, // 40
};

// Polynomial coefficients for Reed-Solomon.
// Key = number of error correction codewords per block.
struct PolyEntry { int ecw; const int* coeff; int len; };
static const int kPoly7[]  = {87,229,146,149,238,102,21};
static const int kPoly10[] = {251,67,46,61,118,70,64,94,32,45};
static const int kPoly13[] = {74,152,176,100,86,100,106,104,130,218,206,140,78};
static const int kPoly15[] = {8,183,61,91,202,37,51,58,58,237,140,124,5,99,105};
static const int kPoly16[] = {120,104,107,109,102,161,76,3,91,191,147,169,182,194,225,120};
static const int kPoly17[] = {43,139,206,78,43,239,123,206,214,147,24,99,150,39,243,163,136};
static const int kPoly18[] = {215,234,158,94,184,97,118,170,79,187,152,148,252,179,5,98,96,153};
static const int kPoly20[] = {17,60,79,50,61,163,26,187,202,180,221,225,83,239,156,164,212,212,188,190};
static const int kPoly22[] = {210,171,247,242,93,230,14,109,221,53,200,74,8,172,98,80,219,134,160,105,165,231};
static const int kPoly24[] = {229,121,135,48,211,117,251,126,159,180,169,152,192,226,228,218,111,0,117,232,87,96,227,21};
static const int kPoly26[] = {173,125,158,2,103,182,118,17,145,201,111,28,165,53,161,21,245,142,13,102,48,227,153,145,218,70};
static const int kPoly28[] = {168,223,200,104,224,234,108,180,110,190,195,147,205,27,232,201,21,43,245,87,42,195,212,119,242,37,9,123};
static const int kPoly30[] = {41,173,145,152,216,31,179,182,50,48,110,86,239,96,222,125,42,173,226,193,224,130,156,37,251,216,238,40,192,180};

static const PolyEntry kPolys[] = {
    {7,kPoly7,7},{10,kPoly10,10},{13,kPoly13,13},{15,kPoly15,15},
    {16,kPoly16,16},{17,kPoly17,17},{18,kPoly18,18},{20,kPoly20,20},
    {22,kPoly22,22},{24,kPoly24,24},{26,kPoly26,26},{28,kPoly28,28},
    {30,kPoly30,30},
};
static const int kNumPolys = (int)(sizeof(kPolys)/sizeof(kPolys[0]));

static const PolyEntry* findPoly(int ecw) {
    for (int i = 0; i < kNumPolys; i++)
        if (kPolys[i].ecw == ecw)
            return &kPolys[i];
    return nullptr;
}

// GF(256) exp and log tables.
static const int kExp[256] = {
    1,2,4,8,16,32,64,128,29,58,116,232,205,135,19,38,76,152,
    45,90,180,117,234,201,143,3,6,12,24,48,96,192,157,39,78,
    156,37,74,148,53,106,212,181,119,238,193,159,35,70,140,5,
    10,20,40,80,160,93,186,105,210,185,111,222,161,95,190,97,
    194,153,47,94,188,101,202,137,15,30,60,120,240,253,231,211,
    187,107,214,177,127,254,225,223,163,91,182,113,226,217,175,
    67,134,17,34,68,136,13,26,52,104,208,189,103,206,129,31,62,
    124,248,237,199,147,59,118,236,197,151,51,102,204,133,23,46,
    92,184,109,218,169,79,158,33,66,132,21,42,84,168,77,154,41,
    82,164,85,170,73,146,57,114,228,213,183,115,230,209,191,99,
    198,145,63,126,252,229,215,179,123,246,241,255,227,219,171,75,
    150,49,98,196,149,55,110,220,165,87,174,65,130,25,50,100,200,
    141,7,14,28,56,112,224,221,167,83,166,81,162,89,178,121,242,
    249,239,195,155,43,86,172,69,138,9,18,36,72,144,61,122,244,
    245,247,243,251,235,203,139,11,22,44,88,176,125,250,233,207,
    131,27,54,108,216,173,71,142,1,
};
static const int kLog[256] = {
    0,0,1,25,2,50,26,198,3,223,51,238,27,104,199,75,4,100,224,
    14,52,141,239,129,28,193,105,248,200,8,76,113,5,138,101,47,225,
    36,15,33,53,147,142,218,240,18,130,69,29,181,194,125,106,39,249,
    185,201,154,9,120,77,228,114,166,6,191,139,98,102,221,48,253,226,
    152,37,179,16,145,34,136,54,208,148,206,143,150,219,189,241,210,
    19,92,131,56,70,64,30,66,182,163,195,72,126,110,107,58,40,84,
    250,133,186,61,202,94,155,159,10,21,121,43,78,212,229,172,115,
    243,167,87,7,112,192,247,140,128,99,13,103,74,222,237,49,197,
    254,24,227,165,153,119,38,184,180,124,17,68,146,217,35,32,137,
    46,55,63,209,91,149,188,207,205,144,135,151,178,220,252,190,
    97,242,86,211,171,20,42,93,158,132,60,57,83,71,109,65,162,31,
    45,67,216,183,123,164,118,196,23,73,236,127,12,111,246,108,
    161,59,82,41,157,85,170,251,96,134,177,187,204,62,90,203,89,
    95,176,156,169,160,81,11,245,22,235,122,117,44,215,79,174,
    213,233,230,231,173,232,116,214,244,234,168,80,88,175,
};

// ====================================================================
// QRBuffer — stores data as a string of '0'/'1' characters
// ====================================================================
class QRBuffer {
    std::string m_bits;
public:
    int  size() const { return (int)m_bits.size(); }
    const std::string& str() const { return m_bits; }
    void clear() { m_bits.clear(); }

    void add(int val, int count) {
        if (count <= 0) return;
        for (int i = count - 1; i >= 0; i--)
            m_bits += (char)('0' + ((val >> i) & 1));
    }

    std::vector<uint8_t> bytes() const {
        const int n = (int)m_bits.size();
        const int nb = (n + 7) / 8;
        std::vector<uint8_t> result(nb, 0);
        for (int i = 0; i < n; i++)
            if (m_bits[i] == '1')
                result[i / 8] |= uint8_t(1 << (7 - (i % 8)));
        return result;
    }
};

// ====================================================================
// QRBitmap — bit-packed 2D bitmap
// ====================================================================
class QRBitmap {
    int m_w, m_h;
    std::vector<uint64_t> m_data;
public:
    QRBitmap() : m_w(0), m_h(0) {}
    QRBitmap(int w, int h) : m_w(w), m_h(h), m_data(((w*h)+63)/64, 0) {}

    int width()  const { return m_w; }
    int height() const { return m_h; }

    // No bounds-checking to match Go behaviour (negative index accesses data[0]).
    bool at(int x, int y) const {
        int i = y * m_w + x;
        return (m_data[i / 64] & (uint64_t(1) << (i & 63))) != 0;
    }
    void set(int x, int y, bool v) {
        int i = y * m_w + x;
        if (v)
            m_data[i / 64] |=  (uint64_t(1) << (i & 63));
        else
            m_data[i / 64] &= ~(uint64_t(1) << (i & 63));
    }
    void fill(int x, int y, int w, int h, bool v) {
        for (int r = y; r < y+h; r++)
            for (int c = x; c < x+w; c++)
                set(c, r, v);
    }
    void place(int ox, int oy, const QRBitmap& other) {
        for (int r = oy; r < oy + other.m_h; r++)
            for (int c = ox; c < ox + other.m_w; c++)
                set(c, r, other.at(c-ox, r-oy));
    }
    void invert() {
        for (auto& d : m_data) d = ~d;
    }
    QRBitmap copy() const { return *this; }
};

// ====================================================================
// Helpers
// ====================================================================
static int qr_min(int a, int b) { return a < b ? a : b; }
static int qr_max(int a, int b) { return a > b ? a : b; }
static int qr_abs(int a)        { return a < 0 ? -a : a; }

static int findMode(const std::string& d) {
    if (d.empty()) return 4;
    static const char* kA = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";
    bool isNum = true, isAlpha = true;
    for (unsigned char c : d) {
        if (c < '0' || c > '9') isNum = false;
        if (!isAlpha) continue;
        bool found = false;
        for (int i = 0; kA[i]; i++) {
            if ((unsigned char)kA[i] == c) {
                found = true;
                break;
            }
        }
        if (!found) isAlpha = false;
    }
    return isNum ? 1 : (isAlpha ? 2 : 4);
}

static int lengthBits(int version, int mode) {
    if (version < 10) {
        if (mode == 1)
            return 10;
        if (mode == 2)
            return 9;
        return 8;
    } else if (version < 27) {
        if (mode == 1)
            return 12;
        if (mode == 2)
            return 11;
        return 16;
    } else {
        if (mode == 1)
            return 14;
        if (mode == 2)
            return 13;
        return 16;
    }
}

static bool applyMask(int m, int x, int y) {
    switch (m) {
    case 0: return (y+x)%2 == 0;
    case 1: return  y%2    == 0;
    case 2: return  x%3    == 0;
    case 3: return (y+x)%3 == 0;
    case 4: return (y/2+x/3)%2 == 0;
    case 5: return (y*x)%2+(y*x)%3 == 0;
    case 6: return ((y*x)%2+(y*x)%3)%2 == 0;
    case 7: return ((y*x)%3+(y+x)%2)%2 == 0;
    default: return false;
    }
}

// ====================================================================
// QRCode (internal)
// ====================================================================
struct QRCode {
    int version, size, mode, errIdx; // errIdx: 0=L,1=M,2=Q,3=H
    QRBitmap qr, mask_bmp;

    // --- encode data into buffer ---
    void encode(QRBuffer& buf, const std::string& data) const {
        static const char* kAN = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";
        switch (mode) {
        case 1: // Numeric
            for (int i = 0; i < (int)data.size(); i += 3) {
                int end = qr_min((int)data.size(), i+3);
                int val = 0;
                for (int j = i; j < end; j++) val = val*10 + (data[j]-'0');
                int bits = end-i == 1 ? 4 : end-i == 2 ? 7 : 10;
                buf.add(val, bits);
            }
            break;
        case 2: // AlphaNum
            for (int i = 0; i < (int)data.size(); i += 2) {
                auto idx = [&](char c) -> int {
                    for (int k = 0; kAN[k]; k++)
                        if (kAN[k] == c)
                            return k;
                    return 0;
                };
                if (i+1 < (int)data.size())
                    buf.add(idx(data[i])*45 + idx(data[i+1]), 11);
                else
                    buf.add(idx(data[i]), 6);
            }
            break;
        default: // Byte (mode 4)
            for (unsigned char b : data) buf.add(b, 8);
            break;
        }
    }

    // --- find smallest version that fits ---
    int findOptimalVersion(const std::string& data) const {
        QRBuffer tmp;
        encode(tmp, data);
        for (int v = 1; v <= 40; v++) {
            int idx = (v-1)*4 + errIdx;
            const BlockEntry& bd = kBlocks[idx];
            int maxbytes = bd.n1 * bd.d1 + bd.n2 * bd.d2;
            int sz = 4 + lengthBits(v, mode) + tmp.size();
            sz += qr_max(qr_min(4, kCapacity[idx]-sz), 0);
            sz += (8 - sz%8) % 8;
            if (sz/8 <= maxbytes) return v;
        }
        return 42;
    }

    // --- Reed-Solomon error correction for one data block ---
    std::vector<uint8_t> encodeError(const std::vector<uint8_t>& block, int blockIdx) const {
        const BlockEntry& bd = kBlocks[(version-1)*4 + errIdx];
        int errorwords = bd.t1 - bd.d1;
        int cwPerBlock = (blockIdx < bd.n1) ? bd.d1 : bd.d2;

        const PolyEntry* poly = findPoly(errorwords);
        if (!poly) return {};

        std::vector<uint8_t> rs(block.size() + errorwords, 0);
        std::copy(block.begin(), block.end(), rs.begin());

        int offset = 0;
        for (int i = 0; i < cwPerBlock; i++) {
            uint8_t coeff = rs[offset];
            rs[offset] = 0;
            offset++;
            if (coeff == 0)
                continue;
            int alpExp = kLog[coeff];
            for (int g = 0; g < poly->len; g++) {
                int v = alpExp + poly->coeff[g];
                if (v > 255)
                    v %= 255;
                rs[offset + g] ^= (uint8_t)kExp[v];
            }
        }

        std::vector<uint8_t> result(rs.begin() + offset, rs.end());
        while ((int)result.size() < cwPerBlock) result.push_back(0);
        return result;
    }

    // --- functional patterns ---
    void addPositionPatterns() {
        // Top-Left
        qr.fill(0, 0, 7, 7, true);
        qr.fill(1, 1, 5, 5, false);
        qr.fill(2, 2, 3, 3, true);
        // Bottom-Left
        qr.fill(0, size-7, 7, 7, true);
        qr.fill(1, size-6, 5, 5, false);
        qr.fill(2, size-5, 3, 3, true);
        // Top-Right
        qr.fill(size-7, 0, 7, 7, true);
        qr.fill(size-6, 1, 5, 5, false);
        qr.fill(size-5, 2, 3, 3, true);

        mask_bmp.fill(0, 0, 9, 9, true);
        mask_bmp.fill(0, size-8, 9, 8, true);
        mask_bmp.fill(size-8, 0, 8, 9, true);
    }

    void addTimingPatterns() {
        for (int i = 8; i < size-8; i += 2) {
            qr.set(i, 6, true);
            qr.set(6, i, true);
            mask_bmp.set(i, 6, true);
            mask_bmp.set(i+1, 6, true);
            mask_bmp.set(6, i, true);
            mask_bmp.set(6, i+1, true);
        }
    }

    void addAlignmentPatterns() {
        const AlignList& al = kAlignPos[version-1];
        for (int i = 0; i < al.n; i++) {
            for (int j = 0; j < al.n; j++) {
                // Skip corners that would overlap finder patterns
                if ((i==0&&j==0)||(i==al.n-1&&j==0)||(i==0&&j==al.n-1))
                    continue;
                int x = al.pos[i]-2, y = al.pos[j]-2;
                qr.fill(x,y,5,5,true);
                qr.fill(x+1,y+1,3,3,false);
                qr.set(x+2,y+2,true);
                mask_bmp.fill(x,y,5,5,true);
            }
        }
    }

    void addVersionInformation() {
        if (version < 7) return;
        mask_bmp.fill(0,size-11,6,3,true);
        mask_bmp.fill(size-11,0,3,6,true);
        int bits = kVersionBits[version];
        int index = 0;
        for (int x = 0; x < 6; x++)
            for (int y = size-11; y < size-8; y++)
                qr.set(x, y, (bits & (1<<index++)) != 0);
        index = 0;
        for (int y = 0; y < 6; y++)
            for (int x = size-11; x < size-8; x++)
                qr.set(x, y, (bits & (1<<index++)) != 0);
    }

    void addFormatInformation(int maskNum) {
        // errMLHQ: M=0, L=1, H=2, Q=3
        static const int kMLHQ[] = {1,0,3,2}; // L=1, M=0, Q=3, H=2
        int err = kMLHQ[errIdx];
        int fmt = kFormatBits[maskNum | (err << 3)];

        int index = 0;
        for (int y = 0; y < 9; y++) {
            if (y == 6)
                continue;
            qr.set(8, y, (fmt & (1<<index++)) != 0);
        }
        for (int x = 7; x >= 0; x--) {
            if (x == 6)
                continue;
            qr.set(x, 8, (fmt & (1<<index++)) != 0);
        }
        index = 0;
        for (int x = size-1; x >= size-8; x--)
            qr.set(x, 8, (fmt & (1<<index++)) != 0);
        for (int y = size-7; y < size; y++)
            qr.set(8, y, (fmt & (1<<index++)) != 0);
        qr.set(8, size-8, true); // dark module
    }

    // --- place data bits into bitmap ---
    void placeBits(QRBitmap& bmp, const std::string& bitstream, int maskNum) const {
        int inc = -1, row = size-1, bitIdx = 0;
        int c = size - 1;
        while (c > 0) {
            if (c == 6)
                c--;  // skip timing column
            int col = c;
            for (;;) {
                for (int i = col; i > col-2; i--) {
                    if (mask_bmp.at(i, row)) {
                        bool dark = false;
                        if (bitIdx < (int)bitstream.size())
                            dark = bitstream[bitIdx++] == '1';
                        if (applyMask(maskNum, i, row))
                            dark = !dark;
                        bmp.set(i, row, dark);
                    }
                }
                row += inc;
                if (row < 0 || size <= row) {
                    row -= inc;
                    inc = -inc;
                    break;
                }
            }
            c -= 2;
        }
    }

    // --- score a mask candidate ---
    int scoreMask(const QRBitmap& bmp) const {
        int score = 0;

        // Rule 1: 5+ consecutive same-colour modules
        for (int y = 0; y < size; y++) {
            int count = 1;
            bool prev = bmp.at(0, y);
            for (int x = 1; x < size; x++) {
                bool curr = bmp.at(x, y);
                if (curr == prev) {
                    count++;
                } else {
                    if (count >= 5)
                        score += count - 5 + 3;
                    count = 1;
                    prev = curr;
                }
            }
            if (count >= 5)
                score += count - 5 + 3;
        }
        for (int x = 0; x < size; x++) {
            int count = 1;
            bool prev = bmp.at(x, 0);
            for (int y = 1; y < size; y++) {
                bool curr = bmp.at(x, y);
                if (curr == prev) {
                    count++;
                } else {
                    if (count >= 5)
                        score += count - 5 + 3;
                    count = 1;
                    prev = curr;
                }
            }
            if (count >= 5)
                score += count - 5 + 3;
        }

        // Rule 2: 2x2 blocks
        for (int y = 0; y < size-1; y++)
            for (int x = 0; x < size-1; x++) {
                bool curr = bmp.at(x, y);
                if (curr==bmp.at(x+1,y) && curr==bmp.at(x,y+1) && curr==bmp.at(x+1,y+1))
                    score += 3;
            }

        // Rule 3: 1011101 pattern
        for (int y = 0; y < size-7; y++)
            for (int x = 0; x < size-7; x++) {
                int pat = 0b1011101, cr=0, cc=0;
                for (int i = 0; i < 7; i++) {
                    if (bmp.at(x+i, y) == ((pat & (1<<i)) != 0))
                        cr++;
                    if (bmp.at(x, y+i) == ((pat & (1<<i)) != 0))
                        cc++;
                }
                if (cr == 7)
                    score += 40;
                if (cc == 7)
                    score += 40;
            }

        // Rule 4: dark/light ratio
        int dark = 0;
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++)
                if (bmp.at(x, y))
                    dark++;
        double ratio = (double)dark / (double)(size*size) * 100.0 - 50.0;
        score += (int)((double)qr_abs((int)ratio) / 5.0 * 10.0);

        return score;
    }

    // --- find best mask (0-7) ---
    int findBestMask(const std::string& bitstream) {
        int best = 0, bestScore = 0;
        for (int m = 0; m < 8; m++) {
            QRBitmap tmp = qr.copy();
            placeBits(tmp, bitstream, m);
            int s = scoreMask(tmp);
            if (m == 0 || s < bestScore) {
                bestScore = s;
                best = m;
            }
        }
        return best;
    }
};

} // anonymous namespace

// ====================================================================
// Public API
// ====================================================================
bool GenerateQRCode(const std::string& data,
                    QRErrorLevel level,
                    std::vector<bool>& outBitmap,
                    int& outSize)
{
    outBitmap.clear();
    outSize = 0;

    QRCode qr;
    qr.errIdx = (int)level;
    qr.mode   = findMode(data);
    qr.version = qr.findOptimalVersion(data);
    if (qr.version > 40) return false; // data too large

    qr.size = qr.version * 4 + 17;
    qr.qr       = QRBitmap(qr.size, qr.size);
    qr.mask_bmp = QRBitmap(qr.size, qr.size);

    // ---- Build data bitstream ----
    QRBuffer buf;
    buf.add(qr.mode, 4);
    buf.add((int)data.size(), lengthBits(qr.version, qr.mode));
    qr.encode(buf, data);

    int capIdx = (qr.version-1)*4 + qr.errIdx;
    // Termination bits
    buf.add(0, qr_min(4, kCapacity[capIdx] - buf.size()));
    // Pad to multiple of 8
    buf.add(0, (8 - buf.size()%8) % 8);
    // Fill remaining capacity
    int remaining = (kCapacity[capIdx] - buf.size()) / 8;
    for (int i = 0; i < remaining; i++)
        buf.add(i%2==0 ? 0b11101100 : 0b00010001, 8);

    // ---- Split into blocks & generate EC ----
    const BlockEntry& bd = kBlocks[capIdx];
    int totalBlocks = bd.n1 + bd.n2;
    std::vector<std::vector<uint8_t>> dataBlocks(totalBlocks);
    std::vector<std::vector<uint8_t>> ecBlocks(totalBlocks);

    std::vector<uint8_t> allBytes = buf.bytes();
    int cur = 0;
    for (int i = 0; i < totalBlocks; i++) {
        int dw = (i < bd.n1) ? bd.d1 : bd.d2;
        dataBlocks[i].assign(allBytes.begin()+cur, allBytes.begin()+cur+dw);
        cur += dw;
    }
    for (int i = 0; i < totalBlocks; i++)
        ecBlocks[i] = qr.encodeError(dataBlocks[i], i);

    // ---- Interleave ----
    buf.clear();
    int largestBlock = bd.d1;
    if (bd.n2 > 0) largestBlock = qr_max(largestBlock, bd.d2);
    int errorwords = bd.t1 - bd.d1;

    for (int i = 0; i < largestBlock + errorwords; i++)
        for (int b = 0; b < totalBlocks; b++)
            if (i < (int)dataBlocks[b].size())
                buf.add(dataBlocks[b][i], 8);
    for (int i = 0; i < errorwords; i++)
        for (int b = 0; b < totalBlocks; b++)
            if (i < (int)ecBlocks[b].size())
                buf.add(ecBlocks[b][i], 8);

    // ---- Functional patterns ----
    qr.addPositionPatterns();
    qr.addTimingPatterns();
    qr.addAlignmentPatterns();
    qr.addVersionInformation();

    qr.mask_bmp.invert(); // now true = data area

    // ---- Pick mask & finalise ----
    std::string bitstream = buf.str();
    int bestMask = qr.findBestMask(bitstream);
    qr.addFormatInformation(bestMask);
    qr.placeBits(qr.qr, bitstream, bestMask);

    // ---- Add 1-module quiet zone ----
    const int qz = 1;
    QRBitmap final_bmp(qr.size + 2 * qz, qr.size + 2 * qz);
    final_bmp.place(qz, qz, qr.qr);

    // ---- Export ----
    outSize = qr.size + 2 * qz;
    outBitmap.resize(outSize * outSize);
    for (int y = 0; y < outSize; y++)
        for (int x = 0; x < outSize; x++)
            outBitmap[y * outSize + x] = final_bmp.at(x, y);

    return true;
}
