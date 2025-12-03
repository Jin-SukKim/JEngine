#pragma once

namespace JEngine {
class Shader
{
  public:
    Shader(const std::string& name, const std::wstring& filePath, const std::string& entryPoint,
           const std::string& targetVersion);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    D3D12_SHADER_BYTECODE GetShader() const;

  private:
    ComPtr<ID3DBlob> compileShader(const std::wstring& filePath, const std::string& entryPoint,
                                   const std::string& targetVersion);
  private:
    std::string name_;
    ComPtr<ID3DBlob> shader_;
};
} // namespace JEngine