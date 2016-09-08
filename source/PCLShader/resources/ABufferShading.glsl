/**
 * Fast Single-pass A-Buffer using OpenGL 4.0 V2.0
 * Copyright Cyril Crassin, July 2010
**/
#ifndef ABUFFERSHADING_HGLSL
#define ABUFFERSHADING_HGLSL

#include "ABufferSort.glsl"

//const vec4 backgroundColor=vec4(BACKGROUND_COLOR_R, BACKGROUND_COLOR_G, BACKGROUND_COLOR_B, 0.0f);


//Blend fragments front-to-back
vec4 resolveAlphaBlend(int abNumFrag,int abSortedFrags){
			
	

	const float sigma = 30.0f;
	float thickness=fragmentList[0].w/2.0f;
	//int i=abSortedFrags-2;
	//i = i < 0? 0:i;
	//for(; i<abSortedFrags; i++){
	int idp = abSortedFrags -2;
	int id = abSortedFrags -1; 
	vec4 finalColor;
	if(idp < 0)
		finalColor =vec4(0.0f);
	else
		finalColor=fragmentList[idp];
	
	vec4 col=fragmentList[id];
	col.rgb=col.rgb*col.w;
	finalColor=finalColor+col*(1.0f-finalColor.a);
		
	//}

	//finalColor=finalColor+backgroundColor*(1.0f-finalColor.a);

	return finalColor;

}


#endif