//
// Created by Junhao Wang (@Forkercat) on 2020/12/18.
//

#ifndef _MODEL_H_
#define _MODEL_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "geometry.h"
#include "material.h"
#include "mesh.h"
#include "pbrmaterial.h"
#include "texture.h"

class Shader;

class Model : public std::enable_shared_from_this<Model>
{
public:
    // Public Static Methods
    static std::shared_ptr<Model> Load(const std::string& filename,
                                       bool               normalized = false,
                                       bool               generateTangent = false,
                                       bool               flipTexCoordY = true);

    // Constructors
    Model() : m_Meshes(), m_Verts(), m_TexCoords(), m_Normals(), m_HasTangents() { }
    Model(const Model& m);

    // Starts Rendering This Fun Stuff!
    void Render(Shader& shader);

    // Get Vertex Data
    Vector3f GetVert(int index) const;
    Vector2f GetTexCoord(int index) const;
    Vector3f GetNormal(int index) const;
    Vector3f GetTangent(int index) const;

    int GetNumVerts() const;
    int GetNumFaces() const;

    inline bool HasTangents() const { return m_HasTangents; }
    inline bool SupportPBR() const { return m_SupportPBR; }

private:
    std::map<std::string, std::shared_ptr<Mesh>>     m_Meshes;
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

#endif  //_MODEL_H_
