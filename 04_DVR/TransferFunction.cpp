// TransferFunction.cpp
#include "TransferFunction.h"
#include "Compression.h"
#include "Base64Url.h"

#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <cmath>

float TransferFunction::clamp01(float v) {
  if (v < 0.0f) return 0.0f;
  if (v > 1.0f) return 1.0f;
  return v;
}

Vec4 TransferFunction::lerp(const Vec4& a, const Vec4& b, float t)
{
  Vec4 r;
  r.r = a.r + (b.r - a.r) * t;
  r.g = a.g + (b.g - a.g) * t;
  r.b = a.b + (b.b - a.b) * t;
  r.a = a.a + (b.a - a.a) * t;
  return r;
}

Vec4 TransferFunction::sample(float s01) const
{
  const uint32_t n = static_cast<uint32_t>(rawData.size());
  if (n == 0) return Vec4{};
  if (n == 1) return rawData.front();

  float s = clamp01(s01);

  float pos = s * static_cast<float>(n - 1);
  uint32_t idx0 = static_cast<uint32_t>(pos);
  uint32_t idx1 = (idx0 + 1 < n) ? (idx0 + 1) : idx0;
  float t = pos - static_cast<float>(idx0);
  return lerp(rawData[idx0], rawData[idx1], t);
}


// Constructors
TransferFunction::TransferFunction(uint32_t size)
: rawData(size)
, textureDirty(true)
{
}

void TransferFunction::decodeFromURL(const std::string& encoded) {
  std::vector<uint8_t> packed = Base64Url::decodeNoPad(encoded);

  std::vector<uint8_t> bytes;
  try {
    bytes = Compression::decompressRLE(packed);
  } catch (const Compression::CompressionError& e) {
    throw std::runtime_error(std::string("TransferFunction: RLE decode failed: ") + e.what());
  }

  if ((bytes.size() % 4) != 0) {
    throw std::runtime_error("TransferFunction: decoded payload size not divisible by 4");
  }

  const size_t n = bytes.size() / 4;

  // Planar: [R0..R(n-1)][G0..G(n-1)][B0..B(n-1)][A0..A(n-1)]
  const uint8_t* r = bytes.data() + 0 * n;
  const uint8_t* g = bytes.data() + 1 * n;
  const uint8_t* b = bytes.data() + 2 * n;
  const uint8_t* a = bytes.data() + 3 * n;

  rawData.resize(n);
  for (size_t i = 0; i < n; ++i) {
    rawData[i].r = float(r[i]) / 255.0f;
    rawData[i].g = float(g[i]) / 255.0f;
    rawData[i].b = float(b[i]) / 255.0f;
    rawData[i].a = float(a[i]) / 255.0f;
  }
}

TransferFunction::TransferFunction(const std::string& s, StringSource source)
: textureDirty(true) {
  if (source == StringSource::FilePath) {
    load(s);
  } else if (source == StringSource::UrlPayload) {
    decodeFromURL(s);
  } else {
    throw std::runtime_error("TransferFunction: unknown StringSource");
  }
}

// File format (very simple binary):
// 4-byte magic: 'T', 'F', 'N', '1'
// uint32_t: size
// size * 4 * float: r,g,b,a in [0,1] for each element
void TransferFunction::load(const std::string& filename)
{
  std::ifstream in(filename, std::ios::binary);
  if (!in)
    throw std::runtime_error("TransferFunction::load: cannot open file " + filename);

  char magic[4];
  in.read(magic, 4);
  if (!in || magic[0] != 'T' || magic[1] != 'F' || magic[2] != 'N' || magic[3] != '1')
    throw std::runtime_error("TransferFunction::load: invalid file format");

  uint32_t size = 0;
  in.read(reinterpret_cast<char*>(&size), sizeof(uint32_t));
  if (!in)
    throw std::runtime_error("TransferFunction::load: failed to read size");

  rawData.resize(size);

  for (uint32_t i = 0; i < size; ++i)
  {
    float rgba[4];
    in.read(reinterpret_cast<char*>(rgba), sizeof(rgba));
    if (!in)
      throw std::runtime_error("TransferFunction::load: unexpected EOF");

    rawData[i].r = rgba[0];
    rawData[i].g = rgba[1];
    rawData[i].b = rgba[2];
    rawData[i].a = rgba[3];
  }

  textureDirty = true;
}

