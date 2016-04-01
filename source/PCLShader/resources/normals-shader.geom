#version 450

uniform mat4 camera;
uniform mat4 model;
//uniform vec3 camPosition;
uniform float resolution;
uniform float scale;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;


in Vertex
{
    vec4 color;
	vec4 normal;
} vertex[];
     
out vec2 VertexUV;
out vec4 VertexColor;
smooth out vec4 fragPos;
flat out int tex;


float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main() {
 //  float dist = distance(camPosition,gl_in[0].gl_Position.xyz);
   //a variável fora da exponencial diz quantas patches vão ter quando a distância for 0,
   // o número da exponencial tem a ver com a distância a qual ele vai ser 1. 
  // float tiling =1+(0*(pow(20,-dist)));
   //se quiser desativar o tiling, é aqui.
   float tiling = 1;
   //float size = 0.002;
  
   float size = resolution;
	vec3 right;
	vec3 up;
	vec4 c = vertex[0].color;
	vec3 normal = vertex[0].normal.xyz;
			
	float nx = vertex[0].normal.x;
	float ny = vertex[0].normal.y;
	float nz = vertex[0].normal.z;
	//float z1;
	//float z2;
	//float x1;
	//float x2;
	
	tex = (int(rand(vec2(nx,ny))*10000))%10;

	//x1 = sqrt(1/(1+(pow(nx,2)/pow(nz,2))));
	//x2 = -x1;
	//z1 = - nx*x1/nz;
	//z2 = -z1;
	
	//  right = vec3(x1,0,z1);
	//  up = cross(right,normal);
	  
	float n = sqrt(pow(nx,2) + pow(ny,2) + pow(nz,2));

	float h1 = max( nx - n , nx + n );

	float h2 = ny;

	float h3 = nz;

	float h = sqrt(pow(h1,2) + pow(h2,2) + pow(h3,2));

	right = vec3(-2*h1*h2/pow(h,2), 1 - 2*pow(h2,2)/pow(h,2), -2*h2*h3/pow(h,2));

	up = vec3(-2*h1*h3/pow(h,2), -2*h2*h3/pow(h,2), 1 - 2*pow(h3,2)/pow(h,2));
	  
	
	  
    vec3 P = gl_in[0].gl_Position.xyz + vertex[0].normal.xyz*(scale -1);
     
     
    vec3 va = P - (right + up) * size;
    fragPos =  camera * model *vec4(va, 1.0);
	gl_Position = fragPos;
    VertexUV = vec2(0.0, 0.0);
    VertexColor = c;
    EmitVertex();  
      
    vec3 vb = P - (right - up) * size;
    fragPos = camera * model *vec4(vb, 1.0);
	gl_Position = fragPos;
    VertexUV = vec2(0.0, tiling);
    VertexColor = c;
    EmitVertex();  
     
    vec3 vd = P + (right - up) * size;
    fragPos = camera * model *vec4(vd, 1.0);
	gl_Position = fragPos;
    VertexUV = vec2(tiling, 0.0);
    VertexColor = c;
    EmitVertex();  
     
    vec3 vc = P + (right + up) * size;
    fragPos =camera * model * vec4(vc, 1.0);
	gl_Position = fragPos;
    VertexUV = vec2(tiling, tiling);
    VertexColor = c;
    EmitVertex();  
      
    EndPrimitive();  
}