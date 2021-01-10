//
// Created by Junhao Wang (@Forkercat) on 2020/12/29.
// Simplified Version From mmp/pbrt-v4/check.h
//

#include <spdlog/spdlog.h>

#ifndef _UTIL_CHECK_H_
#define _UTIL_CHECK_H_

// CHECK Macro Definitions
// #define CHECK(x) \
//     (!(!(x) && (spdlog::error("CHECK Failed: {} | {}() {}:{}", #x, __func__, __FILE__,
//     \
//                               __LINE__), \
//                 true)))

#define CHECK(x) assert(x)

#define CHECK_EQ(a, b) CHECK_IMPL(a, b, ==)
#define CHECK_NE(a, b) CHECK_IMPL(a, b, !=)
#define CHECK_GT(a, b) CHECK_IMPL(a, b, >)
#define CHECK_GE(a, b) CHECK_IMPL(a, b, >=)
#define CHECK_LT(a, b) CHECK_IMPL(a, b, <)
#define CHECK_LE(a, b) CHECK_IMPL(a, b, <=)

#define CHECK_IMPL(a, b, op) assert((a)op(b))

// #define NDEBUG

#ifndef NDEBUG

#define DCHECK(x) (CHECK(x))
#define DCHECK_EQ(a, b) CHECK_EQ(a, b)
#define DCHECK_NE(a, b) CHECK_NE(a, b)
#define DCHECK_GT(a, b) CHECK_GT(a, b)
#define DCHECK_GE(a, b) CHECK_GE(a, b)
#define DCHECK_LT(a, b) CHECK_LT(a, b)
#define DCHECK_LE(a, b) CHECK_LE(a, b)

#else

#define DCHECK(x)
#define DCHECK_EQ(a, b)
#define DCHECK_NE(a, b)
#define DCHECK_GT(a, b)
#define DCHECK_GE(a, b)
#define DCHECK_LT(a, b)
#define DCHECK_LE(a, b)

#endif

#endif  // _UTIL_CHECK_H_
