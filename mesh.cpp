//
// Created by Junhao Wang (@Forkercat) on 2021/1/6.
//

#include "mesh.h"

#include "forkergl.h"
#include "model.h"
#include "shader.h"

// Copy Constructor
Mesh::Mesh(const Mesh& m)
    : model(m.model),
      material(m.material),
      faceVertIndices(m.faceVertIndices),
      faceTexCoordIndices(m.faceTexCoordIndices),
      faceNormalIndices(m.faceNormalIndices)
{
}

void Mesh::Draw(Shader& shader) const
{
    // For each face
    for (int f = 0; f < NumFaces(); ++f)
    {
        // Use Shader To Set Mesh Pointer
        shader.Use(this);

        Vec4f ndcCoords[3];
        for (int v = 0; v < 3; ++v)  // for each vertex
        {
            ndcCoords[v] = shader.ProcessVertex(f, v);
        }
        ForkerGL::DrawTriangle(ndcCoords, shader);
    }
}

/////////////////////////////////////////////////////////////////////////////////

int Mesh::NumFaces() const
{
    return (int)faceVertIndices.size() / 3;
}

Vec3f Mesh::Vert(int faceIdx, int vertIdx) const
{
    int index = faceVertIndices[faceIdx * 3 + vertIdx];
    return model->GetVert(index);
}

Vec2f Mesh::TexCoord(int faceIdx, int vertIdx) const
{
    int index = faceTexCoordIndices[faceIdx * 3 + vertIdx];
    return model->GetTexCoord(index);
}

Vec3f Mesh::Normal(int faceIdx, int vertIdx) const
{
    int index = faceNormalIndices[faceIdx * 3 + vertIdx];
    return Normalize(model->GetNormal(index));
}

/////////////////////////////////////////////////////////////////////////////////

Vec3f Mesh::DiffuseColor(const Vec2f& uv) const
{
    const TGAImage& diffuseMap = material->DiffuseMap;
    int             w = diffuseMap.GetWidth() - 1;
    int             h = diffuseMap.GetHeight() - 1;
    const TGAColor& color = diffuseMap.Get(uv.s * w, uv.t * h);
    return Vec3f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f);
}

Float Mesh::SpecularIntensity(const Vec2f& uv) const
{
    const TGAImage& specularMap = material->SpecularMap;
    int             w = specularMap.GetWidth() - 1;
    int             h = specularMap.GetHeight() - 1;
    return specularMap.Get(uv.s * w, uv.t * h)[0] / 255.0f;
}

Float Mesh::SpecularShininess(const Vec2f& uv) const
{
    const TGAImage& specularMap = material->SpecularMap;
    int             w = specularMap.GetWidth() - 1;
    int             h = specularMap.GetHeight() - 1;
    return specularMap.Get(uv.s * w, uv.t * h)[0] / 1.f;
}

Float Mesh::AmbientOcclusionIntensity(const Vec2f& uv) const
{
    const TGAImage& aoMap = material->AmbientOcclusionMap;
    int             w = aoMap.GetWidth() - 1;
    int             h = aoMap.GetHeight() - 1;
    return aoMap.Get(uv.s * w, uv.t * h)[0] / 255.0f;
}

Vec3f Mesh::Normal(const Vec2f& uv) const
{
    const TGAImage& normalMap = material->NormalMap;
    int             w = normalMap.GetWidth() - 1;
    int             h = normalMap.GetHeight() - 1;
    const TGAColor& color = normalMap.Get(uv.s * w, uv.t * h);
    Vec3f           normal(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f);
    normal = normal * 2.f - Vec3f(1.f);  // from [0, 1] to [-1, 1]
    return Normalize(normal);
}

/////////////////////////////////////////////////////////////////////////////////

void Mesh::AddVertIndex(int index)
{
    faceVertIndices.push_back(index);
}

void Mesh::AddTexCoordIndex(int index)
{
    faceTexCoordIndices.push_back(index);
}

void Mesh::AddNormalIndex(int index)
{
    faceNormalIndices.push_back(index);
}
