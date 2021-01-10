//
// Created by Junhao Wang (@Forkercat) on 2020/12/18.
// Reference: ssloy/TinyRenderer and mmp/pbrt-v3
//

#ifndef _GEOMETRY_H_
#define _GEOMETRY_H_

#include <spdlog/fmt/ostr.h>

#include <cassert>
#include <cmath>
#include <ostream>

#include "check.h"

/////////////////////////////////////////////////////////////////////////////////

// #define FLOAT_AS_DOUBLE

#ifdef FLOAT_AS_DOUBLE
typedef double Float;
#else
typedef float Float;
#endif

#define MinInt std::numeric_limits<int>::min()
#define MaxInt std::numeric_limits<int>::max()
#define MaxFloat std::numeric_limits<Float>::max()
#define MinFloat std::numeric_limits<Float>::min()
#define Infinity std::numeric_limits<Float>::infinity()

/////////////////////////////////////////////////////////////////////////////////

static const Float Pi = 3.14159265358979323846;
static const Float InvPi = 0.31830988618379067154;
static const Float Inv2Pi = 0.15915494309189533577;
static const Float Inv4Pi = 0.07957747154594766788;
static const Float PiOver2 = 1.57079632679489661923;
static const Float PiOver4 = 0.78539816339744830961;
static const Float Sqrt2 = 1.41421356237309504880;

/////////////////////////////////////////////////////////////////////////////////

template <size_t DIM, typename T>
class Vector;

template <size_t DIM_COL, size_t DIM_ROW, typename T>
class Matrix;

template <typename T>
inline bool isNaN(const T x)
{
    return std::isnan(x);
}

template <>
inline bool isNaN(const int x)
{
    return false;
}

/////////////////////////////////////////////////////////////////////////////////
// Vector Declaration

template <size_t DIM, typename T>
class Vector
{
public:
    bool HasNaNs() const
    {
        for (size_t i = 0; i < DIM; ++i)
            if (isNaN(data_[i])) return true;
        return false;
    }

    Vector()
    {
        for (size_t i = 0; i < DIM; ++i)
            data_[i] = T();
    }

    explicit Vector(T val)
    {
        for (size_t i = 0; i < DIM; ++i)
            data_[i] = val;
    }

    T operator[](size_t i) const { return data_[i]; }

    T& operator[](size_t i) { return data_[i]; }

    Vector operator+() const { return Vector(*this); }

    Vector operator-() const
    {
        Vector ret;
        for (size_t i = 0; i < DIM; ++i)
            ret[i] = -data_[i];
        return ret;
    }

private:
    T data_[DIM];
};

// Vector2
template <typename T>
class Vector<2, T>
{
public:
    bool HasNaNs() const { return isNaN(x) || isNaN(y); }

    // Constructors
    Vector() : x(T()), y(T()) { }
    Vector(T xx, T yy) : x(xx), y(yy) { DCHECK(!HasNaNs()); }
    explicit Vector(T val) : x(val), y(val) { DCHECK(!HasNaNs()); }

#ifndef NDEBUG
    // If it is in DEBUG mode, we use these copy constructor and operator=
    // otherwise, we go for the default options
    Vector(const Vector<2, T>& v)
    {
        DCHECK(!v.HasNaNs());
        x = v.x;
        y = v.y;
    }

    Vector<2, T>& operator=(const Vector<2, T>& v)
    {
        DCHECK(!v.HasNaNs());
        x = v.x;
        y = v.y;
        return *this;
    }
#endif

    // Subscript Operators
    T operator[](size_t i) const
    {
        DCHECK(i >= 0 && i < 2);
        if (i == 0) return x;
        return y;
    }

    T& operator[](size_t i)
    {
        DCHECK(i >= 0 && i < 2);
        if (i == 0) return x;
        return y;
    }

    // Positive / Negative Operators
    Vector<2, T> operator+() const { return Vector<2, T>(x, y); }
    Vector<2, T> operator-() const { return Vector<2, T>(-x, -y); }

    // Equality Operators
    bool operator==(const Vector<2, T>& v) const { return x == v.x && y == v.y; }
    bool operator!=(const Vector<2, T>& v) const { return x != v.x || y != v.y; }

    // Add / Minus Operators
    Vector<2, T> operator+(const Vector<2, T>& v) const
    {
        DCHECK(!v.HasNaNs());
        return Vector<2, T>(x + v.x, y + v.y);
    }

