#version 410

in vec3 tc;
out vec4 fc;

uniform sampler2D frontFaces;
uniform sampler3D volume;

uniform float sampleCount;
uniform float smoothStepStart;
uniform float smoothStepWidth;


void main() {
  vec3 currentPoint = texelFetch(frontFaces,ivec2(gl_FragCoord),0).xyz;
  vec3 exitPoint = tc;
  vec3 direction = normalize(exitPoint-currentPoint)/sampleCount;

  // TODO: implement raycaster here, write result into "fc"
  //       the following line is just some dummy code that uses
  //       all variables to avoid this empty start project from
  //       throwing an exception, it has to be replaced by the
  //       raycasting loop
  fc = sampleCount*smoothStepStart*smoothStepWidth*texture(volume,direction);
}
