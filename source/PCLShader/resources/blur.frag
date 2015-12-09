#version 150

uniform sampler2D image;

out vec4 FragmentColor;

uniform float offset[3] = float[]( 0.0, 1.3846153846, 3.2307692308 );
uniform float weight[3] = float[]( 0.2270270270, 0.3162162162, 0.0702702703 );

uniform float width;
uniform float height;
//1 == vert 0 == hori
uniform int d;

void main(void)
{		
	float coordx = gl_FragCoord.x / width;
	float coordy = gl_FragCoord.y / height;

    FragmentColor = texture( image, vec2(coordx,coordy))*weight[0];
    for (int i=1; i<=3; i++) {
		vec2 coord;
		if(d ==0) {
			coord = vec2(offset[i]/width ,0.0);
			}
		else {
			coord = vec2(0.0,offset[i]/height);
			}
        
		FragmentColor +=
            texture( image, vec2(coordx,coordy)+coord)* weight[i];
        FragmentColor +=
            texture( image, vec2(coordx,coordy)-coord) * weight[i];
    }
}