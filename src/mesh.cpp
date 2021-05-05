//
// Created by Junhao Wang (@Forkercat) on 2021/1/6.
//

#include "mesh.h"

#include "forkergl.h"
#include "model.h"
#include "shader.h"

// Copy Constructor
Mesh::Mesh(const Mesh& m)
    : m_Model(m.m_Model),
      m_Material(m.m_Material),
      m_PBRMaterial(m.m_PBRMaterial),
      m_FaceVertIndices(m.m_FaceVertIndices),
      m_FaceTexCoordIndices(m.m_FaceTexCoordIndices),
      m_FaceNormalIndices(m.m_FaceNormalIndices)
{
}

void Mesh::Draw(Shader& shader) const
{
    // For each face
    for (int f = 0; f < NumFaces(); ++f)
    {
        // Use Shader To Set Mesh Pointer
        shader.Use(shared_from_this());

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
    return (int)m_FaceVertIndices.size() / 3;
}

Vector3f Mesh::Vert(int faceIdx, int vertIdx) const
{
    int index = m_FaceVertIndices[faceIdx * 3 + vertIdx];
    return GetModel()->GetVert(index);
}

Vector2f Mesh::TexCoord(int faceIdx, int vertIdx) const
{
    int index = m_FaceTexCoordIndices[faceIdx * 3 + vertIdx];
    return GetModel()->GetTexCoord(index);
}

Vector3f Mesh::Normal(int faceIdx, int vertIdx) const
{
    int index = m_FaceNormalIndices[faceIdx * 3 + vertIdx];
    return Normalize(GetModel()->GetNormal(index));
}

Vector3f Mesh::Tangent(int faceIdx, int vertIdx) const
{
    int index = m_FaceTangentIndices[faceIdx * 3 + vertIdx];
    return Normalize(GetModel()->GetTangent(index));
}

int Mesh::GetVertIndex(int faceIdx, int vertIdx) const
{
    return m_FaceVertIndices[faceIdx * 3 + vertIdx];
}

/////////////////////////////////////////////////////////////////////////////////

void Mesh::AddVertIndex(int index)
{
    m_FaceVertIndices.push_back(index);
}

void Mesh::AddTexCoordIndex(int index)
{
    m_FaceTexCoordIndices.push_back(index);
}

void Mesh::AddNormalIndex(int index)
{
    m_FaceNormalIndices.push_back(index);
}

void Mesh::AddTangentIndex(int index)
{
    m_FaceTangentIndices.push_back(index);
}
