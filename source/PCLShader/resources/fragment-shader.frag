#version 150
uniform sampler2D tex;

in vec4 VertexColor;
in vec2 VertexUV;

out vec4 finalColor;

void main() { 
	vec2 uv = VertexUV.xy;
  
    vec4 t = texture(tex,uv);
    finalColor = t* VertexColor;
}