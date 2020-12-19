//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

inline float uint32_to_float(uint32_t i) { return float(i) * (1.f / (65536.f * 65536.f - 1.f)); }

//-------------------------------------------------------------------------------------------------------

inline float int32_to_float(int32_t i) { return float(i) * (1.f / (32768.f * 65536.f)); }

//-------------------------------------------------------------------------------------------------------

inline float uint8_to_float(uint8_t i) { return float(i) * (1.f / 255.f); }

//----------------------------------------------------------------------------------------

inline float int8_to_float(int8_t i) { return float(i) * (1.f / 128.f); }

//----------------------------------------------------------------------------------------

inline float uint16_to_float(uint16_t i) { return float(i) * (1.f / 65535.f); }

//----------------------------------------------------------------------------------------

inline float int16_to_float(int16_t i) { return float(i) * (1.f / 32768.f); }

//----------------------------------------------------------------------------------------

inline float int100_to_float(int i) { return float(i) * (1.f / 100.f); }

//----------------------------------------------------------------------------------------

inline float int127_to_float(int i) { return float(i) * (1.f / 127.f); }

//----------------------------------------------------------------------------------------

inline bool within_range(int lo, int value, int hi) { return ((value >= lo) && (value <= hi)); }

//----------------------------------------------------------------------------------------

inline float clamp01(float in)
{
    if (in > 1.0f)
        return 1.0f;
    if (in < 0.0f)
        return 0.0f;
    return in;
}

//----------------------------------------------------------------------------------------

inline float clamp1bp(float in)
{
    if (in > 1.0f)
        return 1.0f;
    if (in < -1.0f)
        return -1.0f;
    return in;
}

//----------------------------------------------------------------------------------------

inline float lerp(float a, float b, float x) { return (1.f - x) * a + x * b; }

//----------------------------------------------------------------------------------------

inline double tanh_fast(double in)
{
    double x = fabs(in);
    const double a = 2.0 / 3.0;
    double xx = x * x;
    double denom = 1.0 + x + xx + a * x * xx;
    return ((in > 0.0) ? 1.0 : -1.0) * (1.0 - 1.0 / denom);
}

//----------------------------------------------------------------------------------------

inline double tanh_faster(double x)
{
    const double a = -1 / 3, b = 2 / 15;
    // return tanh(x);
    double xs = x * x;
    double y = 1 + xs * a + xs * xs * b;
    return y * x;
}

//----------------------------------------------------------------------------------------

#define PI 3.14159265359

//----------------------------------------------------------------------------------------

double sincf(double x);
double blackman(int i, int n);
double hanning(int i, int n);
double hamming(int i, int n);
float clamp01(float in);

//----------------------------------------------------------------------------------------

inline double sincf(double x)
{
    if (x == 0)
        return 1;
    return (sin(PI * x)) / (PI * x);
}

//----------------------------------------------------------------------------------------

inline double blackman(int i, int n)
{
    // if (i>=n) return 0;
    return (0.42 - 0.5 * cos(2 * PI * i / (n - 1)) + 0.08 * cos(4 * PI * i / (n - 1)));
}

//----------------------------------------------------------------------------------------

inline float minf(float a, float b) { return 0.5f * (a + b - fabsf(a - b)); }

//----------------------------------------------------------------------------------------

inline double symmetric_blackman(double i, int n)
{
    // if (i>=n) return 0;
    i -= (n / 2);
    return (0.42 - 0.5 * cos(2 * PI * i / (n)) + 0.08 * cos(4 * PI * i / (n)));
}

//----------------------------------------------------------------------------------------

inline double symmetric_nuttall(double i, int n)
{
    double a0 = 0.3635819;
    double a1 = 0.4891775;
    double a2 = 0.1365995;
    double a3 = .010641;

    i -= (n / 2);
    return a0 - a1 * cos(2.0 * PI * i / (n)) + a2 * cos(4.0 * PI * i / (n)) -
           a3 * cos(6.0 * PI * i / (n));
}

//----------------------------------------------------------------------------------------

inline double BESSI0(double X)
{
    double Y, P1, P2, P3, P4, P5, P6, P7, Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8, Q9, AX, BX;
    P1 = 1.0;
    P2 = 3.5156229;
    P3 = 3.0899424;
    P4 = 1.2067429;
    P5 = 0.2659732;
    P6 = 0.360768e-1;
    P7 = 0.45813e-2;
    Q1 = 0.39894228;
    Q2 = 0.1328592e-1;
    Q3 = 0.225319e-2;
    Q4 = -0.157565e-2;
    Q5 = 0.916281e-2;
    Q6 = -0.2057706e-1;
    Q7 = 0.2635537e-1;
    Q8 = -0.1647633e-1;
    Q9 = 0.392377e-2;
    if (fabs(X) < 3.75)
    {
        Y = (X / 3.75) * (X / 3.75);
        return (P1 + Y * (P2 + Y * (P3 + Y * (P4 + Y * (P5 + Y * (P6 + Y * P7))))));
    }
    else
    {
        AX = fabs(X);
        Y = 3.75 / AX;
        BX = exp(AX) / sqrt(AX);
        AX = Q1 +
             Y * (Q2 + Y * (Q3 + Y * (Q4 + Y * (Q5 + Y * (Q6 + Y * (Q7 + Y * (Q8 + Y * Q9)))))));
        return (AX * BX);
    }
}

//----------------------------------------------------------------------------------------

inline double SymmetricKaiser(double x, int nint, double Alpha)
{
    double N = (double)nint;
    x += N * 0.5;

    if (x > N)
        x = N;
    if (x < 0.0)
        x = 0.0;
    // x = std::min((double)x, (double)N);
    // x = std::max(x, 0.0);
    double a = (2.0 * x / N - 1.0);
    return BESSI0(PI * Alpha * sqrt(1.0 - a * a)) / BESSI0(PI * Alpha);
}

//----------------------------------------------------------------------------------------

inline double blackman(double i, int n)
{
    // if (i>=n) return 0;
    return (0.42 - 0.5 * cos(2 * PI * i / (n - 1)) + 0.08 * cos(4 * PI * i / (n - 1)));
}

//----------------------------------------------------------------------------------------

inline double hanning(int i, int n)
{
    if (i >= n)
        return 0;
    return 0.5 * (1 - cos(2 * PI * i / (n - 1)));
}

//----------------------------------------------------------------------------------------

inline double hamming(int i, int n)
{
    if (i >= n)
        return 0;
    return 0.54 - 0.46 * cos(2 * PI * i / (n - 1));
}
