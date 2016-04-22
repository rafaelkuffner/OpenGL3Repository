#version 450

#extension GL_NV_gpu_shader5 : enable
#extension GL_EXT_shader_image_load_store : enable
#extension GL_NV_shader_buffer_load : enable
#extension GL_EXT_bindable_uniform : enable
#extension GL_NV_shader_buffer_store : enable

//Macros changed from the C++ side
#define ABUFFER_USE_TEXTURES	1
#define SCREEN_WIDTH	512
#define SCREEN_HEIGHT	512
#define ABUFFER_RESOLVE_ALPHA_CORRECTION 0
#define BACKGROUND_COLOR_B 1.000000f
#define BACKGROUND_COLOR_G 1.000000f
#define BACKGROUND_COLOR_R 1.000000f
#define ABUFFER_SIZE 16

smooth in vec4 fragPos;
in vec4 VertexColor;
in vec2 VertexUV;
in vec3 upperRowRandom;
in vec3 lowerRowRandom;

uniform float alph;
uniform float saturation;
uniform bool aBuffer;


uniform layout(size1x32) uimage2D abufferCounterImg;
uniform layout(size4x32) image2DArray abufferImg;
uniform layout(size4x32) image2DArray abufferZImg;

out vec4 finalColor;


float gaussian(float x, float x0, float y,float y0, float a, float sigmax, float sigmay, int gamma){
	return a*exp(-0.5 *( pow(pow(( x -x0)/sigmax,2),gamma/2)+pow(pow(( y -y0)/sigmay,2),gamma/2)));
}

vec4 sbrColor(){
	vec2 uv = VertexUV.xy;
	
	//------Brush stroke generation------//

	//ycenters and xcenters
	float xc[9]= {0.3f,0.5f,0.7f,0.3f,0.5f,0.7f,0.3f,0.5f,0.7f};
	float yc[9]= {0.3f,0.3f,0.3f,0.5f,0.5f,0.5f,0.7f,0.7f,0.7f};
	float a = 7f;
	float sigmax = 0.09; float sigmay = 0.06f;
	int gamma =3;

	float alpha =  gaussian(uv.x, xc[0],uv.y,yc[0]+(lowerRowRandom.x)*0.11,a,sigmax,sigmay,gamma) +
				   gaussian(uv.x, xc[1],uv.y,yc[1]+(lowerRowRandom.y)*0.11,a,sigmax,sigmay,gamma) +
				   gaussian(uv.x, xc[2],uv.y,yc[2]+(lowerRowRandom.z)*0.11,a,sigmax,sigmay,gamma) +
				   gaussian(uv.x, xc[3],uv.y,yc[3],a,sigmax,sigmay,gamma) +
				   gaussian(uv.x, xc[4],uv.y,yc[4],a,sigmax,sigmay,gamma) +
				   gaussian(uv.x, xc[5],uv.y,yc[5],a,sigmax,sigmay,gamma) +
				   gaussian(uv.x, xc[6],uv.y,yc[6]-(upperRowRandom.x)*0.11,a,sigmax,sigmay,gamma) +
				   gaussian(uv.x, xc[7],uv.y,yc[7]-(upperRowRandom.y)*0.11,a,sigmax,sigmay,gamma) +
				   gaussian(uv.x, xc[8],uv.y,yc[8]-(upperRowRandom.z)*0.11,a,sigmax,sigmay,gamma);


	//----------------------------------//

	alpha = alpha>1? 1:alpha;
	alpha = alpha < 0.01? 0:alpha;
	vec4 t = vec4(1.0f,1.0f,1.0f,alpha);
	vec3 normal;
	if(t.a ==0 && !aBuffer){
		discard;
	}if(t.a ==0 && aBuffer){
		return vec4(0.0f);
	}
	t = t*VertexColor;
	if(aBuffer)
		t.a *= alph;
	else
		t.a = alph;
	float  P=sqrt(t.r*t.r*0.299+t.g*t.g*0.587+t.b*t.b*0.114 ) ;

	t.r=P+((t.r)-P)*(saturation+0.3);
	t.g=P+((t.g)-P)*(saturation+0.3);
	t.b=P+((t.b)-P)*(saturation+0.3); 
	
	return  t;
}


void main() { 
	if(aBuffer){
		ivec2 coords=ivec2(gl_FragCoord.xy);
		if(coords.x>=0 && coords.y>=0 && coords.x<SCREEN_WIDTH && coords.y<SCREEN_HEIGHT ){

			int abidx=(int)imageAtomicIncWrap(abufferCounterImg, coords, ABUFFER_SIZE );
	
			//Create fragment to be stored
				
			vec4 col = sbrColor();
			imageStore(abufferImg, ivec3(coords, abidx), col);
			imageStore(abufferZImg, ivec3(coords, abidx), fragPos);	
		}
		discard;
	}
	else{
		finalColor = sbrColor();
	}
}