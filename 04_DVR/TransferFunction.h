// TransferFunction.h
#pragma once

#include <vector>
#include <string>
#include <cstdint>

#ifndef __EMSCRIPTEN__
#include "GLTexture1D.h"
#else
#include "GLTexture2D.h"
#endif

// Simple RGBA color in [0,1]
struct RGBA {
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 0.0f;
};

class TransferFunction {
public:
  enum class Channel { R, G, B, A };

  // Construct with given size (initialized to zeros)
  explicit TransferFunction(uint32_t size = 0);

  // Construct and load from file
  explicit TransferFunction(const std::string& filename);

  // Load/save from/to file (simple binary format described in .cpp)
  void load(const std::string& filename);
  void save(const std::string& filename) const;

#ifndef __EMSCRIPTEN__
  GLTexture1D&       getTexture();
  const GLTexture1D& getTexture() const;
#else
  GLTexture2D&       getTexture();
  const GLTexture2D& getTexture() const;
#endif

  // Smooth-step on a single channel over normalized [0,1] domain of the TF
  // 'start' is the normalized start position, 'width' the extent.
  void smoothStep(Channel channel, float start, float width);
  // Convenience: accept 'R','G','B','A'
  void smoothStep(char channel, float start, float width);

  // Access underlying RGBA data (float [0,1])
  const std::vector<RGBA>& getData() const { return data_; }

  // Replace TF data; size is data.size()
  void setData(const std::vector<RGBA>& data);

  // Resample TF to newSize via linear interpolation
  void resample(uint32_t newSize);

  // Current number of samples in the transfer function
  uint32_t getSize() const { return static_cast<uint32_t>(data_.size()); }

private:
  std::vector<RGBA> data_;

#ifndef __EMSCRIPTEN__
  GLTexture1D texture_{GL_LINEAR,GL_LINEAR,GL_CLAMP_TO_EDGE};
#else
  GLTexture2D texture_{GL_LINEAR,GL_LINEAR,GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE};
#endif

  bool textureDirty_ = true;

  void updateTexture();
  static float clamp01(float v);
  static RGBA  lerp(const RGBA& a, const RGBA& b, float t);
  static Channel charToChannel(char c);
};
