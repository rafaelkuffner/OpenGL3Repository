#version 150

uniform mat4 camera;
uniform mat4 model;
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
   float size = 0.0005;
	vec3 zeroup = vec3(0,0,1);
	vec3 zeroup2 = vec3(0,1,0);
	vec3 zeroup3 = vec3(1,0,0);
	vec3 right;
	vec3 up;
	vec4 c = vertex[0].color;
	vec3 normal = vertex[0].normal.xyz;

		
	  float nx = vertex[0].normal.x;
	  float ny = vertex[0].normal.y;
	  float nz = vertex[0].normal.z;
	  
	  float b = - 2 * ny * nz;
	  float a1 = - pow(nx,2) - pow(ny,2);
	  float c1 = pow(nx,2) - pow(nz,2);

	  float a2 = - pow(nx,2) - pow(ny,2);
	  float c2 = -pow(nx,2) - pow(nz,2);


		float delta1 = pow(b,2) - 4*a1*c1;
		float delta2 = pow(b,2) - 4*a2*c2;
	 
		float y1;
		float y2;

		if(delta1>0 && a1 >=0.5){
			y1 = (-b + sqrt(delta1))/(2*a1);
			y2 = (-b - sqrt(delta1))/(2*a1);
		}else if(delta2 >0 && a1 >=0.5){
			y1 = (-b + sqrt(delta2))/(2*a2);
			y2 = (-b - sqrt(delta2))/(2*a2);
		}else{
			right = vec3(1,0,0);
			up = vec3(0,1,0);
		}
	
	  float x1 = sqrt(1 - pow(y1, 2));
	  float x2 = sqrt(1 - pow(y2, 2));
	  
	  right = vec3(x1,y1,0);
	  up = cross(right,normal);

     
      vec3 P = gl_in[0].gl_Position.xyz;
     
     
      vec3 va = P - (right + up) * size;
      gl_Position =  camera * model *vec4(va, 1.0);
      VertexUV = vec2(0.0, 0.0);
      VertexColor = c;
      EmitVertex();  
      
      vec3 vb = P - (right - up) * size;
      gl_Position = camera * model *vec4(vb, 1.0);
      VertexUV = vec2(0.0, 1.0);
      VertexColor = c;
      EmitVertex();  
     
      vec3 vd = P + (right - up) * size;
      gl_Position = camera * model *vec4(vd, 1.0);
      VertexUV = vec2(1.0, 0.0);
      VertexColor = c;
      EmitVertex();  
     
      vec3 vc = P + (right + up) * size;
      gl_Position =camera * model * vec4(vc, 1.0);
      VertexUV = vec2(1.0, 1.0);
      VertexColor = c;
      EmitVertex();  
      
      EndPrimitive();  
}