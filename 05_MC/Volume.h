#pragma once

#include <string>
#include <vector>
#include <sstream>

#include <Vec3.h>

class Volume {
public:
  Volume() :
    width{0}, height{0}, depth{0},
    scale{0.0f, 0.0f, 0.0f}
  {}

  size_t width;
  size_t height;
  size_t depth;
  size_t maxSize;
  Vec3 scale;

  std::vector<uint8_t> data;
  std::vector<Vec3> normals;
  
  void normalizeScale() {
    maxSize = std::max(width,std::max(height,depth));
    const Vec3 extend = scale*Vec3{float(width),float(height),float(depth)}/maxSize;
    const float m = std::max(extend.x,std::max(extend.y,extend.z));
    scale = scale / m;
  }

  Volume resample(size_t targetWidth, size_t targetHeight, size_t targetDepth) {
    Volume result;
    result.width = targetWidth;
    result.height = targetHeight;
    result.depth = targetDepth;
    result.scale = scale;
    result.normalizeScale();

    result.data.resize(result.width*result.height*result.depth);

    size_t targetIndex{0};
    for (size_t w = 0;w<result.depth;++w) {
      for (size_t v = 0;v<result.height;++v) {
        for (size_t u = 0;u<result.width;++u) {
          const uint8_t value = sample(float(u)/float(result.width),
                                       float(v)/float(result.height),
                                       float(w)/float(result.depth));
          result.data[targetIndex++] = value;
        }
      }
    }

    return result;
  }
  
  std::string toString() const {
    std::stringstream ss;
    ss << "width: " << width << "\n";
    ss << "height: " << height << "\n";
    ss << "depth: " << depth << "\n";
    ss << "dataseize: " << data.size() << "\n";
    ss << "scale: " << scale << "\n";
    
    for (size_t i = 0;i<data.size();++i) {
      if (i > 0 && i % width == 0) ss << "\n";
      if (i > 0 && i % (width*height) == 0) ss << "\n";
      ss << int(data[i]) << " ";
    }
    
    return ss.str();
  }
  
  void computeNormals() {
    normals.resize(data.size());
    for (size_t w = 1;w<depth-1;++w) {
      for (size_t v = 1;v<height-1;++v) {
        for (size_t u = 1;u<width-1;++u) {
          const size_t index = u + v * width + w * width * height;
          const Vec3 normal{
            float(data[index-1]) - float(data[index+1]),
            float(data[index-width]) - float(data[index+width]),
            float(data[index-width*height]) - float(data[index+width*height])
          };
          normals[index] = Vec3::normalize(normal);
        }
      }
    }
  }

private:
  uint8_t sample(float u, float v, float w) {
    Vec3 voxelIndex{u*width-1, v*height-1, w*depth-1};

    const Vec3 f{floor(voxelIndex.x),floor(voxelIndex.y),floor(voxelIndex.z)};
    const Vec3 alpha{voxelIndex-f};

    const float v0 = getValue(size_t(f.x)+0,size_t(f.y)+0,size_t(f.z)+0);
    const float v1 = getValue(size_t(f.x)+1,size_t(f.y)+0,size_t(f.z)+0);
    const float v2 = getValue(size_t(f.x)+0,size_t(f.y)+1,size_t(f.z)+0);
    const float v3 = getValue(size_t(f.x)+1,size_t(f.y)+1,size_t(f.z)+0);
    const float v4 = getValue(size_t(f.x)+0,size_t(f.y)+0,size_t(f.z)+1);
    const float v5 = getValue(size_t(f.x)+1,size_t(f.y)+0,size_t(f.z)+1);
    const float v6 = getValue(size_t(f.x)+0,size_t(f.y)+1,size_t(f.z)+1);
    const float v7 = getValue(size_t(f.x)+1,size_t(f.y)+1,size_t(f.z)+1);

    return uint8_t(
             (((v0*(1-alpha.x) + v1*alpha.x) * (1-alpha.y) +
               (v2*(1-alpha.x) + v3*alpha.x) * alpha.y)) * (1-alpha.z) +

             (((v4*(1-alpha.x) + v5*alpha.x) * (1-alpha.y) +
               (v6*(1-alpha.x) + v7*alpha.x) * alpha.y)) * alpha.z);
  }

  uint8_t getValue(size_t u, size_t v, size_t w) {
    const size_t index = u + v * width + w * width * height;
    return data[index];
  }
};