    Vector<2, T> operator-(const Vector<2, T>& v) const
    {
        DCHECK(!v.HasNaNs());
        return Vector<2, T>(x - v.x, y - v.y);
    }

    Vector<2, T>& operator+=(const Vector<2, T>& v)
    {
        DCHECK(!v.HasNaNs());
        x += v.x;
        y += v.y;
        return *this;
    }

    Vector<2, T>& operator-=(const Vector<2, T>& v)
    {
        DCHECK(!v.HasNaNs());
        x -= v.x;
        y -= v.y;
        return *this;
    }

    // Product / Division
    Vector<2, T> operator*(const Vector<2, T>& v) const
    {
        return Vector<2, T>(x * v.x, y * v.y);
    }

    template <typename U>
    Vector<2, T> operator*(U f) const
    {
        return Vector<2, T>(x * f, y * f);
    }

    template <typename U>
    Vector<2, T> operator/(U f) const
    {
        CHECK_NE(f, 0);
        Float inv = (Float)1 / f;
        return Vector<2, T>(x * inv, y * inv);
    }

    template <typename U>
    Vector<2, T>& operator*=(U f)
    {
        DCHECK(!isNaN(f));
        x *= f;
        y *= f;
        return *this;
    }

    template <typename U>
    Vector<2, T>& operator/=(U f)
    {
        CHECK_NE(f, 0);
        Float inv = (Float)1 / f;
        x *= inv;
        y *= inv;
        return *this;
    }

    // Public Methods
    Float LengthSquared() const { return x * x + y * y; }
    Float Length() const { return std::sqrt(LengthSquared()); }

    // Public Data
    union
    {  // clang-format off
        struct { T x, y; };
        struct { T s, t; };
    };  // clang-format on
};

// Vector3
template <typename T>
class Vector<3, T>
{
public:
    bool HasNaNs() const { return isNaN(x) || isNaN(y) || isNaN(z); }

    // Constructors
    Vector() : x(T()), y(T()), z(T()) { }
    Vector(T xx, T yy, T zz) : x(xx), y(yy), z(zz) { DCHECK(!HasNaNs()); }
    explicit Vector(T val) : x(val), y(val), z(val) { DCHECK(!HasNaNs()); }

#ifndef NDEBUG
    // If it is in DEBUG mode, we use these copy constructor and operator=
    // otherwise, we go for the default options
    Vector(const Vector<3, T>& v)
    {
        DCHECK(!v.HasNaNs());
        x = v.x;
        y = v.y;
        z = v.z;
    }

    Vector<3, T>& operator=(const Vector<3, T>& v)
    {
        DCHECK(!v.HasNaNs());
        x = v.x;
        y = v.y;
        z = v.z;
        return *this;
    }
#endif

    // Swizzle Constructors
    Vector(T xx, const Vector<2, T>& v) : x(xx), y(v.x), z(v.y) { DCHECK(!HasNaNs()); }
    Vector(const Vector<2, T>& v, T zz) : x(v.x), y(v.y), z(zz) { DCHECK(!HasNaNs()); }

    // Subscript Operators
    T operator[](size_t i) const
    {
        DCHECK(i >= 0 && i < 3);
        if (i == 0) return x;
        if (i == 1) return y;
        return z;
    }

    T& operator[](size_t i)
    {
        DCHECK(i >= 0 && i < 3);
        if (i == 0) return x;
        if (i == 1) return y;
        return z;
    }

    // Positive / Negative Operators
    Vector<3, T> operator+() const { return Vector<3, T>(x, y, z); }
    Vector<3, T> operator-() const { return Vector<3, T>(-x, -y, -z); }

    // Equality Operators
    bool operator==(const Vector<3, T>& v) const
    {
        return x == v.x && y == v.y && z == v.z;
    }
    bool operator!=(const Vector<3, T>& v) const
    {
        return x != v.x || y != v.y || z != v.z;
    }

    // Add / Minus Operators
    Vector<3, T> operator+(const Vector<3, T>& v) const
    {
        DCHECK(!v.HasNaNs());
        return Vector<3, T>(x + v.x, y + v.y, z + v.z);
    }

    Vector<3, T> operator-(const Vector<3, T>& v) const
    {
        DCHECK(!v.HasNaNs());
        return Vector<3, T>(x - v.x, y - v.y, z - v.z);
    }

