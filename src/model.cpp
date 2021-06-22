//
// Created by Junhao Wang (@Forkercat) on 2020/12/18.
//

#include "model.h"

#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>

#include <fstream>
#include <sstream>
#include <string>

#include "forkergl.h"
#include "shader.h"
#include "utility.h"

/////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<Model> Model::Load(const std::string& filename, bool normalized,
                                   bool generateTangent, bool flipTexCoordY)
{
    // Init Model
    std::unique_ptr<Model> model = std::make_unique<Model>();

    model->m_Meshes.clear();
    model->m_Materials.clear();
    model->m_PBRMaterials.clear();
    model->m_Verts.clear();
    model->m_TexCoords.clear();
    model->m_Normals.clear();

    spdlog::info("  [Model] \'{}\'", filename);

    model->m_HasTangents = generateTangent;

    // Load Object, Material, Texture Files
    bool success = model->loadObjectFile(filename, flipTexCoordY);

    if (!success)
    {
        spdlog::error("Failed to load model");
        return nullptr;
    }

    // Post-Processing
    if (generateTangent) model->generateTangents();
    if (normalized) model->normalizePositionVertices();

    // clang-format off
    spdlog::info(
        "     v# {}, f# {}, vt# {}, vn# {}, tg# {}, mesh# {}, mtl# {} | normalized[{}] generateTangent[{}], flipTexCoordY[{}]",
        model->GetNumVerts(), model->GetNumFaces(), model->m_TexCoords.size(), model->m_Normals.size(), model->m_Tangents.size(), model->m_Meshes.size(),
        model->m_Materials.size(), normalized ? "o" : "x", generateTangent ? "o" : "x", flipTexCoordY ? "o" : "x");

    for (auto iter = model->m_Meshes.begin(); iter != model->m_Meshes.end(); ++iter)
    {
        const Mesh& mesh = *(iter->second);

        std::shared_ptr<const PBRMaterial> pbrMaterial = mesh.GetPBRMaterial();

        if (pbrMaterial->HasMetalnessMap() || pbrMaterial->HasRoughnessMap())
        {
            model->m_SupportPBR = true;
            spdlog::info("     [{:}] f# {:} | PBR[o] {}", iter->first, mesh.NumFaces(), *pbrMaterial);
        }
        else
        {
            model->m_SupportPBR = false;
            spdlog::info("     [{:}] f# {:} | PBR[x] {}", iter->first, mesh.NumFaces(), *(mesh.GetMaterial()));
        }
    }
    // clang-format on

    /* Actually std::move() is not needed because of copy elision */
    return std::move(model);
}

/////////////////////////////////////////////////////////////////////////////////

void Model::Render(Shader& shader) const
{
    spdlog::stopwatch stopwatch;
    // For each mesh
    for (auto iter = m_Meshes.begin(); iter != m_Meshes.end(); ++iter)
    {
        const Mesh& mesh = *(iter->second);

        mesh.Draw(shader);

        spdlog::info("  [{}] Time Used: {:.6} Seconds", iter->first, stopwatch);
        stopwatch.reset();
    }
}

/////////////////////////////////////////////////////////////////////////////////

