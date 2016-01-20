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
     
out vec4 VertexColor;

void main() {
   float size = 0.009;
	vec3 right;
	vec3 up;
	vec4 c = vertex[0].color;
	vec3 normal = vertex[0].normal.xyz;

	 gl_Position =  camera * model *vec4(gl_in[0].gl_Position.xyz,1.0);
     VertexColor = c;
     EmitVertex();  
     
	 vec3 P2 = gl_in[0].gl_Position.xyz + normal.xyz*0.009;
	  gl_Position =  camera * model *vec4(P2,1.0);
      VertexColor = vec4(normal.x,normal.y,normal.z,1.0);
      EmitVertex();
	  EndPrimitive();  
	
	//float nx = vertex[0].normal.x;
	//float ny = vertex[0].normal.y;
	//float nz = vertex[0].normal.z;

	//float n = sqrt(pow(nx,2) + pow(ny,2) + pow(nz,2));
	//float h1 = max( nx - n , nx + n );
	//float h2 = ny;
	//float h3 = nz;
	//float h = sqrt(pow(h1,2) + pow(h2,2) + pow(h3,2));
	//right = vec3(-2*h1*h2/pow(h,2), 1 - 2*pow(h2,2)/pow(h,2), -2*h2*h3/pow(h,2));
	//up = vec3(-2*h1*h3/pow(h,2), -2*h2*h3/pow(h,2), 1 - 2*pow(h3,2)/pow(h,2));
 
	// gl_Position =  camera * model *vec4(gl_in[0].gl_Position.xyz,1.0);
 //    VertexColor = c;
 //    EmitVertex();  
     
	//  P2 = gl_in[0].gl_Position.xyz + right.xyz*0.002;
	//  gl_Position =  camera * model *vec4(P2,1.0);
 //     VertexColor = vec4(right.x,right.y,right.z,1.0);
 //     EmitVertex();
	//  EndPrimitive();  

	// gl_Position =  camera * model *vec4(gl_in[0].gl_Position.xyz,1.0);
 //    VertexColor = c;
 //    EmitVertex();  
     
	//  P2 = gl_in[0].gl_Position.xyz + up.xyz*0.002;
	//  gl_Position =  camera * model *vec4(P2,1.0);
 //     VertexColor = vec4(up.x,up.y,up.z,1.0);
 //     EmitVertex();
	//  EndPrimitive();


	//vec3 P = gl_in[0].gl_Position.xyz;
     
     
 //   vec3 va = P - (right + up) * size;
 //   gl_Position =  camera * model *vec4(va, 1.0);
 //   VertexColor = c;
 //   EmitVertex();  
    
	//vec3 vb = P - (right - up) * size;
 //   gl_Position = camera * model *vec4(vb, 1.0);
 //   VertexColor = c;
 //   EmitVertex();  


 //   vec3 vc = P + (right + up) * size;
 //   gl_Position =camera * model * vec4(vc, 1.0);
 //   VertexColor = c;
 //   EmitVertex();  

	//vec3 vd = P + (right - up) * size;
 //   gl_Position = camera * model *vec4(vd, 1.0);
 //   VertexColor = c;
 //   EmitVertex();

 //   gl_Position =  camera * model *vec4(va, 1.0);
 //   VertexColor = c;
 //   EmitVertex();   
      
 //   EndPrimitive();  

}