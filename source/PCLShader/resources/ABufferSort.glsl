/**
 * Fast Single-pass A-Buffer using OpenGL 4.0 V2.0
 * Copyright Cyril Crassin, July 2010
**/
#ifndef ABUFFERSORT_HGLSL
#define ABUFFERSORT_HGLSL




//Bubble sort used to sort fragments
void bubbleSort(int array_size);
//Bitonic sort test
void bitonicSort( int n );


//Bubble sort used to sort fragments
void bubbleSort(int array_size) {
  for (int i = (array_size -2); i >= 0; --i) {
    for (int j = 0; j <= i; ++j) {
      if (zList[j] > zList[j+1]) {

		vec4 temp = fragmentList[j+1];
		fragmentList[j+1] = fragmentList[j];
		fragmentList[j] = temp;

		float tempf = zList[j+1];
		zList[j+1] = zList[j];
		zList[j] = tempf;
      }
    }
  }
}

//Bubble sort used to sort fragments
int bubbleSortIncremental(int array_size,int minvalue,vec2 coords) {
	bool sorted = true;
    for (int j = (array_size-1); j >minvalue; j--) {
      if (zList[j] < zList[j-1]) {
		sorted = false;
		vec4 higher = fragmentList[j-1];
		vec4 lower = fragmentList[j];
		fragmentList[j-1] = lower;
		fragmentList[j] = higher;

		imageStore(abufferImg, ivec3(coords, j-1), lower);
		imageStore(abufferImg, ivec3(coords, j), higher);

		float higherf = zList[j-1];
		float lowerf = zList[j];

		zList[j-1] = lowerf;
		zList[j] = higherf;

		imageStore(abufferZImg, ivec3(coords, j-1),  vec4(0.0f,0.0f,lowerf,0.0f));	
		imageStore(abufferZImg, ivec3(coords, j), vec4(0.0f,0.0f,higherf,0.0f));	
      }
   }
   if(sorted)
		return array_size;
	else
		return minvalue +1;
}

//Swap function
void swapFragArray(int n0, int n1){
	vec4 temp = fragmentList[n1];
	fragmentList[n1] = fragmentList[n0];
	fragmentList[n0] = temp;

	float tempf = zList[n1];
		zList[n1] = zList[n0];
		zList[n0] = tempf;
}

//->Same artefact than in L.Bavoil
//Bitonic sort: http://www.tools-of-computing.com/tc/CS/Sorts/bitonic_sort.htm
void bitonicSort( int n ) {
	int i,j,k;
	for (k=2;k<=n;k=2*k) {
		for (j=k>>1;j>0;j=j>>1) {
			for (i=0;i<n;i++) {
			  int ixj=i^j;
			  if ((ixj)>i) {
				  if ((i&k)==0 &&zList[i]>zList[ixj]){
					swapFragArray(i, ixj);
				  }
				  if ((i&k)!=0 && zList[i]<zList[ixj]) {
					swapFragArray(i, ixj);
				  }
			  }
			}
		}
	}
}


#endif