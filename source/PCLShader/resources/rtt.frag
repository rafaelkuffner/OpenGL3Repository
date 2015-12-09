#version 150


uniform sampler2D texture_color;

in vec2 texcoord;
out vec4 outColor;


void main(){
		outColor = texture(texture_color, texcoord);
}