#version 450

#extension GL_NV_gpu_shader5 : enable
#extension GL_EXT_shader_image_load_store : enable
#extension GL_NV_shader_buffer_load : enable
#extension GL_EXT_bindable_uniform : enable
#extension GL_NV_shader_buffer_store : enable

//Macros changed from the C++ side
#define SCREEN_WIDTH	512
#define SCREEN_HEIGHT	512
#define ABUFFER_RESOLVE_ALPHA_CORRECTION 0
#define BACKGROUND_COLOR_B 1.000000f
#define BACKGROUND_COLOR_G 1.000000f
#define BACKGROUND_COLOR_R 1.000000f
#define ABUFFER_SIZE 16


coherent uniform layout(size1x32) uimage2D abufferCounterImg;
coherent uniform layout(size1x32) uimage2D abufferCounterIncImg;
coherent uniform layout(size4x32) image2DArray abufferImg;
coherent uniform layout(size4x32) image2DArray abufferZImg;


void main(void) {

	ivec2 coords=ivec2(gl_FragCoord.xy);
	
	if(coords.x>=0 && coords.y>=0 && coords.x<SCREEN_WIDTH && coords.y<SCREEN_HEIGHT ){
		//Reset counter
		imageStore(abufferCounterImg, coords, ivec4(0));
		imageStore(abufferCounterIncImg, coords, ivec4(0));
		////Put black in first layer
		//for(int i = 0; i < ABUFFER_SIZE;i++){
		imageStore(abufferImg, ivec3(coords, 0), vec4(0.0f));
		imageStore(abufferZImg, ivec3(coords, 0), vec4(0.0f));
		////}
	}


	//Discard fragment so nothing is writen to the framebuffer
	discard;
}
