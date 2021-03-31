//
// Created by Junhao Wang (@Forkercat) on 2020/12/21.
//

#include "geometry.h"

bool TestInsideTriangle(const Vector2f& A, const Vector2f& B, const Vector2f& C, const Vector2f& P)
{
    Float a = Cross2(A - C, P - C);
    Float b = Cross2(B - A, P - A);
    Float c = Cross2(C - B, P - B);
    return (a >= 0 && b >= 0 && c >= 0) || (a <= 0 && b <= 0 && c <= 0);
}

Float TriangleArea(Vector2f a, Vector2f b, Vector2f c)
{
    return 0.5f * ((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y));
}

Vector3f Barycentric(const Vector2i& A, const Vector2i& B, const Vector2i& C, const Vector2i& P)
{
    Vector2f a(A.x, A.y);
    Vector2f b(B.x, B.y);
    Vector2f c(C.x, C.y);
    Vector2f p(P.x, P.y);
    return Barycentric(a, b, c, p);
}

Vector3f Barycentric(const Vector2f& A, const Vector2f& B, const Vector2f& C, const Vector2f& P)
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

        Vector3f result(1.f - (bary.x + bary.y), bary.x, bary.y);  // good

        if (result.x < 0.f || result.y < 0.f || result.z < 0.f) return Vector3f(-1, -1, -1);

        return result;
    }

    // Cannot form a triangle
    return Vector3f(-1, -1, -1);
}

/////////////////////////////////////////////////////////////////////////////////

Matrix3f MakeNormalMatrix(const Matrix4f& matrix)
{
    Matrix4f temp = matrix.Inverse().Transpose();
    Matrix3f m;
    m.SetRow(0, temp[0].xyz);
    m.SetRow(1, temp[1].xyz);
    m.SetRow(2, temp[2].xyz);
    return m;
}

// Deprecated (Tangents have been calculated in Model class)
Matrix3f MakeTbnMatrix(const Vector3f& edge1, const Vector3f& edge2, const Vector2f& deltaUv1,
                    const Vector2f& deltaUv2, const Vector3f& N)
{
    Float det = deltaUv1.s * deltaUv2.t - deltaUv2.s * deltaUv1.t;
    if (det == 0.f)
    {
        return Matrix3f(1.f);
    }
    Float inv = 1.f / det;
    Vector3f T = Normalize(inv * Vector3f(deltaUv2.t * edge1.x - deltaUv1.t * edge2.x,
                                    deltaUv2.t * edge1.y - deltaUv1.t * edge2.y,
                                    deltaUv2.t * edge1.z - deltaUv1.t * edge2.z));
    // Re-orthogonalize T with respect to N
    T = Normalize(T - Dot(T, N) * N);
    Vector3f B = Normalize(Cross(N, T));

    Matrix3f TBN;
    TBN.SetCol(0, T).SetCol(1, B).SetCol(2, N);
    return TBN;
}

Matrix4f MakeModelMatrix(const Vector3f& translation, Float yRotateDegree, Float scale)
{
    Matrix4f S(1.f);
    S[0][0] = S[1][1] = S[2][2] = scale;

    Matrix4f R(1.f);
    Float radVal = Radians(yRotateDegree);
    R[0][0] = cos(radVal);
    R[0][2] = sin(radVal);
    R[2][0] = -sin(radVal);
    R[2][2] = cos(radVal);

    Matrix4f T(1.f);
    T.SetCol(3, Vector4f(translation, 1.f));

    return T * R * S;
}

Matrix4f MakeLookAtMatrix(const Vector3f& eyePos, const Vector3f& center, const Vector3f& worldUp)
{
    CHECK(eyePos != center);
    Vector3f front = Normalize(center - eyePos);

    CHECK(front != Vector3f(0.f, -1.f, 0.f));  // cannot do cross with (0, 1, 0) worldUp
    Vector3f right = Normalize(Cross(front, worldUp));
    Vector3f up = Normalize(Cross(right, front));

    Matrix4f R(1.f);
    R.SetRow(0, Vector4f(right, 0.f));
    R.SetRow(1, Vector4f(up, 0.f));
    R.SetRow(2, Vector4f(-front, 0.f));

    Matrix4f T(1.f);
    T.SetCol(3, Vector4f(-eyePos, 1.f));

    return R * T;
}

Matrix4f MakePerspectiveMatrix(Float fov, Float aspectRatio, Float n, Float f)
{
    Float tanFovOver2 = std::tan(Radians(fov / 2.0f));
    Matrix4f m(1.f);
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

Matrix4f MakePerspectiveMatrix(Float l, Float r, Float b, Float t, Float n, Float f)
{
    Matrix4f m(1.f);

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

Matrix4f MakeOrthographicMatrix(Float l, Float r, Float b, Float t, Float n, Float f)
{
    Matrix4f m(1.f);

    m[0][0] = 2.0f / (r - l);
    m[1][1] = 2.0f / (t - b);
    m[2][2] = -2.0f / (f - n);
    m[0][3] = -(r + l) / (r - l);
    m[1][3] = -(t + b) / (t - b);
    m[2][3] = -(f + n) / (f - n);
    m[3][3] = 1.f;

    return m;
}