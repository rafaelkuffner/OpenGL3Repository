#version 150

uniform mat4 camera;
uniform mat4 model;
layout(points) in;
layout(line_strip, max_vertices = 4) out;


in Vertex
{
    vec4 color;
	vec4 normal;
} vertex[];
     
out vec2 VertexUV;
out vec4 VertexColor;

void main() {
   float size = 0.009;
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
     
	 vec4 col;
	 if( ny<0.1 && ny < 0.1){
		col =vec4(1,1,1,1);
	 }else{
		col= vec4(nx,ny,nz,1);
	 }
	 gl_Position =  camera * model *vec4(P,1.0);
      VertexUV = vec2(0.0, 0.0);
      VertexColor = vec4(nx,ny,nz,1);
      EmitVertex();  
     
	  vec3 P2 = gl_in[0].gl_Position.xyz + normal.xyz*0.007;
	  gl_Position =  camera * model *vec4(P2,1.0);
      VertexUV = vec2(0.0, 0.0);
      VertexColor = vec4(nx,ny,nz,1);
      EmitVertex();

      EndPrimitive();  
      	 
     gl_Position =  camera * model *vec4(P,1.0);
      VertexUV = vec2(0.0, 0.0);
      VertexColor = vertex[0].color;
      EmitVertex();  
     
	   P2 = gl_in[0].gl_Position.xyz + right.xyz*0.007;
	  gl_Position =  camera * model *vec4(P2,1.0);
      VertexUV = vec2(0.0, 0.0);
      VertexColor = vertex[0].color;
      EmitVertex();
      EndPrimitive();  

	  // gl_Position =  camera * model *vec4(P,1.0);
   //   VertexUV = vec2(0.0, 0.0);
   //   VertexColor = vertex[0].color;
   //   EmitVertex();  
     
	  // P2 = gl_in[0].gl_Position.xyz + up.xyz*0.007;
	  //gl_Position =  camera * model *vec4(P2,1.0);
   //   VertexUV = vec2(0.0, 0.0);
   //   VertexColor =  vertex[0].color;
   //   EmitVertex();
   //   EndPrimitive();  
}