#version 410

uniform mat4 mvp;
in vec3 vPos;
out vec3 entryPoint;

void main() {
  gl_Position = mvp * vec4(vPos, 1.0);
  entryPoint = vPos+0.5;
}
