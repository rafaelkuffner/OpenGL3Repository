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

uniform float alph;
uniform float saturation;
uniform bool aBuffer;


uniform layout(size1x32) uimage2D abufferCounterImg;
uniform layout(size4x32) image2DArray abufferImg;
uniform layout(size4x32) image2DArray abufferZImg;

out vec4 finalColor;


vec4 sbrColor(){
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