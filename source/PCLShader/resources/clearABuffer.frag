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
#define ABUFFER_RESOLVE_USE_SORTING 0
#define BACKGROUND_COLOR_B 1.000000f
#define BACKGROUND_COLOR_G 1.000000f
#define BACKGROUND_COLOR_R 1.000000f
#define ABUFFER_SIZE 16
#define ABUFFER_PAGE_SIZE 4


uniform vec4 *d_abuffer;
uniform vec4 *d_abufferZ;
uniform uint *d_abufferIdx;

void main(void) {

	ivec2 coords=ivec2(gl_FragCoord.xy);
	
	//Be sure we are into the framebuffer
	if(coords.x>=0 && coords.y>=0 && coords.x<SCREEN_WIDTH && coords.y<SCREEN_HEIGHT ){
		d_abufferIdx[coords.x+coords.y*SCREEN_WIDTH]=0;
		for(int i = 0; i < ABUFFER_SIZE; i++){
			d_abufferZ[coords.x+coords.y*SCREEN_WIDTH+ (i*SCREEN_WIDTH*SCREEN_HEIGHT)]=vec4(0.0f);
			d_abuffer[coords.x+coords.y*SCREEN_WIDTH+ (i*SCREEN_WIDTH*SCREEN_HEIGHT)]=vec4(0.0f);
		}
	}

	//Discard fragment so nothing is writen to the framebuffer
	discard;
}
