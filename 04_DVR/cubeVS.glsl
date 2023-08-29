#version 410

uniform mat4 modelViewProjection;
uniform mat4 clip;
in vec3 vPos;
out vec3 entryPoint;

void main() {
  gl_Position = modelViewProjection * vec4(vPos, 1.0);
  entryPoint = (clip*vec4(vPos, 1.0)).xyz+0.5;
}
