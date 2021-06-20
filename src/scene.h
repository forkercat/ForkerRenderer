//
// Created by Junhao Wang (@Forkercat) on 2021/6/19.
//

#pragma once

#include <camera.h>

#include <memory>
#include <string>
#include <vector>

class PointLight;
class DirLight;
class Model;

class Scene
{
public:
    explicit Scene(const std::string& filename);

    // Light
    const PointLight& GetPointLight() const
    {
        assert(m_PointLight != nullptr);
        return *m_PointLight;
    }

    const DirLight& GetDirLight() const
    {
        assert(m_DirLight != nullptr);
        return *m_DirLight;
    }

    // Camera
    const Camera& GetCamera() const
    {
        assert(m_Camera != nullptr);
        return *m_Camera;
    }

    Camera::ProjectionType GetProjectionType() const { return m_ProjectionType; }

    // Model

    unsigned int GetModelCount() const { return m_Models.size(); }

    const Model& GetModel(int index) const { return *m_Models[index]; }

    Matrix4x4f GetModelMatrix(int index) const { return m_ModelMatrices[index]; }

private:
    std::unique_ptr<PointLight>         m_PointLight;
    std::unique_ptr<DirLight>           m_DirLight;
    std::unique_ptr<Camera>             m_Camera;
    Camera::ProjectionType              m_ProjectionType;
    std::vector<std::unique_ptr<Model>> m_Models;
    std::vector<Matrix4x4f>             m_ModelMatrices;
};