int Model::GetNumFaces() const
{
    int total = 0;
    for (auto iter = m_Meshes.begin(); iter != m_Meshes.end(); ++iter)
    {
        const Mesh& mesh = *(iter->second);
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
        Vector3f v = m_Verts[i];
        xmin = Min(xmin, v.x), xmax = Max(xmax, v.x);
        ymin = Min(ymin, v.y), ymax = Max(ymax, v.y);
        zmin = Min(zmin, v.z), zmax = Max(zmax, v.z);
    }

    CHECK_NE(xmax - xmin, 0);
    CHECK_NE(ymax - ymin, 0);
    CHECK_NE(zmax - zmin, 0);

    Float scaleFactor = 2.f / Max(xmax - xmin, Max(ymax - ymin, zmax - zmin));
    Matrix4f m(1.f);
    m[0][0] = scaleFactor;
    m[1][1] = scaleFactor;
    m[2][2] = scaleFactor;
    m[0][3] = -(xmax + xmin) / (xmax - xmin);
    m[1][3] = -(ymax + ymin) / (ymax - ymin);
    m[2][3] = -(zmax + zmin) / (zmax - zmin);

    for (size_t i = 0; i < GetNumVerts(); ++i)
    {
        m_Verts[i] = (m * Vector4f(m_Verts[i], 1.f)).xyz;
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
        line = Ltrim(line);
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
            Vector3f vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            m_Verts.push_back(vertex);
        }
        else if (line.compare(0, 3, "vt ") == 0)  // vt
        {
            iss >> strTrash;  // skip 'vt' and ' '
            Vector2f texCoord;
            iss >> texCoord.x >> texCoord.y;
            Float floatTrash;
            iss >> floatTrash;  // ignore the last value
            m_TexCoords.push_back(texCoord);
        }
        else if (line.compare(0, 3, "vn ") == 0)  // vn
        {
            iss >> strTrash;  // skip 'vn' and ' '

            Vector3f normal;
            iss >> normal.x >> normal.y >> normal.z;
            m_Normals.push_back(normal);
        }
        // Change Mesh / Material Status
        // Only supported when there are "g" and "usemtl" keywords (in order)
        else if (line.compare(0, 2, "g ") == 0)  // g
        {
            iss >> chTrash >> meshName;
            m_Meshes[meshName] = std::make_unique<Mesh>(*this);
        }
        else if (line.compare(0, 7, "usemtl ") == 0)  // usemtl
        {
            iss >> strTrash >> materialName;
            m_Meshes[meshName]->SetMaterial(m_Materials[materialName]);
            m_Meshes[meshName]->SetPBRMaterial(m_PBRMaterials[materialName]);
        }
        // Faces
        else if (line.compare(0, 2, "f ") == 0)  // f
        {
            iss >> chTrash;  // skip 'f' and ' '
            std::vector<Vector3i> vertices;
            unsigned int       v, t, n;
            while (iss >> v >> chTrash >> t >> chTrash >> n)  // f 24/1/24 25/2/25 26/3/26
            {
                vertices.push_back(Vector3i(--v, --t, --n));
            }

            Mesh& mesh = *m_Meshes[meshName];
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
        spdlog::error("Cannot open the .mtl file: \'{}\'", mtlFilename);
        return;
    }

    std::string line;
    std::string materialName;  // current name
    while (!in.eof())
    {
        std::getline(in, line);
        line = Ltrim(line);
        std::istringstream iss(line.c_str());

        // Trash
        std::string strTrash;

        if (line.compare(0, 7, "newmtl ") == 0)  // newmtl
        {
            iss >> strTrash >> materialName;
            m_Materials[materialName] = std::make_shared<Material>(materialName);
            m_PBRMaterials[materialName] = std::make_shared<PBRMaterial>(materialName);
        }
        // Ka / Kd / Ks / Ke
        else if (line.compare(0, 3, "Ka ") == 0)  // Ka
        {
            iss >> strTrash;  // skip 'Ka' and ' '
            Vector3f floats;
            iss >> floats.x >> floats.y >> floats.z;
            m_Materials[materialName]->ka = floats;
            m_PBRMaterials[materialName]->ka = floats;
        }
        else if (line.compare(0, 3, "Kd ") == 0)  // Kd
        {
            iss >> strTrash;
            Vector3f floats;
            iss >> floats.x >> floats.y >> floats.z;
            m_Materials[materialName]->kd = floats;
            m_PBRMaterials[materialName]->albedo = floats;
        }
        else if (line.compare(0, 3, "Ks ") == 0)  // Ks
        {
            iss >> strTrash;
            Vector3f floats;
            iss >> floats.x >> floats.y >> floats.z;
            m_Materials[materialName]->ks = floats;
        }
        else if (line.compare(0, 3, "Ke ") == 0)  // Ke
        {
            iss >> strTrash;
            Vector3f floats;
            iss >> floats.x >> floats.y >> floats.z;
            m_Materials[materialName]->ke = floats;
            m_PBRMaterials[materialName]->ke = floats;
        }
        else if (line.compare(0, 3, "Pr ") == 0)  // Pr (roughness)
        {
            iss >> strTrash;
            float f;
            iss >> f;
            m_PBRMaterials[materialName]->roughness = f;
        }
        else if (line.compare(0, 3, "Pm ") == 0)  // Pm (metalness)
        {
            iss >> strTrash;
            float f;
            iss >> f;
            m_PBRMaterials[materialName]->metalness = f;
        }
        // Texture Maps
        else if (line.compare(0, 7, "map_Kd ") == 0)  // map_Kd
        {
            std::string filename;
            iss >> strTrash >> filename;

            std::string textureFilename = directory + filename;
            loadTexture(textureFilename, m_Materials[materialName]->diffuseMap,
                        flipVertically);
            loadTexture(textureFilename, m_PBRMaterials[materialName]->baseColorMap,
                        flipVertically);
        }
        else if (line.compare(0, 7, "map_Ks ") == 0)  // map_Ks
        {
            std::string filename;
            iss >> strTrash >> filename;

            std::string textureFilename = directory + filename;
            loadTexture(textureFilename, m_Materials[materialName]->specularMap,
                        flipVertically);
        }
        else if (line.compare(0, 7, "map_Ke ") == 0)  // map_Ke
        {
            std::string filename;
            iss >> strTrash >> filename;

            std::string textureFilename = directory + filename;
            loadTexture(textureFilename, m_Materials[materialName]->emissiveMap,
                flipVertically);
            loadTexture(textureFilename, m_PBRMaterials[materialName]->emissiveMap,
                flipVertically);
        }
        else if (line.compare(0, 9, "map_Bump ") == 0 ||
                 line.compare(0, 5, "norm ") == 0)  // map_Bump / Normal
        {
            std::string filename;
            iss >> strTrash >> filename;

            std::string textureFilename = directory + filename;
            loadTexture(textureFilename, m_Materials[materialName]->normalMap,
                        flipVertically);
            loadTexture(textureFilename, m_PBRMaterials[materialName]->normalMap,
                        flipVertically);
        }
        else if (line.compare(0, 7, "map_Ao ") == 0)  // map_Ao
        {
            std::string filename;
            iss >> strTrash >> filename;

            std::string textureFilename = directory + filename;
            loadTexture(textureFilename,
                        m_PBRMaterials[materialName]->ambientOcclusionMap,
                        flipVertically);
        }
        else if (line.compare(0, 7, "map_Pr ") == 0)  // map_Pr
        {
            std::string filename;
            iss >> strTrash >> filename;

            std::string textureFilename = directory + filename;
            loadTexture(textureFilename, m_PBRMaterials[materialName]->roughnessMap,
                        flipVertically);
        }
        else if (line.compare(0, 7, "map_Pm ") == 0)  // map_Pm
        {
            std::string filename;
            iss >> strTrash >> filename;

            std::string textureFilename = directory + filename;
            loadTexture(textureFilename, m_PBRMaterials[materialName]->metalnessMap,
                        flipVertically);
        }
    }
}

