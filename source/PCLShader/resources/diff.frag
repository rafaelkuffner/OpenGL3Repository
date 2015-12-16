#version 150

uniform sampler2D origImage;
uniform sampler2D blurImage;
uniform int gridSize;
uniform float width;
uniform float height;

out vec4 BrushColor;


void main(void)
{		
	float coordx = gl_FragCoord.x*gridSize / width;
	float coordy = gl_FragCoord.y *gridSize/ height;
	coordx = coordx > 0.999? 0.999: coordx;
	coordy = coordy > 0.999? 0.999: coordy;
	coordx = coordx < 0.001? 0.001: coordx;
	coordy = coordy < 0.001? 0.001: coordy;
	vec2 bigCoord = vec2(coordx,coordy);
    vec4 maxColor = vec4(0,0,0,1);
	float maxDist= 0;
    for (int i=0; i<gridSize; i++) {
		 for (int j=0; j<gridSize; j++) {
			vec4 bC = texture( blurImage, bigCoord + vec2(i/width,j/height));
			vec4 oC = texture( origImage, bigCoord + vec2(i/width,j/height));
			float dist=  distance(bC ,oC);
			if(dist > maxDist ){
				maxDist = dist;
				maxColor= oC;
			}
		}
	}
	BrushColor = maxColor;
}