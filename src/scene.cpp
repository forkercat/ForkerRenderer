//
// Created by Junhao Wang (@Forkercat) on 2021/6/19.
//

#include "scene.h"

#include <spdlog/spdlog.h>

#include <fstream>
#include <sstream>

#include "color.h"
#include "light.h"
#include "model.h"
#include "utility.h"

const static int s_DefaultWidth = 1280;
const static int s_DefaultHeight = 800;

Scene::Scene(const std::string& filename)
    : m_Width(s_DefaultWidth),
      m_Height(s_DefaultHeight),
      m_PointLight(nullptr),
      m_DirLight(nullptr),
      m_Camera(nullptr),
      m_Models(),
      m_ModelMatrices()
{
    // Load Scene File
    std::ifstream in;
    in.open(filename, std::ifstream::in);

    if (in.fail())
    {
        spdlog::error("Failed to open the .scene file");
        assert(false);
    }

    spdlog::info("Scene File: {}", filename);

    // Data

    std::string line;
    while (!in.eof())
    {
        std::getline(in, line);
        line = Ltrim(line);
        std::istringstream iss(line.c_str());

        // Trash
        std::string strTrash;
        if (line.compare(0, 1, "#") == 0)  // comments
        {
            continue;
        }
        else if (line.compare(0, 7, "screen ") == 0)  // screen
        {
            iss >> strTrash >> m_Width >> m_Height;
            spdlog::info("  [Screen] {} x {}", m_Width, m_Height);
        }
        else if (line.compare(0, 6, "light ") == 0)  // light
        {
            std::string lightType;
            iss >> strTrash >> lightType;
            if (lightType == "point")
            {
                Point3f position;
                iss >> position.x >> position.y >> position.z;

                Color3 color;
                iss >> color.x >> color.y >> color.z;

                m_PointLight = std::make_unique<PointLight>(position, color);

                if (m_PointLight)
                {
                    spdlog::info("  [Point Light] position: {}, color: {}",
                                 m_PointLight->position, m_PointLight->color);
                }
            }
            else if (lightType == "dir")
            {
                Vector3f direction;
                iss >> direction.x >> direction.y >> direction.z;

                Color3 color;
                iss >> color.x >> color.y >> color.z;

                m_DirLight = std::make_unique<DirLight>(direction, color);

                if (m_DirLight)
                {
                    spdlog::info("  [Dir Light] direction: {}, color: {}",
                                 m_DirLight->direction, m_DirLight->color);
                }
            }
            else
            {
                spdlog::warn("Invalid light type: {}", lightType);
                continue;
            }
        }
        else if (line.compare(0, 7, "camera ") == 0)  // camera
        {
            std::string cameraType;
            iss >> strTrash >> cameraType;

            if (cameraType == "persp")
                m_ProjectionType = Camera::Perspective;
            else if (cameraType == "ortho")
                m_ProjectionType = Camera::Orthographic;
            else
            {
                spdlog::warn("Invalid camera type: {}", cameraType);
                continue;
            }

            Point3f eyePos;
            Point3f lookAtPos;

            iss >> eyePos.x >> eyePos.y >> eyePos.z;
            iss >> lookAtPos.x >> lookAtPos.y >> lookAtPos.z;

            m_Camera = std::make_unique<Camera>(eyePos, lookAtPos);

            if (m_Camera)
            {
                spdlog::info("  [Camera] position: {}, lookAt: {}",
                             m_Camera->GetPosition(), m_Camera->GetLookAt());
            }
        }
        else if (line.compare(0, 6, "model ") == 0)  // model
        {
            std::string filename;
            iss >> strTrash >> filename;
            std::string boolValue;
            iss >> boolValue;
            bool normalized = (boolValue == "true");
            iss >> boolValue;
            bool generateTangent = (boolValue == "true");

            Point3f position;
            Float   rotateY;
            Float   uniformScale;
            iss >> position.x >> position.y >> position.z;
            iss >> rotateY >> uniformScale;

            std::unique_ptr<Model> m =
                Model::Load(filename, normalized, generateTangent);  // Copy Elision

            m_Models.push_back(std::move(m));
            m_ModelMatrices.push_back(MakeModelMatrix(position, rotateY, uniformScale));
        }
    }
}
