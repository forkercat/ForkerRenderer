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

        Point4f ndcCoords[3];
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

Vector3f Mesh::Vert(int faceIdx, int vertIdx) const
{
    int index = faceVertIndices[faceIdx * 3 + vertIdx];
    return model->GetVert(index);
}

Vector2f Mesh::TexCoord(int faceIdx, int vertIdx) const
{
    int index = faceTexCoordIndices[faceIdx * 3 + vertIdx];
    return model->GetTexCoord(index);
}

Vector3f Mesh::Normal(int faceIdx, int vertIdx) const
{
    int index = faceNormalIndices[faceIdx * 3 + vertIdx];
    return Normalize(model->GetNormal(index));
}

Vector3f Mesh::Tangent(int faceIdx, int vertIdx) const
{
    int index = faceTangentIndices[faceIdx * 3 + vertIdx];
    return Normalize(model->GetTangent(index));
}

int Mesh::GetVertIndex(int faceIdx, int vertIdx) const
{
    return faceVertIndices[faceIdx * 3 + vertIdx];
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

void Mesh::AddTangentIndex(int index)
{
    faceTangentIndices.push_back(index);
}
