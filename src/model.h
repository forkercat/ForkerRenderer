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
#include "texture.h"

class Shader;

class Model : public std::enable_shared_from_this<Model>
{
public:
    Model() : meshes(), verts(), texCoords(), normals(), hasTangents() { }

    // Copy Constructor
    Model(const Model& m);

    // Load Object File
    std::shared_ptr<Model> Load(const std::string& filename, bool normalized = false,
              bool generateTangent = false, bool flipTexCoordY = true);

    // Starts Rendering This Fun Stuff!
    void Render(Shader& shader);

    // Get Vertex Data
    Vector3f GetVert(int index) const;
    Vector2f GetTexCoord(int index) const;
    Vector3f GetNormal(int index) const;
    Vector3f GetTangent(int index) const;

    int GetNumVerts() const;
    int GetNumFaces() const;

    inline bool HasTangents() const { return hasTangents; }

private:
    std::map<std::string, std::shared_ptr<Mesh>>     meshes;
    std::map<std::string, std::shared_ptr<Material>> materials;

    std::vector<Vector3f> verts;
    std::vector<Vector2f> texCoords;
    std::vector<Vector3f> normals;
    std::vector<Vector3f> tangents;
    bool                  hasTangents;

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
