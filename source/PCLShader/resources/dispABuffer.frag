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

uniform layout(size1x32) uimage2D abufferCounterImg;
uniform layout(size1x32) uimage2D abufferCounterIncImg;
uniform layout(size4x32) image2DArray abufferImg;
uniform layout(size4x32) image2DArray abufferZImg;

uniform int cleanframes;

//Local memory array (probably in L1)
vec4 fragmentList[ABUFFER_SIZE];
float zList[ABUFFER_SIZE];

#include "ABufferSort.glsl"
#include "ABufferShading.glsl"


//Whole number pixel offsets (not necessary just to test the layout keyword !)
layout(pixel_center_integer) in vec4 gl_FragCoord;

//Input interpolated fragment position
smooth in vec4 fragPos;
//Output fragment color
out vec4 outFragColor;



//Fill local memory array of fragments
void fillFragmentArray(ivec2 coords, int abNumFrag);

//Resolve A-Buffer and blend sorted fragments
void main(void) {

	ivec2 coords=ivec2(gl_FragCoord.xy);
	
	if(coords.x>=0 && coords.y>=0 && coords.x<SCREEN_WIDTH && coords.y<SCREEN_HEIGHT ){

		//Load the number of fragments in the current pixel.

		int abNumFrag=(int)imageLoad(abufferCounterImg, coords).r;
		int abSortedFrags=(int)imageLoad(abufferCounterIncImg, coords).r;
		
		if(abNumFrag > 0){
			//Copy fragments in local array
			fillFragmentArray(coords, abNumFrag);
			////Sort fragments in local memory array
			abSortedFrags = bubbleSortIncremental(abNumFrag,abSortedFrags,coords);		
			////We want to sort and blend fragments
			outFragColor=resolveAlphaBlend(abNumFrag,abSortedFrags);
			imageStore(abufferCounterIncImg,coords, ivec4(abSortedFrags,0,0,0));
		}else{
			//If no fragment, write nothing
			discard;
		}
	}
}


void fillFragmentArray(ivec2 coords, int abNumFrag){
	//Load fragments into a local memory array for sorting
	for(int i=0; i<abNumFrag; i++){
		fragmentList[i]=imageLoad(abufferImg, ivec3(coords, i));
		zList[i] = imageLoad(abufferZImg, ivec3(coords, i)).z;
	}
}