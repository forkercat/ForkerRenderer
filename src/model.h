//
// Created by Junhao Wang (@Forkercat) on 2020/12/18.
//

#pragma once

#include "geometry.h"
#include "material.h"
#include "mesh.h"
#include "pbrmaterial.h"
#include "texture.h"

class Shader;

class Model
{
public:
    // Public Static Methods
    static std::unique_ptr<Model> Load(const std::string& filename,
                                       bool               normalized = false,
                                       bool               generateTangent = false,
                                       bool               flipTexCoordY = true);

    // Constructors
    Model() : m_Meshes(), m_Verts(), m_TexCoords(), m_Normals(), m_HasTangents() { }
    explicit Model(const Model& m) = delete;

    // Starts Rendering This Fun Stuff!
    void Render(Shader& shader) const;

    // Get Vertex Data
    Vector3f GetVert(int index) const { return m_Verts[index]; }
    Vector2f GetTexCoord(int index) const { return m_TexCoords[index]; }
    Vector3f GetNormal(int index) const { return m_Normals[index]; }
    Vector3f GetTangent(int index) const { return m_Tangents[index]; }

    int GetNumVerts() const { return (int)m_Verts.size(); }
    int GetNumFaces() const;

    inline bool HasTangents() const { return m_HasTangents; }
    inline bool SupportPBR() const { return m_SupportPBR; }

private:
    std::map<std::string, std::shared_ptr<Mesh>>        m_Meshes;
    std::map<std::string, std::shared_ptr<Material>>    m_Materials;
    std::map<std::string, std::shared_ptr<PBRMaterial>> m_PBRMaterials;

    std::vector<Vector3f> m_Verts;
    std::vector<Vector2f> m_TexCoords;
    std::vector<Vector3f> m_Normals;
    std::vector<Vector3f> m_Tangents;
    bool                  m_HasTangents;
    bool                  m_SupportPBR;

    // .obj and .mtl Parsers
    // Supported Format: 'g ' is followed by 'usemtl ', which is followed by 'f ...'
    bool loadObjectFile(const std::string& filename, bool flipVertically);
    void loadMaterials(const std::string& directory, const std::string& filename,
                       bool flipVertically);
    void loadTexture(const std::string&        textureFilename,
                     std::shared_ptr<Texture>& texture, bool flipVertically);

    // Make Position Coordinates Between [-1, 1]
    void normalizePositionVertices();
    // Generate Tangents For TBN Matrix Transformation
    void generateTangents();
};
