#include "pch.h"
#include "ShaderManager.h"

namespace JEngine {
ShaderManager::ShaderManager(const std::wstring& assetsPath) : assetsPath_(assetsPath) {
}

ShaderManager::~ShaderManager() {
}

void ShaderManager::LoadShader(const ShaderConfig& config) {
    if (shaders_.find(config.name) != shaders_.end()) {
        LogInfo("Shader '{}' is already loaded. Skipping.\n", config.name);
        return;
    }

    shaders_[config.name] = std::make_unique<Shader>(config.name, assetsPath_ + config.path,
                                              config.entryPoint, config.targetVersion);
}
void ShaderManager::CreatePipelineShaders(const std::string& pipelineName,
                                          const PipelineShadersConfig& config) {
    if (pipelineShaders_.find(pipelineName) != pipelineShaders_.end()) {
        LogInfo("Pipeline '{}' shaders are already created. Skipping.\n", pipelineName);
        return;
    }

    PipelineShaders shaders;
    shaders.vs_ = GetShaderPtr(config.vsName_);
    shaders.ps_ = GetShaderPtr(config.psName_);
    shaders.hs_ = GetShaderPtr(config.hsName_);
    shaders.ds_ = GetShaderPtr(config.dsName_);
    shaders.gs_ = GetShaderPtr(config.gsName_);
    shaders.cs_ = GetShaderPtr(config.csName_);
    pipelineShaders_[pipelineName] = std::move(shaders);
}

Shader* ShaderManager::GetShaderPtr(const std::string& name) const {
    auto it = shaders_.find(name);
    if (it == shaders_.end()) 
        return nullptr;
    return it->second.get();
}

const PipelineShaders& ShaderManager::GetPipelineShaders(const std::string& pipelineName) const {
    return pipelineShaders_.at(pipelineName);
}

bool ShaderManager::IsPipelineShadersExist(const std::string& pipelineName) const {
    return pipelineShaders_.find(pipelineName) != pipelineShaders_.end();
}

} // namespace JEngine