    Vector<3, T>& operator+=(const Vector<3, T>& v)
    {
        DCHECK(!v.HasNaNs());
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    Vector<3, T>& operator-=(const Vector<3, T>& v)
    {
        DCHECK(!v.HasNaNs());
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }

    // Product / Division
    Vector<3, T> operator*(const Vector<3, T>& v) const
    {
        return Vector<3, T>(x * v.x, y * v.y, z * v.z);
    }

    template <typename U>
    Vector<3, T> operator*(U f) const
    {
        return Vector<3, T>(x * f, y * f, z * f);
    }

    template <typename U>
    Vector<3, T> operator/(U f) const
    {
        CHECK_NE(f, 0);
        Float inv = (Float)1 / f;
        return Vector<3, T>(x * inv, y * inv, z * inv);
    }

    template <typename U>
    Vector<3, T>& operator*=(U f)
    {
        DCHECK(!isNaN(f));
        x *= f;
        y *= f;
        z *= f;
        return *this;
    }

    template <typename U>
    Vector<3, T>& operator/=(U f)
    {
        CHECK_NE(f, 0);
        Float inv = (Float)1 / f;
        x *= inv;
        y *= inv;
        z *= inv;
        return *this;
    }

    // Public Methods
    Float LengthSquared() const { return x * x + y * y + z * z; }
    Float Length() const { return std::sqrt(LengthSquared()); }

    // Public Data (glm-style swizzle)
    union
    {  // clang-format off
        struct { T x, y, z; };
        struct { T r, g, b; };
        struct { Vector<2, T> xy; };  // __z_ignore_it__
        struct { T __x_ignore_it__; Vector<2, T> yz; };
    };  // clang-format on
};

template <typename T>
class Vector<4, T>
{
public:
    bool HasNaNs() const { return isNaN(x) || isNaN(y) || isNaN(z) || isNaN(w); }

    // Constructors
    Vector() : x(T()), y(T()), z(T()), w(T()) { }
    Vector(T xx, T yy, T zz, T ww) : x(xx), y(yy), z(zz), w(ww) { DCHECK(!HasNaNs()); }
    Vector(T val) : x(val), y(val), z(val), w(val) { DCHECK(!HasNaNs()); }

#ifndef NDEBUG
    // If it is in DEBUG mode, we use these copy constructor and operator=
    // otherwise, we go for the default options
    Vector(const Vector<4, T>& v)
    {
        DCHECK(!v.HasNaNs());
        x = v.x;
        y = v.y;
        z = v.z;
        w = v.w;
    }

    Vector<4, T>& operator=(const Vector<4, T>& v)
    {
        DCHECK(!v.HasNaNs());
        x = v.x;
        y = v.y;
        z = v.z;
        w = v.w;
        return *this;
    }
#endif

    // Swizzle Constructors
    Vector(T xx, const Vector<3, T>& v) : x(xx), y(v.x), z(v.y), w(v.z)
    {
        DCHECK(!HasNaNs());
    }
    Vector(const Vector<3, T>& v, T ww) : x(v.x), y(v.y), z(v.z), w(ww)
    {
        DCHECK(!HasNaNs());
    }
    Vector(T xx, T yy, const Vector<2, T>& v) : x(xx), y(yy), z(v.x), w(v.y)
    {
        DCHECK(!HasNaNs());
    }
    Vector(const Vector<2, T>& v, T zz, T ww) : x(v.x), y(v.y), z(zz), w(ww)
    {
        DCHECK(!HasNaNs());
    }

    // Subscript Operators
    T operator[](size_t i) const
    {
        DCHECK(i >= 0 && i < 4);
        if (i == 0) return x;
        if (i == 1) return y;
        if (i == 2) return z;
        return w;
    }

    T& operator[](size_t i)
    {
        DCHECK(i >= 0 && i < 4);
        if (i == 0) return x;
        if (i == 1) return y;
        if (i == 2) return z;
        return w;
    }

    // Positive / Negative Operators
    Vector<4, T> operator+() const { return Vector<4, T>(x, y, z, w); }
    Vector<4, T> operator-() const { return Vector<4, T>(-x, -y, -z, -w); }

    // Equality Operators
    bool operator==(const Vector<4, T>& v) const
    {
        return x == v.x && y == v.y && z == v.z && w == v.w;
    }
    bool operator!=(const Vector<4, T>& v) const
    {
        return x != v.x || y != v.y || z != v.z || w != v.w;
    }

