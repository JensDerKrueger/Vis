#pragma once

#include "Vec3.h"
#include <vector>

class Tesselation {
public:
  static Tesselation genSphere(const Vec3& center, const float radius, const uint32_t sectorCount, const uint32_t stackCount);
  static Tesselation genRectangle(const Vec3& center, const float width, const float height);
  static Tesselation genRectangle(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d);
  static Tesselation genBrick(const Vec3& center, const Vec3& size, const Vec3& texScale=Vec3{1,1,1});
  static Tesselation genTorus(const Vec3 &center, float majorRadius, float minorRadius, uint32_t majorSteps=200, uint32_t minorSteps=50);

  const std::vector<float>& getVertices() const {return vertices;}
  const std::vector<float>& getNormals() const {return normals;}
  const std::vector<float>& getTangents() const {return tangents;}
  const std::vector<float>& getTexCoords() const {return texCoords;}
  const std::vector<uint32_t>& getIndices() const {return indices;}

  Tesselation unpack() const;


private:
  Tesselation() {}

  std::vector<float> vertices;
  std::vector<float> normals;
  std::vector<float> tangents;
  std::vector<float> texCoords;
  std::vector<uint32_t> indices;
};

