#include "pch.h"
#include "Shader.h"

namespace JEngine {
Shader::Shader(const std::string& name, const std::wstring& filePath, const std::string& entryPoint,
               const std::string& targetVersion) : name_(name), shader_(compileShader(filePath, entryPoint, targetVersion)) {
}

Shader::~Shader() {
}

Shader::Shader(Shader&& other) noexcept : name_(std::move(other.name_)), shader_(std::move(other.shader_)) {

}
Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        name_ = std::move(other.name_);
        shader_ = std::move(other.shader_);
    }
    return *this;
}
D3D12_SHADER_BYTECODE Shader::GetShader() const {
    return {reinterpret_cast<BYTE*>(shader_->GetBufferPointer()), shader_->GetBufferSize()};
}

ComPtr<ID3DBlob> Shader::compileShader(const std::wstring& filePath, const std::string& entryPoint,
                                       const std::string& targetVersion) {
    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> byteCode;
    ComPtr<ID3DBlob> errors;
    ThrowIfFailed(::D3DCompileFromFile(filePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                     entryPoint.c_str(), targetVersion.c_str(), compileFlags, 0,
                                     &byteCode, &errors));

    return byteCode;
}
} // namespace JEngine