    // Add / Minus Operators
    Vector<4, T> operator+(const Vector<4, T>& v) const
    {
        DCHECK(!v.HasNaNs());
        return Vector<4, T>(x + v.x, y + v.y, z + v.z, w + v.w);
    }

    Vector<4, T> operator-(const Vector<4, T>& v) const
    {
        DCHECK(!v.HasNaNs());
        return Vector<4, T>(x - v.x, y - v.y, z - v.z, w - v.w);
    }

    Vector<4, T>& operator+=(const Vector<4, T>& v)
    {
        DCHECK(!v.HasNaNs());
        x += v.x;
        y += v.y;
        z += v.z;
        w += v.w;
        return *this;
    }

    Vector<4, T>& operator-=(const Vector<4, T>& v)
    {
        DCHECK(!v.HasNaNs());
        x -= v.x;
        y -= v.y;
        z -= v.z;
        w -= v.w;
        return *this;
    }

    // Product / Division
    Vector<4, T> operator*(const Vector<4, T>& v) const
    {
        return Vector<4, T>(x * v.x, y * v.y, z * v.z, w * v.w);
    }

    template <typename U>
    Vector<4, T> operator*(U f) const
    {
        return Vector<4, T>(x * f, y * f, z * f, w * f);
    }

    template <typename U>
    Vector<4, T> operator/(U f) const
    {
        CHECK_NE(f, 0);
        Float inv = (Float)1 / f;
        return Vector<4, T>(x * inv, y * inv, z * inv, w * inv);
    }

    template <typename U>
    Vector<4, T>& operator*=(U f)
    {
        DCHECK(!isNaN(f));
        x *= f;
        y *= f;
        z *= f;
        w *= f;
        return *this;
    }

    template <typename U>
    Vector<4, T>& operator/=(U f)
    {
        CHECK_NE(f, 0);
        Float inv = (Float)1 / f;
        x *= inv;
        y *= inv;
        z *= inv;
        w *= inv;
        return *this;
    }

    // Public Methods
    Float LengthSquared() const { return x * x + y * y + z * z + w * w; }
    Float Length() const { return std::sqrt(LengthSquared()); }

    // Public Data (glm-style swizzle)
    union
    {  // clang-format off
        struct { T x, y, z, w; };
        struct { Vector<2, T> xy; Vector<2, T> zw; };
        struct { Vector<3, T> xyz; };  // __w_ignore_it__
        struct { T __x_ignore_it__; Vector<3, T> yzw; };
    };  // clang-format on
};

/////////////////////////////////////////////////////////////////////////////////
// Vector Operators

template <size_t DIM, typename T>
std::ostream& operator<<(std::ostream& out, const Vector<DIM, T>& v)
{
    char buffer[50];
    out << "(";
    for (size_t i = 0; i < DIM - 1; ++i)
    {
        sprintf(buffer, "%7.8f", (Float)v[i]);
        out << buffer << ", ";
    }
    sprintf(buffer, "%7.8f", (Float)v[DIM - 1]);
    out << buffer << ")";
    return out;
}

/////////////////////////////////////////////////////////////////////////////////
// dt

template <size_t DIM, typename T>
struct dt
{
    static T det(const Matrix<DIM, DIM, T>& src)
    {
        T ret = 0;
        for (int i = 0; i < DIM; ++i)
        {
            // expand by the first row (cofactor expansion)
            ret += src[0][i] * src.Cofactor(0, i);
        }
        return ret;
    }
};

template <typename T>
struct dt<1, T>
{
    static T det(const Matrix<1, 1, T>& src) { return src[0][0]; }
};

/////////////////////////////////////////////////////////////////////////////////
// Matrix Declaration

template <size_t DIM_ROW, size_t DIM_COL, typename T>
class Matrix
{
public:
    Matrix() = default;

    explicit Matrix(T val)  // if val = 1, it is identity matrix
    {
        for (size_t i = 0; i < DIM_ROW; ++i)
            rows_[i][i] = val;
    }

    // Overload Operators
    Vector<DIM_COL, T>& operator[](const size_t idx)
    {
        DCHECK(idx >= 0 && idx < DIM_ROW);
        return rows_[idx];
    }

    const Vector<DIM_COL, T>& operator[](const size_t idx) const
    {
        DCHECK(idx >= 0 && idx < DIM_ROW);
        return rows_[idx];
    }

