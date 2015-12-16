#version 150

in vec4 VertexColor;
in vec2 VertexUV;
flat in int tex;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;

out vec4 finalColor;

void main() { 
	vec2 uv = VertexUV.xy;
	vec4 t;
	if(tex==0) t= texture(tex0,uv);
	else if(tex == 1) t= texture(tex1,uv);
	else if(tex == 2) t= texture(tex2,uv);
	else if(tex == 3) t= texture(tex3,uv);
	else if(tex == 4) t= texture(tex4,uv);
	if(t.a<0.5) discard;
    //finalColor = VertexColor;
	finalColor = t* VertexColor;
}