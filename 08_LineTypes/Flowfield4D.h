#pragma once

#include <vector>
#include <Vec3.h>


enum class DemoType {
  DRAIN,
  SATTLE,
  CRITICAL
};

class Flowfield4D {
public:
  Flowfield4D(size_t sizeX, size_t sizeY, size_t sizeZ, uint8_t timesteps);
  Vec3 interpolate(const Vec3& pos, float time) const;
  
  static Flowfield4D genDemo(size_t size, const std::vector<DemoType>& d);
private:
  size_t sizeX;
  size_t sizeY;
  size_t sizeZ;
  std::vector<std::vector<Vec3>> data;
  Vec3 getData(size_t x, size_t y, size_t z, size_t timeStep) const;
  Vec3 interpolateSteps(const Vec3& pos, size_t timeStep) const;
  
  Vec3 linear(const Vec3& a, const Vec3& b, float alpha) const;
};
