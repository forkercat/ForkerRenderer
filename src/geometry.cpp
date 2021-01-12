//
// Created by Junhao Wang (@Forkercat) on 2020/12/21.
//

#include "geometry.h"

bool TestInsideTriangle(const Vec2f& A, const Vec2f& B, const Vec2f& C, const Vec2f& P)
{
    Float a = Cross2(A - C, P - C);
    Float b = Cross2(B - A, P - A);
    Float c = Cross2(C - B, P - B);
    return (a >= 0 && b >= 0 && c >= 0) || (a <= 0 && b <= 0 && c <= 0);
}

Float TriangleArea(Vec2f a, Vec2f b, Vec2f c)
{
    return 0.5f * ((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y));
}

Vec3f Barycentric(const Vec2i& A, const Vec2i& B, const Vec2i& C, const Vec2i& P)
{
    Vec2f a(A.x, A.y);
    Vec2f b(B.x, B.y);
    Vec2f c(C.x, C.y);
    Vec2f p(P.x, P.y);
    return Barycentric(a, b, c, p);
}

Vec3f Barycentric(const Vec2f& A, const Vec2f& B, const Vec2f& C, const Vec2f& P)
{
    Vector<3, double> s[2];
    for (int i = 0; i < 2; ++i)
    {
        s[i].x = B[i] - A[i];
        s[i].y = C[i] - A[i];
        s[i].z = A[i] - P[i];
    }

    Vector<3, double> bary = Cross(s[0], s[1]);
    if (std::abs(bary.z) > 1e-2)
    {
        // Do not use /= operator as it continues using Float
        double inv = 1.f / bary.z;
        bary.x *= inv;
        bary.y *= inv;  // from [bx, by, bz] to [u, v, 1]

        Vec3f result(1.f - (bary.x + bary.y), bary.x, bary.y);  // good

        if (result.x < 0.f || result.y < 0.f || result.z < 0.f) return Vec3f(-1, -1, -1);

        return result;
    }

    // Cannot form a triangle
    return Vec3f(-1, -1, -1);
}

/////////////////////////////////////////////////////////////////////////////////

Mat3f MakeNormalMatrix(const Mat4f& matrix)
{
    Mat4f temp = matrix.Inverse().Transpose();
    Mat3f m;
    m.SetRow(0, temp[0].xyz);
    m.SetRow(1, temp[1].xyz);
    m.SetRow(2, temp[2].xyz);
    return m;
}

// Deprecated (Tangents have been calculated in Model class)
Mat3f MakeTbnMatrix(const Vec3f& edge1, const Vec3f& edge2, const Vec2f& deltaUv1,
                    const Vec2f& deltaUv2, const Vec3f& N)
{
    Float det = deltaUv1.s * deltaUv2.t - deltaUv2.s * deltaUv1.t;
    if (det == 0.f)
    {
        return Mat3f(1.f);
    }
    Float inv = 1.f / det;
    Vec3f T = Normalize(inv * Vec3f(deltaUv2.t * edge1.x - deltaUv1.t * edge2.x,
                                    deltaUv2.t * edge1.y - deltaUv1.t * edge2.y,
                                    deltaUv2.t * edge1.z - deltaUv1.t * edge2.z));
    // Re-orthogonalize T with respect to N
    T = Normalize(T - Dot(T, N) * N);
    Vec3f B = Normalize(Cross(N, T));

    Mat3f TBN;
    TBN.SetCol(0, T).SetCol(1, B).SetCol(2, N);
    return TBN;
}

Mat4f MakeModelMatrix(const Vec3f& translation, Float yRotateDegree, Float scale)
{
    Mat4f S(1.f);
    S[0][0] = S[1][1] = S[2][2] = scale;

    Mat4f R(1.f);
    Float radVal = Radians(yRotateDegree);
    R[0][0] = cos(radVal);
    R[0][2] = sin(radVal);
    R[2][0] = -sin(radVal);
    R[2][2] = cos(radVal);

    Mat4f T(1.f);
    T.SetCol(3, Vec4f(translation, 1.f));

    return T * R * S;
}

Mat4f MakeLookAtMatrix(const Vec3f& eyePos, const Vec3f& center, const Vec3f& worldUp)
{
    CHECK(eyePos != center);
    Vec3f front = Normalize(center - eyePos);

    CHECK(front != Vec3f(0.f, -1.f, 0.f));  // cannot do cross with (0, 1, 0) worldUp
    Vec3f right = Normalize(Cross(front, worldUp));
    Vec3f up = Normalize(Cross(right, front));

    Mat4f R(1.f);
    R.SetRow(0, Vec4f(right, 0.f));
    R.SetRow(1, Vec4f(up, 0.f));
    R.SetRow(2, Vec4f(-front, 0.f));

    Mat4f T(1.f);
    T.SetCol(3, Vec4f(-eyePos, 1.f));

    return R * T;
}

Mat4f MakePerspectiveMatrix(Float fov, Float aspectRatio, Float n, Float f)
{
    Float tanFovOver2 = std::tan(Radians(fov / 2.0f));
    Mat4f m(1.f);
    // for x
    m[0][0] = 1.f / (aspectRatio * tanFovOver2);
    // for y
    m[1][1] = 1.f / tanFovOver2;
    // for z
    m[2][2] = -(f + n) / (f - n);
    m[2][3] = -2 * f * n / (f - n);
    // for w
    m[3][2] = -1;

    return m;
}

Mat4f MakePerspectiveMatrix(Float l, Float r, Float b, Float t, Float n, Float f)
{
    Mat4f m(1.f);

    // for x
    m[0][0] = 2 * n / (r - l);
    m[0][2] = (r + l) / (r - l);
    // for y
    m[1][1] = 2 * n / (t - b);
    m[1][2] = (t + b) / (t - b);
    // for z
    m[2][2] = -(f + n) / (f - n);
    m[2][3] = -2 * f * n / (f - n);
    // for w
    m[3][2] = -1;

    return m;
}

Mat4f MakeOrthographicMatrix(Float l, Float r, Float b, Float t, Float n, Float f)
{
    Mat4f m(1.f);

    m[0][0] = 2.0f / (r - l);
    m[1][1] = 2.0f / (t - b);
    m[2][2] = -2.0f / (f - n);
    m[0][3] = -(r + l) / (r - l);
    m[1][3] = -(t + b) / (t - b);
    m[2][3] = -(f + n) / (f - n);
    m[3][3] = 1.f;

    return m;
}