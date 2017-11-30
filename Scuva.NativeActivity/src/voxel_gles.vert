#version 310 es
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 fragColor;

out gl_PerVertex {vec4 gl_Position;};

void main() {
    gl_Position = proj * view * model * vec4(pos, 1.0);
    fragColor = color;
}