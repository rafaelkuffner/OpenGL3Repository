/**
 * Fast Single-pass A-Buffer using OpenGL 4.0 V2.0
 * Copyright Cyril Crassin, July 2010
**/
#ifndef ABUFFERSHADING_HGLSL
#define ABUFFERSHADING_HGLSL

#include "ABufferSort.glsl"

//const vec4 backgroundColor=vec4(BACKGROUND_COLOR_R, BACKGROUND_COLOR_G, BACKGROUND_COLOR_B, 0.0f);


//Blend fragments front-to-back
vec4 resolveAlphaBlend(int abNumFrag,int abSortedPrev, int abSortedFrags){
			

	const float sigma = 30.0f;
	float thickness=fragmentList[0].w/2.0f;
	int i = abSortedPrev;
	vec4 finalColor=vec4(0.0f);
	if(i>0) finalColor =fragmentList[i-1];
	i = i <0? 0:i;
	for(; i<abSortedFrags; i++){
		vec4 col=fragmentList[i];
		col.rgb=col.rgb*col.a;
		finalColor=finalColor+col*(1.0f-finalColor.a);	
	}

//	finalColor=finalColor+backgroundColor*(1.0f-finalColor.a);

	return finalColor;

}


#endif