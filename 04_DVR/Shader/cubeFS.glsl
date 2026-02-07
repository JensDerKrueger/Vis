in vec3 entryPoint;
out vec4 result;

uniform sampler3D volume;
uniform sampler1D transfer;

uniform float oversampling;
uniform vec3 cameraPosInTextureSpace;
uniform vec3 minBounds;
uniform vec3 maxBounds;

vec4 transferFunction(float v) {
  return texture(transfer, v);
}

vec4 under(vec4 current, vec4 last) {
  last.rgb = last.rgb + (1.0-last.a) * current.a * current.rgb;
  last.a   = last.a + (1.0-last.a) * current.a;
  return last;
}

bool inBounds(vec3 pos) {
  return pos.x >= minBounds.x && pos.y >= minBounds.y && pos.z >= minBounds.z &&
  pos.x <= maxBounds.x && pos.y <= maxBounds.y && pos.z <= maxBounds.z;
}

void main() {
  vec3 rayDirectionInTextureSpace = normalize(entryPoint-cameraPosInTextureSpace);

  // compute delta
  vec3 voxelCount = vec3(textureSize(volume,0));
  float samples = dot(abs(rayDirectionInTextureSpace),voxelCount);
  float opacityCorrection = 100/(samples*oversampling);
  vec3 delta = rayDirectionInTextureSpace/(samples*oversampling);

  vec3 currentPoint = entryPoint;

  // TODO: add raycaster here
  result = vec4(1.0);
}