    Matrix operator+() const { return Matrix(*this); }

    Matrix operator-() const
    {
        Matrix ret(*this);
        for (size_t i = 0; i < DIM_ROW; ++i)
            for (size_t j = 0; j < DIM_COL; ++j)
                ret[i][j] = -rows_[i][j];
        return ret;
    }

    // Public Methods
    Vector<DIM_COL, T> Row(const size_t rowIdx) const
    {
        DCHECK(rowIdx >= 0 && rowIdx < DIM_ROW);
        return (*this)[rowIdx];
    }

    Matrix& SetRow(const size_t rowIdx, Vector<DIM_COL, T> v)
    {
        DCHECK(rowIdx >= 0 && rowIdx < DIM_ROW);
        (*this)[rowIdx] = v;
        return *this;
    }

    Vector<DIM_ROW, T> Col(const size_t colIdx) const
    {
        DCHECK(colIdx >= 0 && colIdx < DIM_COL);
        Vector<DIM_ROW, T> ret;
        for (size_t i = 0; i < DIM_ROW; ++i)
            ret[i] = rows_[i][colIdx];
        return ret;
    }

    Matrix& SetCol(size_t colIdx, Vector<DIM_ROW, T> v)
    {
        DCHECK(colIdx >= 0 && colIdx < DIM_COL);
        for (size_t i = 0; i < DIM_ROW; ++i)
            rows_[i][colIdx] = v[i];
        return *this;
    }

    static Matrix<DIM_ROW, DIM_COL, T> Identity()
    {
        Matrix<DIM_ROW, DIM_COL, T> ret;
        for (size_t i = 0; i < DIM_ROW; ++i)
            for (size_t j = 0; j < DIM_COL; ++j)
                ret[i][j] = (i == j);
        return ret;
    }

    T Det() const { return dt<DIM_COL, T>::det(*this); }

    // Returns a matrix without [row] and [col] (like a cross)
    Matrix<DIM_ROW - 1, DIM_COL - 1, T> GetMinor(size_t row, size_t col) const
    {
        Matrix<DIM_ROW - 1, DIM_COL - 1, T> ret;
        for (size_t i = 0; i < DIM_ROW - 1; ++i)
            for (size_t j = 0; j < DIM_COL - 1; ++j)
                ret[i][j] = rows_[i < row ? i : i + 1][j < col ? j : j + 1];
        return ret;
    }

    T Cofactor(size_t row, size_t col) const
    {
        return GetMinor(row, col).Det() * ((row + col) % 2 ? -1 : 1);
    }

    Matrix<DIM_ROW, DIM_COL, T> Adjugate() const
    {
        Matrix<DIM_ROW, DIM_COL, T> ret;
        for (size_t i = 0; i < DIM_ROW; ++i)
            for (size_t j = 0; j < DIM_COL; ++j)
                ret[i][j] = Cofactor(i, j);
        return ret.Transpose();
    }

    Matrix<DIM_ROW, DIM_COL, T> Inverse() const { return Adjugate() / (Float)Det(); }

    bool IsInvertible() const { return Det() != 0; }

