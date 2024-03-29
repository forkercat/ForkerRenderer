//
// Created by Junhao Wang (@Forkercat) on 2021/4/5.
//

#pragma once

#include <spdlog/stopwatch.h>

#include "constant.h"

// Utility Inline Functions

inline std::string Ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \n\r\t\f\v");
    return (start == std::string::npos) ? "" : s.substr(start);
}

inline void TimeElapsed(spdlog::stopwatch& sw, std::string note = "")
{
    spdlog::info("------------------------------------------------------------");
    spdlog::info("Time Used: {:.6} Seconds ({})\n", sw, note);
    sw.reset();
}

inline Float Lerp(Float t, Float v1, Float v2)
{
    return (1 - t) * v1 + t * v2;
}

template <typename T>
inline T Clamp(T val, T min, T max)
{
    return std::min(max, std::max(val, min));
}

template <typename T>
inline T Clamp01(T val)
{
    return Clamp(val, (T)0, (T)1);
}

inline Float Smoothstep(Float edge0, Float edge1, Float x)
{
    // Scale, bias, and saturate x to 0..1 range
    x = Clamp01((x - edge0) / (edge1 - edge0));
    // Evaluate polynomial
    return x * x * (3 - 2 * x);
}

inline Float Radians(Float deg)
{
    return deg * Pi / 180.f;
}

inline Float Degrees(Float rad)
{
    return rad * 180.f / Pi;
}

template <typename T>
inline T Min(const T& v1, const T& v2)
{
    return std::min(v1, v2);
}

template <typename T>
inline T Max(const T& v1, const T& v2)
{
    return std::max(v1, v2);
}

template <typename T>
inline T Min3(const T& v1, const T& v2, const T& v3)
{
    return std::min(v1, std::min(v2, v3));
}

template <typename T>
inline T Max3(const T& v1, const T& v2, const T& v3)
{
    return std::max(v1, std::max(v2, v3));
}

inline Float Pow(Float val, Float pval)
{
    return std::pow(val, pval);
}

inline Float Random01()
{
    // Old
    // return rand() / (RAND_MAX + 1.f);
    // New
    static std::uniform_real_distribution<Float> distribution(0.f, 1.f);
    static std::mt19937                          generator;
    return distribution(generator);
}

inline Float Random(Float min, Float max)
{
    return min + (max - min) * Random01();
}