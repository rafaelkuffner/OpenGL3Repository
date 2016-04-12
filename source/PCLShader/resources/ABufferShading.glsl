/**
 * Fast Single-pass A-Buffer using OpenGL 4.0 V2.0
 * Copyright Cyril Crassin, July 2010
**/
#ifndef ABUFFERSHADING_HGLSL
#define ABUFFERSHADING_HGLSL

#include "ABufferSort.glsl"

const vec4 backgroundColor=vec4(BACKGROUND_COLOR_R, BACKGROUND_COLOR_G, BACKGROUND_COLOR_B, 0.0f);


//Blend fragments front-to-back
vec4 resolveAlphaBlend(int abNumFrag,int abSortedFrags){
			
	vec4 finalColor=vec4(0.0f);

	const float sigma = 30.0f;
	float thickness=fragmentList[0].w/2.0f;
	if(cleanframes < 5){
			finalColor = fragmentList[0];
			finalColor.a = 1;
			return finalColor;
	}
	for(int i=0; i<abSortedFrags; i++){
		vec4 frag=fragmentList[i];
		vec4 col=frag;
			//uses constant alpha

#if ABUFFER_RESOLVE_ALPHA_CORRECTION
		if(i%2==abNumFrag%2)
			thickness=(zList[i+1]-frag.w)*0.5f;
		col.w=1.0f-pow(1.0f-col.w, thickness* sigma );
#endif

		col.rgb=col.rgb*col.w;

		finalColor=finalColor+col*(1.0f-finalColor.a);
		
	}

	finalColor=finalColor+backgroundColor*(1.0f-finalColor.a);

	return finalColor;

}


#endif