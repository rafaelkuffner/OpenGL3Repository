#version 150

uniform mat4 camera;
uniform mat4 model;
layout(points) in;
layout(line_strip, max_vertices = 2) out;


in Vertex
{
    vec4 color;
	vec4 normal;
} vertex[];
     
out vec2 VertexUV;
out vec4 VertexColor;

void main() {

      vec3 P = gl_in[0].gl_Position.xyz;

      gl_Position =  camera * model *vec4(P,1.0);
      VertexUV = vec2(0.0, 0.0);
      VertexColor = vertex[0].color;
      EmitVertex();  
     
	  vec3 P2 = gl_in[0].gl_Position.xyz + vertex[0].normal.xyz*0.001;
	  gl_Position =  camera * model *vec4(P2,1.0);
      VertexUV = vec2(0.0, 0.0);
      VertexColor = vertex[0].color;
      EmitVertex();

      
      EndPrimitive();  
}