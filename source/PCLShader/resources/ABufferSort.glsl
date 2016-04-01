/**
 * Fast Single-pass A-Buffer using OpenGL 4.0 V2.0
 * Copyright Cyril Crassin, July 2010
**/
#ifndef ABUFFERSORT_HGLSL
#define ABUFFERSORT_HGLSL

//Local memory array (probably in L1)
vec4 fragmentList[ABUFFER_SIZE];
float zList[ABUFFER_SIZE];


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