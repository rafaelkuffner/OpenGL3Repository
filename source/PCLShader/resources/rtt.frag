#version 150


uniform sampler2D texture_color;
uniform int screen_width, screen_height;
uniform int c;

in vec2 texcoord;
out vec4 outColor;

vec3 blur(){
	float dx = 1.0f/screen_width;
	float dy = 1.0f/screen_height;
	vec3 sum = vec3(0,0,0);
	for(int i=-5;i<5;i++) 
	for(int j=-5;j<5;j++) 
		sum += texture(texture_color, texcoord + vec2(i * dx, j  * dy)).xyz;
	return sum/50;
}

vec3 color(){
	return texture(texture_color, texcoord).xyz;
}

void main(){
	if(c==0)
		outColor = vec4(blur(),1);
	else if (c==1)
		outColor = vec4(color(), 1);
}