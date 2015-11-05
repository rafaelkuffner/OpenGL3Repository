#version 150

in vec4 vert;
in vec4 norm;
in vec4 col;

out Vertex
{
    vec4 color;
	vec4 normal;
} vertex;
   
void main() {
    // Apply all matrix transformations to vert
    gl_Position =  vert;
    vertex.color = col;
	vertex.normal = norm;
}
