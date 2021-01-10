//
// Created by Junhao Wang (@Forkercat) on 2021/1/6.
//

#ifndef _MESH_H_
#define _MESH_H_

#include "geometry.h"
#include "material.h"
#include "tgaimage.h"

class Shader;
class Model;

class Mesh
{
public:
    Mesh() = default;

    // Copy Constructor
    Mesh(const Mesh& m);

    // Model will call it in its Render()
    void Draw(Shader& shader) const;

    // Vertex Properties
    int   NumFaces() const;
    Vec3f Vert(int faceIdx, int vertIdx) const;
    Vec2f TexCoord(int faceIdx, int vertIdx) const;
    Vec3f Normal(int faceIdx, int vertIdx) const;

    // Material Properties
    Vec3f GetKa() const { return material->Ka; }
    Vec3f GetKd() const { return material->Kd; }
    Vec3f GetKs() const { return material->Ks; }
    Vec3f GetKe() const { return material->Ke; }

    Vec3f DiffuseColor(const Vec2f& uv) const;
    Float SpecularIntensity(const Vec2f& uv) const;
    Float SpecularShininess(const Vec2f& uv) const;
    Float AmbientOcclusionIntensity(const Vec2f& uv) const;
    Vec3f Normal(const Vec2f& uv) const;

    // Helper
    const Model*    GetModel() const { return model; }
    void            SetModel(const Model* m) { model = m; };
    const Material* GetMaterial() const { return material; };
    void            SetMaterial(const Material* m) { material = m; }
    void            AddVertIndex(int index);
    void            AddTexCoordIndex(int index);
    void            AddNormalIndex(int index);

private:
    // Model and material are set right after Mesh creation
    const Model*    model;
    const Material* material;

    // Stores indices of actual vertices in Model class
    std::vector<int> faceVertIndices;
    std::vector<int> faceTexCoordIndices;
    std::vector<int> faceNormalIndices;  // 3 vertices form a triangle
};

#endif  // _MESH_H_
