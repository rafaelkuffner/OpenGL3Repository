#version 150

uniform mat4 camera;
uniform mat4 model;
uniform vec3 camPosition;
uniform float resolution;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;


in Vertex
{
    vec4 color;
	vec4 normal;
} vertex[];
     
out vec2 VertexUV;
out vec4 VertexColor;

void main() {
   float dist = distance(camPosition,gl_in[0].gl_Position.xyz);
   //a variável fora da exponencial diz quantas patches vão ter quando a distância for 0,
   // o número da exponencial tem a ver com a distância a qual ele vai ser 1. 
   float tiling =1+(0*(pow(20,-dist)));
   //se quiser desativar o tiling, é aqui.
   //tiling = 1;
   //float size = 0.002;
   float size = 1.5*resolution;
	vec3 right;
	vec3 up;
	vec4 c = vertex[0].color;
	vec3 normal = vertex[0].normal.xyz;
			
	float nx = vertex[0].normal.x;
	float ny = vertex[0].normal.y;
	float nz = vertex[0].normal.z;
	float z1;
	float z2;
	float x1;
	float x2;

	x1 = sqrt(1/(1+(pow(nx,2)/pow(nz,2))));
	x2 = -x1;
	z1 = - nx*x1/nz;
	z2 = -z1;
	
	  right = vec3(x1,0,z1);
	  up = cross(right,normal);
	  
	  
	
	  
    vec3 P = gl_in[0].gl_Position.xyz;
     
     
    vec3 va = P - (right + up) * size;
    gl_Position =  camera * model *vec4(va, 1.0);
    VertexUV = vec2(0.0, 0.0);
    VertexColor = c;
    EmitVertex();  
      
    vec3 vb = P - (right - up) * size;
    gl_Position = camera * model *vec4(vb, 1.0);
    VertexUV = vec2(0.0, tiling);
    VertexColor = c;
    EmitVertex();  
     
    vec3 vd = P + (right - up) * size;
    gl_Position = camera * model *vec4(vd, 1.0);
    VertexUV = vec2(tiling, 0.0);
    VertexColor = c;
    EmitVertex();  
     
    vec3 vc = P + (right + up) * size;
    gl_Position =camera * model * vec4(vc, 1.0);
    VertexUV = vec2(tiling, tiling);
    VertexColor = c;
    EmitVertex();  
      
    EndPrimitive();  
}