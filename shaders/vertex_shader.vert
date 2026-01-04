#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 instanceOffset;
layout(location = 2) in vec3 instanceColor;

uniform float size;

out vec3 fragColor;

void main() {
    vec2 scaled = aPos * size + instanceOffset;
    gl_Position = vec4(scaled, 0.0, 1.0);
    fragColor = instanceColor;
}
