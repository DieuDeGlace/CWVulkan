#pragma once
// The math functions the game takes from the system's ucrtbase.dll (cosf, sinf, asinf,
// acosf, expf, powf, asin). The static CRT's versions can differ in the last bit, so code
// that must match the game bit for bit calls these; elsewhere they fall back to the CRT's own.
namespace nativecrt {

float cosf(float x);
float sinf(float x);
float asinf(float x);
float acosf(float x);
float expf(float x);
float powf(float x, float y);
double asin(double x);

} // namespace nativecrt