    Matrix<DIM_COL, DIM_ROW, T> Transpose() const
    {
        Matrix<DIM_COL, DIM_ROW, T> ret;
        for (size_t i = 0; i < DIM_COL; ++i)
            ret[i] = this->Col(i);
        return ret;
    }

private:
    // Private Data
    Vector<DIM_COL, T> rows_[DIM_ROW];
};

/////////////////////////////////////////////////////////////////////////////////
// Matrix Operators

template <size_t DIM_ROW, size_t DIM_COL, typename T>
Matrix<DIM_ROW, DIM_COL, T> operator+(
    const Matrix<DIM_ROW, DIM_COL, T>& lhs,
    const Matrix<DIM_ROW, DIM_COL, T>& rhs)  // Matrix + Matrix
{
    Matrix<DIM_ROW, DIM_COL, T> ret;
    for (int i = 0; i < DIM_ROW; ++i)
        ret[i] = lhs[i] + rhs[i];
    return ret;
}

template <size_t DIM_ROW, size_t DIM_COL, typename T>
Matrix<DIM_ROW, DIM_COL, T> operator-(
    const Matrix<DIM_ROW, DIM_COL, T>& lhs,
    const Matrix<DIM_ROW, DIM_COL, T>& rhs)  // Matrix - Matrix
{
    Matrix<DIM_ROW, DIM_COL, T> ret;
    for (int i = 0; i < DIM_ROW; ++i)
        ret[i] = lhs[i] - rhs[i];
    return ret;
}

template <size_t DIM_ROW, size_t DIM_COL, typename T>
Vector<DIM_ROW, T> operator*(const Matrix<DIM_ROW, DIM_COL, T>& lhs,
                             const Vector<DIM_COL, T>&          rhs)  // Matrix x Scalar
{
    Vector<DIM_ROW, T> ret;
    for (int i = 0; i < DIM_ROW; ++i)
        ret[i] = Dot(lhs[i], rhs);
    return ret;
}

template <size_t R1, size_t C1, size_t C2, typename T>
Matrix<R1, C2, T> operator*(const Matrix<R1, C1, T>& lhs,
                            const Matrix<C1, C2, T>& rhs)  // Matrix x Matrix
{
    Matrix<R1, C2, T> ret;
    for (size_t i = 0; i < R1; ++i)
        for (size_t j = 0; j < C2; ++j)
            ret[i][j] = Dot(lhs.Row(i), rhs.Col(j));
    return ret;
}

template <size_t DIM_ROW, size_t DIM_COL, typename T, typename U>
Matrix<DIM_ROW, DIM_COL, T> operator*(const Matrix<DIM_ROW, DIM_COL, T>& lhs,
                                      const U& val)  // Matrix x Scalar
{
    Matrix<DIM_ROW, DIM_COL, T> ret;
    for (int i = 0; i < DIM_ROW; ++i)
        ret[i] = lhs[i] * val;
    return ret;
}

template <size_t DIM_ROW, size_t DIM_COL, typename T, typename U>
Matrix<DIM_ROW, DIM_COL, U> operator*(
    const T& val, const Matrix<DIM_ROW, DIM_COL, U>& rhs)  // Scalar x Matrix
{
    return rhs * val;
}

template <size_t DIM_ROW, size_t DIM_COL, typename T, typename U>
Matrix<DIM_ROW, DIM_COL, T> operator/(const Matrix<DIM_ROW, DIM_COL, T>& lhs,
                                      const U& rhs)  // Matrix / Scalar
{
    Matrix<DIM_ROW, DIM_COL, T> ret;
    for (size_t i = 0; i < DIM_ROW; ++i)
        ret[i] = lhs[i] / rhs;
    return ret;
}

template <size_t DIM_ROW, size_t DIM_COL, typename T>
std::ostream& operator<<(std::ostream& out, const Matrix<DIM_ROW, DIM_COL, T>& m)
{
    out << "\n    Mat" << DIM_ROW << "x" << DIM_COL << " [";
    out << m[0] << std::endl;
    for (size_t i = 1; i < DIM_ROW - 1; ++i)
        out << "            " << m[i] << std::endl;
    out << "            " << m[DIM_ROW - 1] << "]";
    return out;
}

/////////////////////////////////////////////////////////////////////////////////

// Vector Inline Functions
template <size_t DIM, typename T, typename U>
inline Vector<DIM, T> operator*(U s, const Vector<DIM, T>& v)
{
    return v * s;
}

template <size_t DIM, typename T>
inline Vector<DIM, T> Abs(const Vector<DIM, T>& v)
{
    DCHECK(!v.HasNaNs());
    Vector<DIM, T> ret;
    for (size_t i = 0; i < DIM; ++i)
        ret[i] = std::abs(v[i]);
    return ret;
}

template <size_t DIM, typename T>
inline Float Dot(const Vector<DIM, T>& v1, const Vector<DIM, T>& v2)
{
    // DCHECK(!v1.HasNaNs() && !v2.HasNaNs());
    Float ret = 0.f;
    for (size_t i = 0; i < DIM; ++i)
        ret += v1[i] * v2[i];
    return ret;
}

template <typename T>
inline Vector<3, T> Cross(const Vector<3, T>& v1, const Vector<3, T>& v2)
{
    DCHECK(!v1.HasNaNs() && !v2.HasNaNs());
    return Vector<3, T>(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z,
                        v1.x * v2.y - v1.y * v2.x);
}

template <typename T>
inline Float Cross2(const Vector<2, T>& v1, const Vector<2, T>& v2)
{
    DCHECK(!v1.HasNaNs() && !v2.HasNaNs());
    return v1.x * v2.y - v1.y * v2.x;
}

template <size_t DIM, typename T>
inline Vector<DIM, T> Normalize(const Vector<DIM, T>& v, T l = 1)
{
    return v * l / v.Length();
}

template <typename T>
inline Vector<4, T> DivideByW(const Vector<4, T>& v)
{
    DCHECK_NE(v.w, 0);
    Float        inv = (Float)1 / v.w;
    Vector<4, T> ret(v);
    ret.x *= inv;
    ret.y *= inv;
    ret.z *= inv;
    ret.w *= inv;
    return ret;
}

// Utility Inline Functions
inline Float Lerp(Float t, Float v1, Float v2)
{
    return (1 - t) * v1 + t * v2;
}

template <size_t DIM, typename T>
inline Vector<DIM, T> Lerp(Float t, const Vector<DIM, T>& v1, const Vector<DIM, T>& v2)
{
    return (1 - t) * v1 + t * v2;
}

template <typename T>
inline T Clamp(T val, T min, T max)
{
    return std::min(max, std::max(val, min));
}

template <size_t DIM, typename T>
inline Vector<DIM, T> Clamp(const Vector<DIM, T>& val, T min, T max)
{
    Vector<DIM, T> ret;
    for (size_t i = 0; i < DIM; ++i)
        ret[i] = Clamp(val[i], min, max);
    return ret;
}

inline Float Radians(Float deg)
{
    return deg * Pi / 180.0f;
}

inline Float Degrees(Float rad)
{
    return rad * 180.0f / Pi;
}

template <typename T>
inline T Min(const T& v1, const T& v2)
{
    return std::min(v1, v2);
}

template <typename T>
inline T Min3(const T& v1, const T& v2, const T& v3)
{
    return std::min(v1, std::min(v2, v3));
}

template <typename T>
inline T Max(const T& v1, const T& v2)
{
    return std::max(v1, v2);
}

template <typename T>
inline T Max3(const T& v1, const T& v2, const T& v3)
{
    return std::max(v1, std::max(v2, v3));
}

/////////////////////////////////////////////////////////////////////////////////
// Vector & Matrix Typedef

// vec
typedef Vector<2, Float> Vec2f;
typedef Vector<2, int>   Vec2i;
typedef Vector<3, Float> Vec3f;
typedef Vector<3, int>   Vec3i;
typedef Vector<4, Float> Vec4f;
typedef Vector<4, int>   Vec4i;
// square
typedef Matrix<2, 2, Float> Mat2x2f, Mat2f;
typedef Matrix<3, 3, Float> Mat3x3f, Mat3f;
typedef Matrix<4, 4, Float> Mat4x4f, Mat4f;
// non-square
typedef Matrix<2, 3, Float> Mat2x3f;
typedef Matrix<3, 2, Float> Mat3x2f;
typedef Matrix<3, 4, Float> Mat3x4f;
typedef Matrix<4, 3, Float> Mat4x3f;

/////////////////////////////////////////////////////////////////////////////////

bool TestInsideTriangle(const Vec2f& A, const Vec2f& B, const Vec2f& C, const Vec2f& P);

Float TriangleArea(Vec2f a, Vec2f b, Vec2f c);

Vec3f Barycentric(const Vec2i& A, const Vec2i& B, const Vec2i& C, const Vec2i& P);
Vec3f Barycentric(const Vec2f& A, const Vec2f& B, const Vec2f& C, const Vec2f& P);

Mat3f MakeNormalMatrix(const Mat4f& matrix);

Mat3f MakeTbnMatrix(const Vec3f& edge1, const Vec3f& edge2, const Vec2f& deltaUv1,
                    const Vec2f& deltaUv2, const Vec3f& N);

Mat4f MakeModelMatrix(const Vec3f& translation, Float yRotateDegree = 0.f,
                      Float scale = 1.f);

Mat4f MakeLookAtMatrix(const Vec3f& eyePos, const Vec3f& center,
                       const Vec3f& worldUp = Vec3f(0.f, 1.f, 0.f));

Mat4f MakePerspectiveMatrix(Float fov, Float aspectRatio, Float n, Float f);

Mat4f MakePerspectiveMatrix(Float l, Float r, Float b, Float t, Float n, Float f);

Mat4f MakeOrthographicMatrix(Float l, Float r, Float b, Float t, Float n, Float f);

#endif  //_GEOMETRY_H_
