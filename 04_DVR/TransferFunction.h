// TransferFunction.h
#pragma once

#include <vector>
#include <string>
#include <cstdint>

#include "Vec4.h"

#include "GLTexture1D.h"
#include "GLTexture2D.h"

class TransferFunction {
public:
  enum class Channel { R, G, B, A };

  enum class StringSource {
    FilePath,
    UrlPayload
  };

  // Construct with given size (initialized to zeros)
  explicit TransferFunction(uint32_t size = 0);

  // Construct and load from file
  explicit TransferFunction(const std::string& s,
                            StringSource source = StringSource::FilePath);

  // Load/save from/to file (simple binary format described in .cpp)
  void load(const std::string& filename);
  void save(const std::string& filename) const;

  std::string encodeForUrl() const;
  void decodeFromURL(const std::string& s);

  GLTexture1D& getTexture();
  GLTexture2D& getPreIntegratedTexture();
  GLTexture2D& getPeakFindingTexture();

  void setDelta(float dx);

  // Smooth-step on a single channel over normalized [0,1] domain of the TF
  // 'start' is the normalized start position, 'width' the extent.
  void smoothStep(Channel channel, float start, float width);

  // Set a single channel to a Gaussian bell curve over normalized [0,1] domain
  // of the TF.
  // 'center' is the normalized peak position, 'width' is the normalized
  // standard deviation (sigma). Values are in [0,1].
  void gaussian(Channel channel, float center, float width);

  // Access underlying RGBA data (float [0,1])
  const std::vector<Vec4>& getData() const { return rawData; }

  // Replace TF data; size is data.size()
  void setData(const std::vector<Vec4>& data);

  void setValue(float pos, float value, Channel channel);
  void setValue(size_t pos, float value, Channel channel);

  void setPreIntegration(bool p) {
    if (!usePreIntegration && p) {
      // when turning pre-integration on
      // make sure to regenerate the pre-integrated texture
      textureDirty = true;
    }
    usePreIntegration = p;
  }

  void setPeakFinding(bool p) {
    if (!usePeakFinding && p) {
      // when turning peak finding on
      // make sure to regenerate the peak finding texture
      textureDirty = true;
    }
    usePeakFinding = p;
  }

  // Resample TF to newSize via linear interpolation
  void resample(uint32_t newSize);

  // Current number of samples in the transfer function
  uint32_t getSize() const { return static_cast<uint32_t>(rawData.size()); }

private:
  std::vector<Vec4> rawData;

  GLTexture1D texture{
    GL_LINEAR,
    GL_LINEAR,
    GL_CLAMP_TO_EDGE
  };
  GLTexture2D preIntegratedTexture{
    GL_LINEAR,
    GL_LINEAR,
    GL_CLAMP_TO_EDGE,
    GL_CLAMP_TO_EDGE
  };
  GLTexture2D peakFindingTexture{
    GL_NEAREST,
    GL_NEAREST,
    GL_CLAMP_TO_EDGE,
    GL_CLAMP_TO_EDGE
  };

  bool textureDirty{true};
  bool usePreIntegration{false};
  bool usePeakFinding{false};

  // Cached step length for the pre-integration LUT.
  float delta{1.0f};

  void updateTexture();
  void updatePreIntTexture();
  void updatePeakFindingTexture();
  static float clamp01(float v);
  static Vec4  lerp(const Vec4& a, const Vec4& b, float t);
  Vec4         sample(float s01) const;
};
