//
// Created by Junhao Wang (@Forkercat) on 2021/1/6.
//

#ifndef _MESH_H_
#define _MESH_H_

#include <memory>

#include "color.h"
#include "geometry.h"
#include "material.h"
#include "tgaimage.h"

class Shader;
class Model;

class Mesh : public std::enable_shared_from_this<Mesh>
{
public:
    Mesh(std::shared_ptr<const Model> m) : model(m) { }

    // Copy Constructor
    Mesh(const Mesh& m);

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
    std::shared_ptr<const Model>    GetModel() const { return model.lock(); }
    std::shared_ptr<const Material> GetMaterial() const { return material.lock(); };

    void SetModel(std::shared_ptr<const Model> m) { model = m; };
    void SetMaterial(std::shared_ptr<const Material> m) { material = m; }

    void AddVertIndex(int index);
    void AddTexCoordIndex(int index);
    void AddNormalIndex(int index);
    void AddTangentIndex(int index);

private:
    // Model and material are set right after Mesh creation
    std::weak_ptr<const Model>    model;
    std::weak_ptr<const Material> material;

    // Stores indices of actual vertices in Model class
    std::vector<int> faceVertIndices;
    std::vector<int> faceTexCoordIndices;
    std::vector<int> faceNormalIndices;  // 3 vertices form a triangle
    std::vector<int> faceTangentIndices;
};

#endif  // _MESH_H_
