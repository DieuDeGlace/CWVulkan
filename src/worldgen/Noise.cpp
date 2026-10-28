// The world generator's building blocks, written from docs/specs/worldgen-terrain.md §1 and the
// tables in worldgen-terrain-tables.md §1.
#include "worldgen/Noise.h"

namespace worldgen {

uint32_t hash(uint32_t n) {
    const uint32_t v = n ^ (n << 13);
    uint32_t t = v * v;
    t = t * 60493u + 19990303u;
    t = t * v;
    t = t + 1376312589u; // the game adds 0xd208dd0d; bit 31 is masked off below
    return t & 0x7fffffffu;
}

uint32_t random(uint32_t n) { return 100000u - hash(n) / 21474u; }

bool oddHash(uint32_t n) { return (hash(n) / 21474u) & 1u; }

namespace {

// Ken Perlin's permutation (tables §1), the game's copy at 0x470ed0.
constexpr uint8_t PERMUTATION[256] = {
    151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103, 30,  69,  142,
    8,   99,  37,  240, 21,  10,  23,  190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203,
    117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136, 171, 168, 68,  175, 74,  165,
    71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,  41,
    55,  46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209, 76,  132, 187, 208, 89,
    18,  169, 200, 196, 135, 130, 116, 188, 159, 86,  164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250,
    124, 123, 5,   202, 38,  147, 118, 126, 255, 82,  85,  212, 207, 206, 59,  227, 47,  16,  58,  17,  182, 189,
    28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,
    129, 22,  39,  253, 19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,  228, 251, 34,
    242, 193, 238, 210, 144, 12,  191, 179, 162, 241, 81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,
    181, 199, 106, 157, 184, 84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,  222, 114,
    67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156, 180};

// The twelve gradient directions (only their x and y are used in 2D).
constexpr double GRADIENT_X[12] = {1, -1, 1, -1, 1, -1, 1, -1, 0, 0, 0, 0};
constexpr double GRADIENT_Y[12] = {1, 1, -1, -1, 0, 0, 0, 0, 1, -1, 1, -1};

// The doubled permutation and its gradient indices, as the game fills them at start-up.
struct Tables {
    int perm[512];
    int gradient[512];
    double skew;   // F2 = (sqrt(3) - 1) / 2
    double unskew; // G2 = (3 - sqrt(3)) / 6
    Tables() {
        for (int i = 0; i < 512; i++) {
            perm[i] = PERMUTATION[i & 255];
            gradient[i] = perm[i] % 12;
        }
        skew = (std::sqrt(3.0) - 1.0) * 0.5;
        unskew = (3.0 - std::sqrt(3.0)) / 6.0;
    }
};
const Tables tables;

// The floor of a double as a 32-bit integer, comparing in double (§1.4 step 3).
int32_t cellOf(double v) {
    int32_t i = trunc32(v);
    if ((double)i > v)
        i -= 1;
    return i;
}

// One corner's contribution (step 9).
double corner(double x, double y, int gradient) {
    double a = (0.5 - x * x) - y * y;
    if (0.0 > a)
        return 0.0;
    a = a * a;
    a = a * a;
    return ((y * GRADIENT_Y[gradient]) + (x * GRADIENT_X[gradient])) * a;
}

} // namespace

float simplex(double xIn, double yIn) {
    const double x = xIn * 0.32, y = yIn * 0.32;
    // Which simplex cell the point is in.
    const double s = (x + y) * tables.skew;
    const int32_t i = cellOf(s + x), j = cellOf(s + y);
    const double t = (double)(int32_t)((uint32_t)i + (uint32_t)j) * tables.unskew;
    const double x0 = x - ((double)i - t), y0 = y - ((double)j - t);
    // The middle corner: the lower or the upper triangle.
    const int i1 = x0 > y0 ? 1 : 0, j1 = x0 > y0 ? 0 : 1;
    const double x1 = (x0 - (double)i1) + tables.unskew, y1 = (y0 - (double)j1) + tables.unskew;
    const double x2 = (x0 - 1.0) + tables.unskew * 2.0, y2 = (y0 - 1.0) + tables.unskew * 2.0;
    const int ii = i & 255, jj = j & 255;
    const int g0 = tables.gradient[tables.perm[jj] + ii];
    const int g1 = tables.gradient[tables.perm[jj + j1] + ii + i1];
    const int g2 = tables.gradient[tables.perm[jj + 1] + ii + 1];
    const double n0 = corner(x0, y0, g0), n1 = corner(x1, y1, g1), n2 = corner(x2, y2, g2);
    return (float)(((n1 + n0) + n2) * 70.0);
}

} // namespace worldgen