void TransferFunction::save(const std::string& filename) const
{
  std::ofstream out(filename, std::ios::binary);
  if (!out)
    throw std::runtime_error("TransferFunction::save: cannot open file " + filename);

  const char magic[4] = { 'T', 'F', 'N', '1' };
  out.write(magic, 4);

  uint32_t size = static_cast<uint32_t>(rawData.size());
  out.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));

  for (uint32_t i = 0; i < size; ++i)
  {
    const float rgba[4] = {
      rawData[i].r,
      rawData[i].g,
      rawData[i].b,
      rawData[i].a
    };
    out.write(reinterpret_cast<const char*>(rgba), sizeof(rgba));
  }

  if (!out)
    throw std::runtime_error("TransferFunction::save: write error");
}

static GLubyte toByte(float v) {
  float clamped = v;
  if (clamped < 0.0f) clamped = 0.0f;
  if (clamped > 1.0f) clamped = 1.0f;
  int iv = static_cast<int>(clamped * 255.0f + 0.5f);
  if (iv < 0) iv = 0;
  if (iv > 255) iv = 255;
  return static_cast<GLubyte>(iv);
};

std::string TransferFunction::encodeForUrl() const {
  const size_t n = rawData.size();
  std::vector<uint8_t> bytes;
  bytes.resize(n * 4);

  uint8_t* r = bytes.data() + 0 * n;
  uint8_t* g = bytes.data() + 1 * n;
  uint8_t* b = bytes.data() + 2 * n;
  uint8_t* a = bytes.data() + 3 * n;

  for (size_t i = 0; i < n; ++i) {
    r[i] = uint8_t(toByte(rawData[i].r));
    g[i] = uint8_t(toByte(rawData[i].g));
    b[i] = uint8_t(toByte(rawData[i].b));
    a[i] = uint8_t(toByte(rawData[i].a));
  }

  std::vector<uint8_t> packed;
  try {
    packed = Compression::compressRLE(bytes);
  } catch (const Compression::CompressionError& e) {
    throw std::runtime_error(std::string("TransferFunction: RLE encode failed: ") + e.what());
  }

  return Base64Url::encodeNoPad(packed);
}

GLTexture1D& TransferFunction::getTexture()
{
  if (textureDirty)
    updateTexture();
  return texture;
}

GLTexture2D& TransferFunction::getPreIntegratedTexture() {
  if (textureDirty)
    updateTexture();
  return preIntegratedTexture;
}

GLTexture2D& TransferFunction::getPeakFindingTexture() {
  if (textureDirty)
    updateTexture();
  return peakFindingTexture;
}


void TransferFunction::setDelta(float dx) {
  float newDelta = dx;
  if (newDelta <= 0.0f) newDelta = 1e-6f;

  if (std::abs(newDelta - delta) > 1e-9f) {
    delta = newDelta;
    textureDirty = true;
  }
}

void TransferFunction::smoothStep(Channel channel, float start, float width)
{
  if (rawData.empty() || width <= 0.0f)
    return;

  const uint32_t n = static_cast<uint32_t>(rawData.size());

  for (uint32_t i = 0; i < n; ++i)
  {
    float x = (n > 1) ? static_cast<float>(i) / static_cast<float>(n - 1) : 0.0f;

    float t = (x - start) / width;
    t = clamp01(t);
    // classic smoothstep: 3t^2 - 2t^3
    t = t * t * (3.0f - 2.0f * t);

    switch (channel)
    {
      case Channel::R: rawData[i].r = t; break;
      case Channel::G: rawData[i].g = t; break;
      case Channel::B: rawData[i].b = t; break;
      case Channel::A: rawData[i].a = t; break;
    }
  }

  textureDirty = true;
}

