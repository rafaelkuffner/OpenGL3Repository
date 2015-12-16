#version 150


uniform sampler2D tex;
uniform sampler2D brushMap;
uniform sampler2D blurtex;

uniform int gridSize;
uniform float width;
uniform float height;

in vec2 texcoord;
out vec4 finalColor;


void main(){
	float bx = gl_FragCoord.x/width;
	float by = gl_FragCoord.y/height;
	float coordx = floor(gl_FragCoord.x/gridSize) / (width/gridSize);
	float coordy = floor(gl_FragCoord.y/gridSize) / (height/gridSize);
	float texx = mod(gl_FragCoord.x,gridSize)/gridSize;
	float texy = mod(gl_FragCoord.y,gridSize)/gridSize;

	vec4 blurColor = texture(blurtex,vec2(bx,by));
	vec4 brushColor = texture(brushMap, vec2(coordx,coordy));
	vec4 texColor = texture(tex, vec2(texx,texy));
	float threshold =0.03;
	if(texColor.a<0.5){
		finalColor = blurColor;
	}else if(brushColor != vec4(0,0,0,1) && distance(brushColor,blurColor) > threshold){
		finalColor = (brushColor+blurColor)/2;
		
	}else{
		finalColor = blurColor;
	}
}