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
#define ABUFFER_RESOLVE_USE_SORTING 0
#define BACKGROUND_COLOR_B 1.000000f
#define BACKGROUND_COLOR_G 1.000000f
#define BACKGROUND_COLOR_R 1.000000f
#define ABUFFER_SIZE 16
#define ABUFFER_PAGE_SIZE 4

#include "ABufferSort.glsl"
#include "ABufferShading.glsl"


//Whole number pixel offsets (not necessary just to test the layout keyword !)
layout(pixel_center_integer) in vec4 gl_FragCoord;

//Input interpolated fragment position
smooth in vec4 fragPos;
//Output fragment color
out vec4 outFragColor;


uniform vec4 *d_abuffer;
uniform vec4 *d_abufferZ;
uniform uint *d_abufferIdx;

//Keeps only closest fragment
vec4 resolveClosest(ivec2 coords, int abNumFrag);
//Fill local memory array of fragments
void fillFragmentArray(ivec2 coords, int abNumFrag);

//Resolve A-Buffer and blend sorted fragments
void main(void) {

	ivec2 coords=ivec2(gl_FragCoord.xy);
	
	if(coords.x>=0 && coords.y>=0 && coords.x<SCREEN_WIDTH && coords.y<SCREEN_HEIGHT ){

		//Load the number of fragments in the current pixel.

		int abNumFrag=(int)d_abufferIdx[coords.x+coords.y*SCREEN_WIDTH];
			
		//Crash without this (WTF ??)
		if(abNumFrag<0 )
			abNumFrag=0;
		if(abNumFrag>ABUFFER_SIZE ){
			abNumFrag=ABUFFER_SIZE;
		}

		if(abNumFrag > 0){


			//Copute ans output final color for the frame buffer
#	if ABUFFER_RESOLVE_USE_SORTING==0	
			//If we only want the closest fragment
			outFragColor=resolveClosest(coords, abNumFrag);
#	else
			
			//Copy fragments in local array
			fillFragmentArray(coords, abNumFrag);
			////Sort fragments in local memory array
			bubbleSort(abNumFrag);		
			////We want to sort and blend fragments
			outFragColor=resolveAlphaBlend(abNumFrag);
#	endif
		}else{
			//If no fragment, write nothing
			discard;
		}
	}
}


vec4 resolveClosest(ivec2 coords, int abNumFrag){

	//Search smallest z
	vec4 minFrag=vec4(0.0f, 0.0f, 0.0f, 0.0f);
	float minZ = 100000.0f;
	for(int i=0; i<abNumFrag; i++){
		vec4 val=d_abuffer[coords.x+coords.y*SCREEN_WIDTH + (i*SCREEN_WIDTH*SCREEN_HEIGHT)];
		float valz=d_abufferZ[coords.x+coords.y*SCREEN_WIDTH + (i*SCREEN_WIDTH*SCREEN_HEIGHT)].z;
		if(valz<minZ){
			minFrag=val;
			minZ = valz;
		}
	}
	//Output final color for the frame buffer
	return minFrag;
}


void fillFragmentArray(ivec2 coords, int abNumFrag){
	//Load fragments into a local memory array for sorting
	for(int i=0; i<abNumFrag; i++){
		fragmentList[i]=d_abuffer[coords.x+coords.y*SCREEN_WIDTH + (i*SCREEN_WIDTH*SCREEN_HEIGHT)];
		zList[i] = d_abufferZ[coords.x+coords.y*SCREEN_WIDTH + (i*SCREEN_WIDTH*SCREEN_HEIGHT)].z;
	}
}