void TransferFunction::gaussian(Channel channel, float center, float width)
{
  if (rawData.empty() || width <= 0.0f)
    return;

  const uint32_t n = static_cast<uint32_t>(rawData.size());

  for (uint32_t i = 0; i < n; ++i)
  {
    float x = (n > 1) ? static_cast<float>(i) / static_cast<float>(n - 1) : 0.0f;

    float d = (x - center) / width;
    float v = std::exp(-0.5f * d * d);
    v = clamp01(v);

    switch (channel)
    {
      case Channel::R: rawData[i].r = v; break;
      case Channel::G: rawData[i].g = v; break;
      case Channel::B: rawData[i].b = v; break;
      case Channel::A: rawData[i].a = v; break;
    }
  }

  textureDirty = true;
}

void TransferFunction::setData(const std::vector<Vec4>& data)
{
  rawData = data;
  textureDirty = true;
}

void TransferFunction::resample(uint32_t newSize)
{
  const uint32_t oldSize = static_cast<uint32_t>(rawData.size());

  if (newSize == oldSize)
    return;

  if (newSize == 0)
  {
    rawData.clear();
    textureDirty = true;
    return;
  }

  if (oldSize == 0)
  {
    // If we have no data, just create zeros
    rawData.assign(newSize, Vec4{});
    textureDirty = true;
    return;
  }

  std::vector<Vec4> newData(newSize);

  if (newSize == 1)
  {
    newData[0] = rawData.front();
  }
  else
  {
    for (uint32_t i = 0; i < newSize; ++i)
    {
      // position in old TF space [0, oldSize-1]
      float pos = (oldSize > 1)
      ? static_cast<float>(i) * static_cast<float>(oldSize - 1) / static_cast<float>(newSize - 1)
      : 0.0f;

      uint32_t idx0 = static_cast<uint32_t>(pos);
      uint32_t idx1 = (idx0 + 1 < oldSize) ? idx0 + 1 : idx0;

      float t = pos - static_cast<float>(idx0);

      newData[i] = lerp(rawData[idx0], rawData[idx1], t);
    }
  }

  rawData.swap(newData);
  textureDirty = true;
}

void TransferFunction::updatePreIntTexture() {
  const uint32_t n = static_cast<uint32_t>(rawData.size());
  const uint32_t integrationSteps = 128;
  const float invSteps = 1.0f / static_cast<float>(integrationSteps);
  const float ds = delta * invSteps;

  std::vector<GLubyte> preIntegratedData(4 * n * n);

  const float tfSize = (n > 1) ? static_cast<float>(n - 1) : 1.0f;

  for (uint32_t y = 0; y < n; ++y){
    const float s1 = static_cast<float>(y) / tfSize;

    for (uint32_t x = 0; x < n; ++x){
      const float s0 = static_cast<float>(x) / tfSize;

      float accR = 0.0f;
      float accG = 0.0f;
      float accB = 0.0f;
      float accA = 0.0f;

      for (uint32_t k = 0; k < integrationSteps; ++k) {
        float t = (static_cast<float>(k) + 0.5f) * invSteps;
        float s = s0 + (s1 - s0) * t;

        Vec4 tf = sample(s);

        float a = clamp01(tf.a);
        float alpha = 1.0f - std::pow(1.0f - a, ds);
        float oneMinusA = 1.0f - accA;

        accR += oneMinusA * tf.r * alpha;
        accG += oneMinusA * tf.g * alpha;
        accB += oneMinusA * tf.b * alpha;
        accA += oneMinusA * alpha;

        if (accA > 0.9995f) {
          accA = 1.0f;
          break;
        }
      }

      const size_t o = static_cast<size_t>(4u) * (static_cast<size_t>(y) * n + x);
      preIntegratedData[o + 0] = toByte(accR);
      preIntegratedData[o + 1] = toByte(accG);
      preIntegratedData[o + 2] = toByte(accB);
      preIntegratedData[o + 3] = toByte(accA);
    }
  }

  preIntegratedTexture.setData(preIntegratedData, n, n, 4);
}