// Load Texture File
void Model::loadTexture(const std::string&        textureFilename,
                        std::shared_ptr<Texture>& texture, bool flipVertically)
{
    TGAImage image;
    bool     success = image.ReadTgaFile(textureFilename.c_str());

    if (!success)
    {
        spdlog::warn("Failed to load texture: {}", textureFilename);
        return;
    }

    if (flipVertically) image.FlipVertically();  // flip v coordinate

    texture = std::make_shared<Texture>(image, ForkerGL::TextureWrapping,
                                        ForkerGL::TextureFiltering);
}

// Generate Tangents
void Model::generateTangents()
{
    m_Tangents = std::vector<Vector3f>(m_Verts.size(), Vector3f(0.f));
    for (auto iter = m_Meshes.begin(); iter != m_Meshes.end(); ++iter)
    {
        Mesh& mesh = *(iter->second);
        for (int f = 0; f < mesh.NumFaces(); ++f)
        {
            int      indexP0 = mesh.GetVertIndex(f, 0);
            int      indexP1 = mesh.GetVertIndex(f, 1);
            int      indexP2 = mesh.GetVertIndex(f, 2);
            Vector3f edge1 = mesh.Vert(f, 1) - mesh.Vert(f, 0);
            Vector3f edge2 = mesh.Vert(f, 2) - mesh.Vert(f, 0);
            Vector2f deltaUv1 = mesh.TexCoord(f, 1) - mesh.TexCoord(f, 0);
            Vector2f deltaUv2 = mesh.TexCoord(f, 2) - mesh.TexCoord(f, 0);
            Float    det = deltaUv1.s * deltaUv2.t - deltaUv2.s * deltaUv1.t;
            if (det == 0.f)
            {
                // spdlog::error("det = 0");
                mesh.AddTangentIndex(indexP0);
                mesh.AddTangentIndex(indexP1);
                mesh.AddTangentIndex(indexP2);
                continue;
            }
            Float    inv = 1.f / det;
            Vector3f T =
                Normalize(inv * Vector3f(deltaUv2.t * edge1.x - deltaUv1.t * edge2.x,
                                         deltaUv2.t * edge1.y - deltaUv1.t * edge2.y,
                                         deltaUv2.t * edge1.z - deltaUv1.t * edge2.z));
            m_Tangents[indexP0] += T;
            m_Tangents[indexP1] += T;
            m_Tangents[indexP2] += T;
            mesh.AddTangentIndex(indexP0);
            mesh.AddTangentIndex(indexP1);
            mesh.AddTangentIndex(indexP2);
        }
    }

    // Average Tangents
    for (auto& v : m_Tangents)
    {
        if (v.Length() == 0.f)
            v = Vector3f(1, 0, 0);  // random (to be improved)
        else
            v = Normalize(v);
    }
}