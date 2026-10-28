#include "NativeCrt.h"

#include <cmath>
#include <type_traits>

#ifdef _WIN32
extern "C" __declspec(dllimport) void *__stdcall LoadLibraryA(const char *name);
extern "C" __declspec(dllimport) void *__stdcall GetProcAddress(void *module, const char *name);
#endif

namespace nativecrt {
namespace {

struct Crt {
    float (*cos)(float) = ::cosf, (*sin)(float) = ::sinf, (*asin)(float) = ::asinf, (*acos)(float) = ::acosf;
    float (*exp)(float) = ::expf, (*pow)(float, float) = ::powf;
    double (*asinD)(double) = ::asin;
    Crt() {
#ifdef _WIN32
        if (void *m = LoadLibraryA("ucrtbase.dll")) {
            auto get = [m](auto &slot, const char *name) {
                if (void *f = GetProcAddress(m, name))
                    slot = (std::remove_reference_t<decltype(slot)>)f;
            };
            get(cos, "cosf");
            get(sin, "sinf");
            get(asin, "asinf");
            get(acos, "acosf");
            get(exp, "expf");
            get(pow, "powf");
            get(asinD, "asin");
        }
#endif
    }
};
const Crt &crt() {
    static const Crt c;
    return c;
}

} // namespace

float cosf(float x) { return crt().cos(x); }
float sinf(float x) { return crt().sin(x); }
float asinf(float x) { return crt().asin(x); }
float acosf(float x) { return crt().acos(x); }
float expf(float x) { return crt().exp(x); }
float powf(float x, float y) { return crt().pow(x, y); }
double asin(double x) { return crt().asinD(x); }

} // namespace nativecrt