void TransferFunction::updatePeakFindingTexture() {
  const uint32_t n = static_cast<uint32_t>(rawData.size());
  std::vector<GLubyte> preakFindingData(n * n);

  if (n == 0) {
    peakFindingTexture.setData(preakFindingData, n, n, 1);
    return;
  }

  const GLubyte invalid_flag = 255;

  std::vector<uint32_t> peaks;
  peaks.reserve(n / 2);

  auto a = [&](uint32_t i) -> float { return rawData[i].a; };

  const float eps = 1e-6f;

  uint32_t i = 1;
  while (i + 1 < n) {
    if (a(i) + eps < a(i - 1)) { ++i; continue; }

    uint32_t j = i;
    while (j + 1 < n && std::fabs(a(j + 1) - a(i)) <= eps) ++j;

    const float left  = a(i - 1);
    const float mid   = a(i);
    const float right = (j + 1 < n) ? a(j + 1) : -std::numeric_limits<float>::infinity();

    const bool isMax = (mid + eps >= left) && (mid > right + eps);
    if (isMax) {
      peaks.push_back((i + j) / 2);
    }

    i = j + 1;
  }

  if (peaks.empty()) {
    std::fill(preakFindingData.begin(), preakFindingData.end(), invalid_flag);
    peakFindingTexture.setData(preakFindingData, n, n, 1);
    return;
  }

  std::vector<int32_t> nextPeak(n, -1), prevPeak(n, -1);

  // nextPeak sweep from right
  int32_t p = (int32_t)peaks.size() - 1;
  int32_t next = -1;
  for (int32_t k = (int32_t)n - 1; k >= 0; --k) {
    while (p >= 0 && (int32_t)peaks[(size_t)p] == k) { next = k; --p; }
    nextPeak[(size_t)k] = next;
  }

  // prevPeak sweep from left
  size_t q = 0;
  int32_t prev = -1;
  for (uint32_t k = 0; k < n; ++k) {
    while (q < peaks.size() && peaks[q] == k) { prev = (int32_t)k; ++q; }
    prevPeak[k] = prev;
  }

  const float denom = (n > 1) ? float(n - 1) : 1.0f;
  auto peakIndexToByte = [&](uint32_t peakIndex) -> GLubyte {
    float iso01 = float(peakIndex) / denom;
    int v = (int)std::lround(iso01 * 254.0f);
    v = std::clamp(v, 0, 254);
    return (GLubyte)v;
  };

  for (uint32_t y = 0; y < n; ++y) {
    for (uint32_t x = 0; x < n; ++x) {
      GLubyte outIso = invalid_flag;

      if (x != y) {
        int32_t peakIdx = -1;

        if (x < y) {
          uint32_t start = std::min(x + 1u, n - 1u);
          peakIdx = nextPeak[start];
          if (peakIdx < 0 || peakIdx > (int32_t)y) peakIdx = -1;
        } else { // x > y
          uint32_t start = (x > 0) ? (x - 1u) : 0u;
          peakIdx = prevPeak[start];
          if (peakIdx < 0 || peakIdx < (int32_t)y) peakIdx = -1;
        }

        if (peakIdx >= 0) outIso = peakIndexToByte((uint32_t)peakIdx);
      }

      preakFindingData[(size_t)y * n + x] = outIso;
    }
  }

  peakFindingTexture.setData(preakFindingData, n, n, 1);
}


void TransferFunction::updateTexture()
{
  const uint32_t n = static_cast<uint32_t>(rawData.size());
  if (n == 0) {
    textureDirty = false;
    return;
  }

  std::vector<GLubyte> raw(4 * n);
  for (uint32_t i = 0; i < n; ++i)
  {
    const Vec4& c = rawData[i];

    raw[4 * i + 0] = toByte(c.r);
    raw[4 * i + 1] = toByte(c.g);
    raw[4 * i + 2] = toByte(c.b);
    raw[4 * i + 3] = toByte(c.a);
  }

  texture.setData(raw, n, 4);

  if (usePreIntegration) updatePreIntTexture();
  if (usePeakFinding) updatePeakFindingTexture();

  textureDirty = false;
}

void TransferFunction::setValue(float pos, float value, Channel channel) {
  const size_t iPos = size_t(clamp01(pos)*(rawData.size()-1));
  setValue(iPos, value, channel);
}

void TransferFunction::setValue(size_t pos, float value, Channel channel) {
  switch (channel) {
    case Channel::R: rawData[pos].r = clamp01(value); break;
    case Channel::G: rawData[pos].g = clamp01(value); break;
    case Channel::B: rawData[pos].b = clamp01(value); break;
    case Channel::A: rawData[pos].a = clamp01(value); break;
  }
  textureDirty = true;
}
