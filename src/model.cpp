//
// Created by Junhao Wang (@Forkercat) on 2020/12/18.
//

#include "model.h"

#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>

#include <fstream>
#include <sstream>
#include <string>

#include "shader.h"

const std::string WHITESPACE = " \n\r\t\f\v";

std::string ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

/////////////////////////////////////////////////////////////////////////////////

Model::Model(const std::string& filename, bool normalized, bool flipTexCoordY)
    : meshes(), verts(), texCoords(), normals()
{
    spdlog::info("Model File: {}", filename);

    // Load Object, Material, Texture Files
    bool success = loadObjectFile(filename, flipTexCoordY);

    if (!success)
    {
        spdlog::error("Failed to load model");
        return;
    }

    // Post-Processing
    generateTangents();
    if (normalized) this->normalizePositionVertices();

    // clang-format off
    spdlog::info(
        "v# {}, f# {}, vt# {}, vn# {}, tg# {}, mesh# {}, mtl# {} | normalized: {}, flipTexCoordY: {}",
        GetNumVerts(), GetNumFaces(), texCoords.size(), normals.size(), tangents.size(), meshes.size(),
        materials.size(), normalized, flipTexCoordY);

    for (auto iter = meshes.begin(); iter != meshes.end(); ++iter)
    {
        const Mesh& mesh = iter->second;
        spdlog::info("[{:>8}] f# {:>5} | {}",
                     iter->first, mesh.NumFaces(), *mesh.GetMaterial());
    }
    // clang-format on

    spdlog::info("------------------------------------------------------------");
}

// Copy Constructor
Model::Model(const Model& m)
    : meshes(m.meshes),
      materials(m.materials),
      verts(m.verts),
      texCoords(m.texCoords),
      normals(m.normals)
{
    // Update Mesh's pointers
    for (auto iter = meshes.begin(); iter != meshes.end(); ++iter)
    {
        Mesh& mesh = iter->second;
        mesh.SetModel(this);
        mesh.SetMaterial(&(this->materials[mesh.GetMaterial()->Name]));
    }
}

/////////////////////////////////////////////////////////////////////////////////

void Model::Render(Shader& shader)
{
    spdlog::stopwatch stopwatch;
    // For each mesh
    for (auto iter = meshes.begin(); iter != meshes.end(); ++iter)
    {
        const Mesh& mesh = iter->second;
        mesh.Draw(shader);

        spdlog::info("-- [{}] Time Used: {:.6} Seconds", iter->first, stopwatch);
        stopwatch.reset();
    }
}

/////////////////////////////////////////////////////////////////////////////////

Vec3f Model::GetVert(int index) const
{
    return verts[index];
}

Vec2f Model::GetTexCoord(int index) const
{
    return texCoords[index];
}

Vec3f Model::GetNormal(int index) const
{
    return normals[index];
}

Vec3f Model::GetTangent(int index) const
{
    return tangents[index];
}

int Model::GetNumVerts() const
{
    return (int)verts.size();
}

int Model::GetNumFaces() const
{
    int total = 0;
    for (auto iter = meshes.begin(); iter != meshes.end(); ++iter)
    {
        const auto& mesh = iter->second;
        total += mesh.NumFaces();
    }
    return total;
}

/////////////////////////////////////////////////////////////////////////////////

void Model::normalizePositionVertices()
{
    Float xmin = MaxFloat, xmax = MinFloat;
    Float ymin = MaxFloat, ymax = MinFloat;
    Float zmin = MaxFloat, zmax = MinFloat;

    for (size_t i = 0; i < GetNumVerts(); ++i)
    {
        Vec3f v = verts[i];
        xmin = Min(xmin, v.x), xmax = Max(xmax, v.x);
        ymin = Min(ymin, v.y), ymax = Max(ymax, v.y);
        zmin = Min(zmin, v.z), zmax = Max(zmax, v.z);
    }

    CHECK_NE(xmax - xmin, 0);
    CHECK_NE(ymax - ymin, 0);
    CHECK_NE(zmax - zmin, 0);

    Float scaleFactor = 2.0f / Max(xmax - xmin, Max(ymax - ymin, zmax - zmin));
    Mat4f m(1.f);
    m[0][0] = scaleFactor;
    m[1][1] = scaleFactor;
    m[2][2] = scaleFactor;
    m[0][3] = -(xmax + xmin) / (xmax - xmin);
    m[1][3] = -(ymax + ymin) / (ymax - ymin);
    m[2][3] = -(zmax + zmin) / (zmax - zmin);

    for (size_t i = 0; i < GetNumVerts(); ++i)
    {
        verts[i] = (m * Vec4f(verts[i], 1.f)).xyz;
    }
}

/////////////////////////////////////////////////////////////////////////////////

