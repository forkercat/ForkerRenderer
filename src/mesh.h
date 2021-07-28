//
// Created by Junhao Wang (@Forkercat) on 2021/1/6.
//

#pragma once

#include "color.h"
#include "geometry.h"
#include "material.h"
#include "pbrmaterial.h"
#include "tgaimage.h"

class Shader;
class Model;

class Mesh : public std::enable_shared_from_this<Mesh>
{
public:
    Mesh(const Model& model) : m_Model(model) { }

    // Copy Constructor
    explicit Mesh(const Mesh& m) = delete;

    // Model will call it in its Render()
    void Draw(Shader& shader) const;

    // Vertex Properties
    int      NumFaces() const;
    Vector3f Vert(int faceIdx, int vertIdx) const;
    Vector2f TexCoord(int faceIdx, int vertIdx) const;
    Vector3f Normal(int faceIdx, int vertIdx) const;
    Vector3f Tangent(int faceIdx, int vertIdx) const;
    int      GetVertIndex(int faceIdx, int vertIdx) const;

    // Helper
    const Model&                       GetModel() const { return m_Model; }
    std::shared_ptr<const Material>    GetMaterial() const { return m_Material.lock(); };
    std::shared_ptr<const PBRMaterial> GetPBRMaterial() const
    {
        return m_PBRMaterial.lock();
    }

    void SetMaterial(std::shared_ptr<const Material> m) { m_Material = m; }
    void SetPBRMaterial(std::shared_ptr<const PBRMaterial> m) { m_PBRMaterial = m; }

    void AddVertIndex(int index);
    void AddTexCoordIndex(int index);
    void AddNormalIndex(int index);
    void AddTangentIndex(int index);

private:
    // Model and material are set right after Mesh creation
    const Model&                     m_Model;
    std::weak_ptr<const Material>    m_Material;
    std::weak_ptr<const PBRMaterial> m_PBRMaterial;

    // Stores indices of actual vertices in Model class
    std::vector<int> m_FaceVertIndices;
    std::vector<int> m_FaceTexCoordIndices;
    std::vector<int> m_FaceNormalIndices;  // 3 vertices form a triangle
    std::vector<int> m_FaceTangentIndices;
};
