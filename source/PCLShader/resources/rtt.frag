#version 150


uniform sampler2D texture_color;
uniform sampler2D texture_blur;

in vec2 texcoord;
out vec4 outColor;


void main(){
		vec4 texcol = texture(texture_color, texcoord);
		vec4 blurcol =  2*texture(texture_blur,texcoord);
		if(texcol == vec4(0,0,0,1))
			outColor =  blurcol;
		else
			outColor = (0.8*texcol + blurcol)/2;
}