#version 150

in vec4 VertexColor;
in vec2 VertexUV;

out vec4 finalColor;

void main() {
    finalColor = VertexColor;
}