#pragma once
#include "Shader.h"

namespace JEngine {

struct ShaderConfig
{
    std::wstring fileName;
    std::string entryPoint;
    std::string targetVersion;
};

struct PipelineShadersConfig
{
    std::string vsName_;
    std::string psName_;
    std::string hsName_;
    std::string dsName_;
    std::string gsName_;
    std::string csName_;
};

struct PipelineShaders
{
    Shader* vs_;
    Shader* ps_;
    Shader* hs_;
    Shader* ds_;
    Shader* gs_;
    Shader* cs_;
};

class ShaderManager
{
  public:
    ShaderManager(const std::wstring& assetsPath);
    ~ShaderManager();

    void LoadShader(const std::string& name, const ShaderConfig& config);
    void CreatePipelineShaders(const std::string& pipelineName, const PipelineShadersConfig& config);
    Shader* GetShaderPtr(const std::string& name) const;

    const PipelineShaders& GetPipelineShaders(const std::string& pipelineName) const;
    bool IsPipelineShadersExist(const std::string& pipelineName) const;
  private:
    const std::wstring assetsPath_;
    std::unordered_map<std::string, std::unique_ptr<Shader>> shaders_;
    std::unordered_map<std::string, PipelineShaders> pipelineShaders_;
};
} // namespace JEngine