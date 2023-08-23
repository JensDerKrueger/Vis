#version 410

in vec3 tc;
out vec4 fc;

uniform sampler3D volume;

uniform float smoothStepStart;
uniform float smoothStepWidth;
uniform float oversampling;
uniform vec3 voxelCount;
uniform mat4 v2t;

float maxValue(vec3 v) {
  return max(v.x, max(v.y, v.z));
}

float minPositive(float a, float b) {
  if (a < 0.005) return b;
  if (b < 0.005) return a;
  return min(a,b);
}

float minPositive(vec3 a, vec3 b) {
  vec3 c = vec3(minPositive(a.x, b.x),minPositive(a.y, b.y),minPositive(a.z, b.z));
  return minPositive(c.x, minPositive(c.y, c.z));
}

vec4 transferFunction(float v) {
  v = clamp((v - smoothStepStart) / (smoothStepWidth), 0.0, 1.0);
  return vec4(v*v * (3-2*v));
}

vec4 under(vec4 current, vec4 last) {
  last.rgb = last.rgb + (1.0-last.a) * current.a * current.rgb;
  last.a   = last.a + (1.0-last.a) * current.a;
  return last;
}

bool inBounds(vec3 pos) {
  return pos.x >= 0.0 && pos.y >= 0.0 && pos.z >= 0.0 &&
  pos.x <= 1.0 && pos.y <= 1.0 && pos.z <= 1.0;
}

void main() {
  // exit point is given by the backface geometry
  vec3 exitPoint = tc;

  // compute vector to camera in texture space
  vec3 cameraPosInTextureSpace = (v2t * vec4(0,0,0,1)).xyz;
  vec3 rayDirectionInTextureSpace = normalize(cameraPosInTextureSpace-exitPoint);

  // compute entry point and direction
  vec3 a = (0-exitPoint)/rayDirectionInTextureSpace;
  vec3 b = (1-exitPoint)/rayDirectionInTextureSpace;
  float t = minPositive(a,b);
  vec3 currentPoint = exitPoint + rayDirectionInTextureSpace * t;
  float samples = maxValue(abs(rayDirectionInTextureSpace)*t*voxelCount);
  float opacityCorrection = 100/(samples*oversampling);
  vec3 direction = normalize(exitPoint-currentPoint)/(samples*oversampling);

  fc = vec4(0.0);
  do {

    // TODO: implement raycaster here, write result into "fc"
    //       the following two liens are just dummy code that uses
    //       all variables to avoid this empty start project from
    //       throwing an exception, it has to be replaced by the
    //       raycasting loop
    fc = 1+transferFunction(texture(volume,direction).r);
    break;


  } while (inBounds(currentPoint));


}
