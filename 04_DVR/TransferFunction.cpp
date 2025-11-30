// TransferFunction.cpp
#include "TransferFunction.h"

#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <cctype>

float TransferFunction::clamp01(float v) {
  if (v < 0.0f) return 0.0f;
  if (v > 1.0f) return 1.0f;
  return v;
}

RGBA TransferFunction::lerp(const RGBA& a, const RGBA& b, float t)
{
  RGBA r;
  r.r = a.r + (b.r - a.r) * t;
  r.g = a.g + (b.g - a.g) * t;
  r.b = a.b + (b.b - a.b) * t;
  r.a = a.a + (b.a - a.a) * t;
  return r;
}

TransferFunction::Channel TransferFunction::charToChannel(char c)
{
  switch (static_cast<char>(std::toupper(static_cast<unsigned char>(c))))
  {
    case 'R': return Channel::R;
    case 'G': return Channel::G;
    case 'B': return Channel::B;
    case 'A': return Channel::A;
    default:
      throw std::invalid_argument("TransferFunction::smoothStep: invalid channel");
  }
}

// Constructors
TransferFunction::TransferFunction(uint32_t size)
: data_(size)
, textureDirty_(true)
{
}

TransferFunction::TransferFunction(const std::string& filename)
: textureDirty_(true)
{
  load(filename);
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

  data_.resize(size);

  for (uint32_t i = 0; i < size; ++i)
  {
    float rgba[4];
    in.read(reinterpret_cast<char*>(rgba), sizeof(rgba));
    if (!in)
      throw std::runtime_error("TransferFunction::load: unexpected EOF");

    data_[i].r = rgba[0];
    data_[i].g = rgba[1];
    data_[i].b = rgba[2];
    data_[i].a = rgba[3];
  }

  textureDirty_ = true;
}

void TransferFunction::save(const std::string& filename) const
{
  std::ofstream out(filename, std::ios::binary);
  if (!out)
    throw std::runtime_error("TransferFunction::save: cannot open file " + filename);

  const char magic[4] = { 'T', 'F', 'N', '1' };
  out.write(magic, 4);

  uint32_t size = static_cast<uint32_t>(data_.size());
  out.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));

  for (uint32_t i = 0; i < size; ++i)
  {
    const float rgba[4] = {
      data_[i].r,
      data_[i].g,
      data_[i].b,
      data_[i].a
    };
    out.write(reinterpret_cast<const char*>(rgba), sizeof(rgba));
  }

  if (!out)
    throw std::runtime_error("TransferFunction::save: write error");
}

#ifndef __EMSCRIPTEN__

GLTexture1D& TransferFunction::getTexture()
{
  if (textureDirty_)
    updateTexture();
  return texture_;
}

const GLTexture1D& TransferFunction::getTexture() const
{
  if (textureDirty_)
    const_cast<TransferFunction*>(this)->updateTexture();
  return texture_;
}

#else

GLTexture2D& TransferFunction::getTexture()
{
  if (textureDirty_)
    updateTexture();
  return texture_;
}

const GLTexture2D& TransferFunction::getTexture() const
{
  if (textureDirty_)
    const_cast<TransferFunction*>(this)->updateTexture();
  return texture_;
}

#endif

void TransferFunction::smoothStep(Channel channel, float start, float width)
{
  if (data_.empty() || width <= 0.0f)
    return;

  const uint32_t n = static_cast<uint32_t>(data_.size());

  for (uint32_t i = 0; i < n; ++i)
  {
    float x = (n > 1) ? static_cast<float>(i) / static_cast<float>(n - 1) : 0.0f;

    float t = (x - start) / width;
    t = clamp01(t);
    // classic smoothstep: 3t^2 - 2t^3
    t = t * t * (3.0f - 2.0f * t);

    switch (channel)
    {
      case Channel::R: data_[i].r = t; break;
      case Channel::G: data_[i].g = t; break;
      case Channel::B: data_[i].b = t; break;
      case Channel::A: data_[i].a = t; break;
    }
  }

  textureDirty_ = true;
}

void TransferFunction::smoothStep(char channel, float start, float width)
{
  smoothStep(charToChannel(channel), start, width);
}

void TransferFunction::setData(const std::vector<RGBA>& data)
{
  data_ = data;
  textureDirty_ = true;
}

void TransferFunction::resample(uint32_t newSize)
{
  const uint32_t oldSize = static_cast<uint32_t>(data_.size());

  if (newSize == oldSize)
    return;

  if (newSize == 0)
  {
    data_.clear();
    textureDirty_ = true;
    return;
  }

  if (oldSize == 0)
  {
    // If we have no data, just create zeros
    data_.assign(newSize, RGBA{});
    textureDirty_ = true;
    return;
  }

  std::vector<RGBA> newData(newSize);

  if (newSize == 1)
  {
    newData[0] = data_.front();
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

      newData[i] = lerp(data_[idx0], data_[idx1], t);
    }
  }

  data_.swap(newData);
  textureDirty_ = true;
}

void TransferFunction::updateTexture()
{
  const uint32_t n = static_cast<uint32_t>(data_.size());
  if (n == 0)
  {
    textureDirty_ = false;
    return;
  }

  std::vector<GLubyte> raw(4 * n);
  for (uint32_t i = 0; i < n; ++i)
  {
    const RGBA& c = data_[i];

    auto toByte = [](float v) -> GLubyte
    {
      float clamped = v;
      if (clamped < 0.0f) clamped = 0.0f;
      if (clamped > 1.0f) clamped = 1.0f;
      int iv = static_cast<int>(clamped * 255.0f + 0.5f);
      if (iv < 0) iv = 0;
      if (iv > 255) iv = 255;
      return static_cast<GLubyte>(iv);
    };

    raw[4 * i + 0] = toByte(c.r);
    raw[4 * i + 1] = toByte(c.g);
    raw[4 * i + 2] = toByte(c.b);
    raw[4 * i + 3] = toByte(c.a);
  }

#ifndef __EMSCRIPTEN__
  // 1D texture: length = n
  texture_.setData(raw, n, 4);  // GLTexture1D API :contentReference[oaicite:0]{index=0}
#else
  // WebGL: use 2D texture with size n x 1
  texture_.setData(raw, n, 1, 4);  // GLTexture2D API :contentReference[oaicite:1]{index=1}
#endif

  textureDirty_ = false;
}
