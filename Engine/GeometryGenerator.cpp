#include "pch.h"
#include "GeometryGenerator.h"
#include "Vertex.h"

namespace JEngine {
void GeometryGenerator::CreateBox(std::vector<Vertex>& outVertices,
                                   std::vector<uint32_t>& outIndices, float size) {
    using namespace DirectX;

    outVertices = {Vertex({XMFLOAT3(-size, -size, -size), XMFLOAT4(Colors::White)}),
                   Vertex({XMFLOAT3(-size, +size, -size), XMFLOAT4(Colors::Black)}),
                   Vertex({XMFLOAT3(+size, +size, -size), XMFLOAT4(Colors::Red)}),
                   Vertex({XMFLOAT3(+size, -size, -size), XMFLOAT4(Colors::Green)}),
                   Vertex({XMFLOAT3(-size, -size, +size), XMFLOAT4(Colors::Blue)}),
                   Vertex({XMFLOAT3(-size, +size, +size), XMFLOAT4(Colors::Yellow)}),
                   Vertex({XMFLOAT3(+size, +size, +size), XMFLOAT4(Colors::Cyan)}),
                   Vertex({XMFLOAT3(+size, -size, +size), XMFLOAT4(Colors::Magenta)})};

    outIndices = {0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 4, 5, 1, 4, 1, 0,
                  3, 2, 6, 3, 6, 7, 1, 5, 6, 1, 6, 2, 4, 0, 3, 4, 3, 7};
}
} // namespace JEngine
