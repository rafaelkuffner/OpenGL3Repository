#version 150

in vec4 VertexColor;
in vec2 VertexUV;
flat in int tex;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;
uniform sampler2D tex5;
uniform sampler2D tex6;
uniform sampler2D tex7;
uniform sampler2D tex8;
uniform sampler2D tex9;
uniform sampler2D tex10;

out vec4 finalColor;

void main() { 
	vec2 uv = VertexUV.xy;
	vec4 t;
	if(tex==0) t= texture(tex0,uv);
	else if(tex == 1) t= texture(tex1,uv);
	else if(tex == 2) t= texture(tex2,uv);
	else if(tex == 3) t= texture(tex3,uv);
	else if(tex == 4) t= texture(tex4,uv);
	else if(tex == 5) t= texture(tex5,uv);
	else if(tex == 6) t= texture(tex6,uv);
	else if(tex == 7) t= texture(tex7,uv);
	else if(tex == 8) t= texture(tex8,uv);
	else if(tex == 9) t= texture(tex9,uv);

	//vec3 normal;
	//normal.x = dFdx(t.a);
	//normal.y = dFdy(t.a);
	//normal.z = sqrt(1 - normal.x*normal.x - normal.y * normal.y); // Reconstruct z component to get a unit normal.

	if(t.a== 0) discard;
	else t.a = 1;

	//vec3 light_pos = normalize(vec3(1.0, 0.0, 1.5));  
	//    // Calculate the lighting diffuse value  
 //   float diffuse = max(dot(normal, light_pos), 0.0);  
 //   vec3 color = diffuse * VertexColor.rgb;      
 //   // Set the output color of our current pixel   
	
	//finalColor = vec4(color,1.0);
	finalColor = t* VertexColor;
	
}