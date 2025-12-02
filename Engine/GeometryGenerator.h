#pragma once

namespace JEngine {

class Vertex;

class GeometryGenerator
{
  public:
    static void CreateBox(std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices,
                           float size = 1.f);
};
}