// Load .obj File
bool Model::loadObjectFile(const std::string& filename, bool flipVertically)
{
    // Load Object File
    std::ifstream in;
    in.open(filename, std::ifstream::in);

    if (in.fail())
    {
        spdlog::error("Failed to open the .obj file");
        return false;
    }

    std::string line;
    std::string meshName;
    std::string materialName;  // current name
    while (!in.eof())          // for each line
    {
        std::getline(in, line);
        line = ltrim(line);
        std::istringstream iss(line.c_str());

        // Trash
        char        chTrash;
        std::string strTrash;

        // Material File
        if (line.compare(0, 7, "mtllib ") == 0)  // mtllib
        {
            std::string mtlFilename;
            iss >> strTrash >> mtlFilename;

            std::string directory;
            size_t      slashPos = filename.find_last_of("/");

            // Format: "horizon.obj" or "...../obj/horizon.obj"
            directory = (slashPos == std::string::npos)
                            ? ""
                            : filename.substr(0, slashPos + 1);  // include "/"

            loadMaterials(directory, mtlFilename, flipVertically);
        }
        // Vertices
        else if (line.compare(0, 2, "v ") == 0)  // v
        {
            iss >> strTrash;  // skip 'v' and ' '
            Vec3f vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            verts.push_back(vertex);
        }
        else if (line.compare(0, 3, "vt ") == 0)  // vt
        {
            iss >> strTrash;  // skip 'vt' and ' '
            Vec2f texCoord;
            iss >> texCoord.x >> texCoord.y;
            Float floatTrash;
            iss >> floatTrash;  // ignore the last value

            // For repeated texture coordinates
            // clang-format off
            // if (texCoord.s < 0.f) texCoord.s += 1.f;
            // if (texCoord.s > 1.f) texCoord.s -= 1.f;
            // if (texCoord.t < 0.f) texCoord.t += 1.f;
            // if (texCoord.t > 1.f) texCoord.t -= 1.f;
            // clang-format on
            texCoords.push_back(texCoord);
        }
        else if (line.compare(0, 3, "vn ") == 0)  // vn
        {
            iss >> strTrash;  // skip 'vn' and ' '

            Vec3f normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        // Change Mesh / Material Status
        // Only supported when there are "g" and "usemtl" keywords (in order)
        else if (line.compare(0, 2, "g ") == 0)  // g
        {
            iss >> chTrash >> meshName;
            meshes[meshName] = Mesh();
            meshes[meshName].SetModel(this);
        }
        else if (line.compare(0, 7, "usemtl ") == 0)  // usemtl
        {
            iss >> strTrash >> materialName;
            meshes[meshName].SetMaterial(&materials[materialName]);
        }
        // Faces
        else if (line.compare(0, 2, "f ") == 0)  // f
        {
            iss >> chTrash;  // skip 'f' and ' '
            std::vector<Vec3i> vertices;
            unsigned int       v, t, n;
            while (iss >> v >> chTrash >> t >> chTrash >> n)  // f 24/1/24 25/2/25 26/3/26
            {
                vertices.push_back(Vec3i(--v, --t, --n));
            }

            Mesh& mesh = meshes[meshName];
            for (int i = 1; i < vertices.size() - 1; ++i)
            {
                // First Vertex
                mesh.AddVertIndex(vertices[0].x);
                mesh.AddTexCoordIndex(vertices[0].y);
                mesh.AddNormalIndex(vertices[0].z);

                // Second Vertex
                mesh.AddVertIndex(vertices[i].x);
                mesh.AddTexCoordIndex(vertices[i].y);
                mesh.AddNormalIndex(vertices[i].z);

                // Third Vertex
                mesh.AddVertIndex(vertices[i + 1].x);
                mesh.AddTexCoordIndex(vertices[i + 1].y);
                mesh.AddNormalIndex(vertices[i + 1].z);
            }
        }
    }
    in.close();
    return true;
}

// Load .mtl File
void Model::loadMaterials(const std::string& directory, const std::string& filename,
                          bool flipVertically)
{
    std::string mtlFilename = directory + filename;

    std::ifstream in;
    in.open(mtlFilename, std::ifstream::in);

    if (in.fail())
    {
        spdlog::error("Cannot not open the .mtl file: {}", mtlFilename);
        return;
    }

    std::string line;
    std::string materialName;  // current name
    while (!in.eof())
    {
        std::getline(in, line);
        line = ltrim(line);
        std::istringstream iss(line.c_str());

        // Trash
        std::string strTrash;

        if (line.compare(0, 7, "newmtl ") == 0)  // newmtl
        {
            iss >> strTrash >> materialName;
            materials[materialName] = Material(materialName);
        }
        // Ka / Kd / Ks / Ke
        else if (line.compare(0, 3, "Ka ") == 0)  // Ka
        {
            iss >> strTrash;  // skip 'Ka' and ' '
            Vec3f floats;
            iss >> floats.x >> floats.y >> floats.z;
            materials[materialName].Ka = floats;
        }
        else if (line.compare(0, 3, "Kd ") == 0)  // Kd
        {
            iss >> strTrash;
            Vec3f floats;
            iss >> floats.x >> floats.y >> floats.z;
            materials[materialName].Kd = floats;
        }
        else if (line.compare(0, 3, "Ks ") == 0)  // Ks
        {
            iss >> strTrash;
            Vec3f floats;
            iss >> floats.x >> floats.y >> floats.z;
            materials[materialName].Ks = floats;
        }
        else if (line.compare(0, 3, "Ke ") == 0)  // Ke
        {
            iss >> strTrash;
            Vec3f floats;
            iss >> floats.x >> floats.y >> floats.z;
            materials[materialName].Ke = floats;
        }
        // Texture Maps
        else if (line.compare(0, 7, "map_Kd ") == 0)  // map_Kd
        {
            std::string filename;
            iss >> strTrash >> filename;

            std::string textureFilename = directory + filename;
            loadTexture(textureFilename, materials[materialName].DiffuseMap,
                        flipVertically);
        }
        else if (line.compare(0, 7, "map_Ks ") == 0)  // map_Ks
        {
            std::string filename;
            iss >> strTrash >> filename;

            std::string textureFilename = directory + filename;
            loadTexture(textureFilename, materials[materialName].SpecularMap,
                        flipVertically);
        }
        else if (line.compare(0, 9, "map_Bump ") == 0)  // map_Bump / Normal
        {
            std::string filename;
            iss >> strTrash >> filename;

            std::string textureFilename = directory + filename;
            loadTexture(textureFilename, materials[materialName].NormalMap,
                        flipVertically);
        }
        else if (line.compare(0, 7, "map_Ao ") == 0)  // map_Ao
        {
            std::string filename;
            iss >> strTrash >> filename;

            std::string textureFilename = directory + filename;
            loadTexture(textureFilename, materials[materialName].AmbientOcclusionMap,
                        flipVertically);
        }
    }
    in.close();
}

// Load Texture File
void Model::loadTexture(const std::string& textureFilename, TGAImage& image,
                        bool flipVertically)
{
    bool success = image.ReadTgaFile(textureFilename.c_str());

    if (!success)
    {
        spdlog::warn("Failed to load texture: {}", textureFilename);
        return;
    }

    if (flipVertically) image.FlipVertically();  // flip v coordinate
}

// Generate Tangents
void Model::generateTangents()
{
    tangents = std::vector<Vec3f>(verts.size(), Vec3f(0.f));
    for (auto iter = meshes.begin(); iter != meshes.end(); ++iter)
    {
        Mesh& mesh = iter->second;
        for (int f = 0; f < mesh.NumFaces(); ++f)
        {
            int   indexP0 = mesh.GetVertIndex(f, 0);
            int   indexP1 = mesh.GetVertIndex(f, 1);
            int   indexP2 = mesh.GetVertIndex(f, 2);
            Vec3f edge1 = mesh.Vert(f, 1) - mesh.Vert(f, 0);
            Vec3f edge2 = mesh.Vert(f, 2) - mesh.Vert(f, 0);
            Vec2f deltaUv1 = mesh.TexCoord(f, 1) - mesh.TexCoord(f, 0);
            Vec2f deltaUv2 = mesh.TexCoord(f, 2) - mesh.TexCoord(f, 0);
            Float det = deltaUv1.s * deltaUv2.t - deltaUv2.s * deltaUv1.t;
            if (det == 0.f)
            {
                // spdlog::error("det = 0");
                mesh.AddTangentIndex(indexP0);
                mesh.AddTangentIndex(indexP1);
                mesh.AddTangentIndex(indexP2);
                continue;
            }
            Float inv = 1.f / det;
            Vec3f T = Normalize(inv * Vec3f(deltaUv2.t * edge1.x - deltaUv1.t * edge2.x,
                                            deltaUv2.t * edge1.y - deltaUv1.t * edge2.y,
                                            deltaUv2.t * edge1.z - deltaUv1.t * edge2.z));
            tangents[indexP0] += T;
            tangents[indexP1] += T;
            tangents[indexP2] += T;
            mesh.AddTangentIndex(indexP0);
            mesh.AddTangentIndex(indexP1);
            mesh.AddTangentIndex(indexP2);
        }
    }

    // Average Tangents
    for (auto& v : tangents)
    {
        if (v.Length() == 0.f)
            v = Vec3f(1, 0, 0);  // random (to be improved)
        else
            v = Normalize(v);
    }
}