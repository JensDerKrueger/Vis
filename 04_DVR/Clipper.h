#pragma once

#include <vector>
#include <Vec3.h>

class Clipper {
public:
  static std::vector<Vec3> triPlane(std::vector<Vec3>& posData,
                                    const Vec3 &normal, const float D);
  static void meshPlane(std::vector<Vec3>& posData, const Vec3 &normal,
                       const float D);

  static std::vector<float> meshPlane(std::vector<float> posData, const Vec3 &normal,
                       const float D);
};
