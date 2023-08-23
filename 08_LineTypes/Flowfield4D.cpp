#include <cmath>

#include "Flowfield4D.h"

Flowfield4D Flowfield4D::genDemo(size_t size, const std::vector<DemoType>& d) {
  Flowfield4D f{size,size,size,2};

  for (size_t ts = 0; ts < 2;++ts) {


    switch (d[ts]) {
      case DemoType::DRAIN :
      {
        for (size_t z = 0;z<size;++z) {
          const float localZ = float(z)/size;
          for (size_t y = 0;y<size;++y) {
            const float localY = float(y)/size;
            for (size_t x = 0;x<size;++x) {
              const float localX = float(x)/size;
              f.data[ts][x+y*size+z*size*size] = Vec3{(-localY+0.5f)+(0.5f-localX)/10.0f,(localX-0.5f)+(0.5f-localY)/10.0f,-localZ/10.0f};
            }
          }
        }
      }
        break;
      case DemoType::SATTLE :
      {
        for (size_t z = 0;z<size;++z) {
          const float localZ = float(z)/size;
          for (size_t y = 0;y<size;++y) {
            const float localY = float(y)/size;
            for (size_t x = 0;x<size;++x) {
              const float localX = float(x)/size;
              f.data[ts][x+y*size+z*size*size] = Vec3{0.5f-localX,localY-0.5f,0.5f-localZ};
            }
          }
        }
      }
        break;
      case DemoType::CRITICAL :
      {
        for (size_t z = 0;z<size;++z) {
          const float localZ = float(z)/size;
          for (size_t y = 0;y<size;++y) {
            const float localY = float(y)/size;
            for (size_t x = 0;x<size;++x) {
              const float localX = float(x)/size;
              f.data[ts][x+y*size+z*size*size] = Vec3{(localX-0.1f)*(localY-0.3f)*(localX-0.8f),(localY-0.7f)*(localZ-0.2f)*(localX-0.3f),(localZ-0.9f)*(localZ-0.6f)*(localX-0.5f)};
            }
          }
        }
      }
        break;
    }
  }
  
  return f;
}

Flowfield4D::Flowfield4D(size_t sizeX, size_t sizeY, size_t sizeZ, uint8_t timesteps) :
sizeX(sizeX),
sizeY(sizeY),
sizeZ(sizeZ)
{
  data.resize(timesteps);
  for (size_t i = 0;i<data.size();++i)
    data[i].resize(sizeX*sizeY*sizeZ);
}

Vec3 Flowfield4D::getData(size_t x, size_t y, size_t z, size_t timeStep) const {
  return data[timeStep%data.size()][x+y*sizeY+z*sizeX*sizeY];
}

Vec3 Flowfield4D::linear(const Vec3& a, const Vec3& b, float alpha) const {
  return a * (1.0f - alpha) + b * alpha;
}

Vec3 Flowfield4D::interpolate(const Vec3& pos, float time) const{
  const size_t fTime = size_t(floor(time));
  const size_t cTime = size_t(ceil(time));

  const Vec3 fData = interpolateSteps(pos, fTime);
  const Vec3 cData = interpolateSteps(pos, cTime);

  const float alpha = time - fTime;

  return linear(fData, cData, alpha);
}

Vec3 Flowfield4D::interpolateSteps(const Vec3& pos, size_t timeStep) const {
  const size_t fX = size_t(floor(pos.x * (sizeX-1)));
  const size_t fY = size_t(floor(pos.y * (sizeY-1)));
  const size_t fZ = size_t(floor(pos.z * (sizeZ-1)));
  
  const size_t cX = size_t(ceil(pos.x * (sizeX-1)));
  const size_t cY = size_t(ceil(pos.y * (sizeY-1)));
  const size_t cZ = size_t(ceil(pos.z * (sizeZ-1)));


  const std::array<Vec3, 8> values = {
    getData(fX,fY,fZ,timeStep),
    getData(cX,fY,fZ,timeStep),
    getData(fX,cY,fZ,timeStep),
    getData(cX,cY,fZ,timeStep),
    getData(fX,fY,cZ,timeStep),
    getData(cX,fY,cZ,timeStep),
    getData(fX,cY,cZ,timeStep),
    getData(cX,cY,cZ,timeStep)
  };
  
  const float alpha = pos.x * (sizeX-1) - fX;
  const float beta  = pos.y * (sizeY-1) - fY;
  const float gamma = pos.z * (sizeZ-1) - fZ;
    
  return linear(linear(linear(values[0], values[1], alpha),
                       linear(values[2], values[3], alpha),
                       beta),
                linear(linear(values[4], values[5], alpha),
                       linear(values[6], values[7], alpha),
                       beta),
                gamma);
}
