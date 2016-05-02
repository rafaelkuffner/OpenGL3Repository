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

in float theta1;
in float theta2;
in float theta3;

uniform float dev;
uniform float gamma;
uniform float sigmax;
uniform float sigmay;
uniform float a;


uniform float alph;
uniform float saturation;
uniform bool aBuffer;


uniform layout(size1x32) uimage2D abufferCounterImg;
uniform layout(size4x32) image2DArray abufferImg;
uniform layout(size4x32) image2DArray abufferZImg;

out vec4 finalColor;


float gaussian(float x, float x0, float y,float y0, float a, float sigmax, float sigmay, float gamma){
	return a*exp(-0.5 *( pow(pow(( x -x0)/sigmax,2),gamma/2)+pow(pow(( y -y0)/sigmay,2),gamma/2)));
}

float gaussianTheta(float x, float x0, float y,float y0, float a, float sigmax, float sigmay, float gamma,float theta){
	//float a1 = pow(cos(theta),2)/(2*pow(sigmax,2)) + pow(sin(theta),2)/(2*pow(sigmay,2));
	//float b1 = pow(sin(2*theta),2)/(4*pow(sigmax,2)) + pow(sin(2*theta),2)/(4*pow(sigmay,2));
	//float c1 = pow(sin(theta),2)/(2*pow(sigmax,2)) + pow(cos(theta),2)/(2*pow(sigmay,2));

	//return  a * exp(- (a1*pow(x-x0,2) - (2*b1*(x-x0)*(y-y0)) + c1*pow(y-y0,2))) ;

	float x2 = cos(theta)*(x-x0)-sin(theta)*(y-y0)+x0;
	float y2 = sin(theta)*(x-x0)+cos(theta)*(y-y0)+y0;
	float z2 = a*exp(-0.5*( pow(pow((x2-x0)/sigmax,2),gamma/2)))*exp(-0.5*( pow(pow((y2-y0)/sigmay,2),gamma/2)));
	return z2;
}

vec4 sbrColor9Gaussian(){
	vec2 uv = VertexUV.xy;
	
	//------Brush stroke generation------//

	//ycenters and xcenters
	float xc[9]= {0.25f,0.5f,0.75f,0.25f,0.5f,0.75f,0.25f,0.5f,0.75f};
	float yc[9]= {0.25f,0.25f,0.25f,0.5f,0.5f,0.5f,0.75f,0.75f,0.75f};
	//float a =1.0f;
	//float sigmax = 0.15f; float sigmay = 0.12f;
	// float gamma =4;
	// float dev = 0.12;
	float alpha = 0;

	float a1 =gaussianTheta(uv.x, xc[0],uv.y,yc[0]+dev-theta1/6,a,sigmax,sigmay,gamma,theta1);
	float a2 =gaussianTheta(uv.x, xc[1],uv.y,yc[1]+dev-theta2/6,a*0.9,sigmax,sigmay,gamma,theta2); 
	float a3 =gaussianTheta(uv.x, xc[2],uv.y,yc[2]+dev-theta3/6,a*0.8,sigmax,sigmay,gamma,theta3); 
	float a4 =gaussianTheta(uv.x, xc[3],uv.y,yc[3]-theta1/6,a,sigmax,sigmay,gamma,theta1);
	float a5 =gaussianTheta(uv.x, xc[4],uv.y,yc[4]-theta2/6,a*0.9,sigmax,sigmay,gamma,theta2);
	float a6 =gaussianTheta(uv.x, xc[5],uv.y,yc[5]-theta3/6,a*0.8,sigmax,sigmay,gamma,theta3);
	float a7 =gaussianTheta(uv.x, xc[6],uv.y,yc[6]-dev-theta1/6,a,sigmax,sigmay,gamma,theta1);
	float a8 =gaussianTheta(uv.x, xc[7],uv.y,yc[7]-dev-theta2/6,a*0.9,sigmax,sigmay,gamma,theta2);
	float a9 =gaussianTheta(uv.x, xc[8],uv.y,yc[8]-dev-theta3/6,a*0.8,sigmax,sigmay,gamma,theta3);

	//alpha = max(max(max(max(max(max(max(max(a1,a2),a3),a4),a5),a6),a7),a8),a9);
	alpha= a1+a2+a3+a4+a5+a6+a7+a8+a9;
//	alpha = gaussianTheta(uv.x, xc[4],uv.y,yc[4],a,sigmax,sigmay,gamma,0.7853981634);
	//----------------------------------//

	alpha = alpha>1? 1:alpha;
	alpha = alpha < 1? alpha*1:alpha;
	alpha = alpha < 0.01? 0:alpha;
	vec4 t = vec4(1.0f,1.0f,1.0f,alpha);
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

vec4 sbrColor6Gaussian(){
	vec2 uv = VertexUV.xy;
	
	//------Brush stroke generation------//

	//ycenters and xcenters
	float xc[9]= {0.25f,0.5f,0.75f,0.25f,0.5f,0.75f,0.25f,0.5f,0.75f};
	float yc[9]= {0.25f,0.25f,0.25f,0.5f,0.5f,0.5f,0.75f,0.75f,0.75f};
	//float a =1.0f;
	//float sigmax = 0.15f; float sigmay = 0.12f;
	// float gamma =4;
	// float dev = 0.12;
	float alpha = 0;

	float a1 =gaussianTheta(uv.x, xc[0],uv.y,yc[0]+dev-theta1/6,a,sigmax,sigmay,gamma,theta1);
	float a2 =gaussianTheta(uv.x, xc[1],uv.y,yc[1]+dev-theta2/6,a*0.9,sigmax,sigmay,gamma,theta2); 
	//float a3 =gaussianTheta(uv.x, xc[2],uv.y,yc[2]+dev-theta3/6,a*0.8,sigmax,sigmay,gamma,theta3); 
	float a4 =gaussianTheta(uv.x, xc[3],uv.y,yc[3]-theta1/6,a,sigmax,sigmay,gamma,theta1);
	//float a5 =gaussianTheta(uv.x, xc[4],uv.y,yc[4]-theta2/6,a*0.9,sigmax,sigmay,gamma,theta2);
	float a6 =gaussianTheta(uv.x, xc[5],uv.y,yc[5]-theta3/6,a*0.8,sigmax,sigmay,gamma,theta3);
	float a7 =gaussianTheta(uv.x, xc[6],uv.y,yc[6]-dev-theta1/6,a,sigmax,sigmay,gamma,theta1);
	float a8 =gaussianTheta(uv.x, xc[7],uv.y,yc[7]-dev-theta2/6,a*0.9,sigmax,sigmay,gamma,theta2);
	//float a9 =gaussianTheta(uv.x, xc[8],uv.y,yc[8]-dev-theta3/6,a*0.8,sigmax,sigmay,gamma,theta3);

	//alpha = max(max(max(max(max(max(max(max(a1,a2),a3),a4),a6),a8);
	alpha= a1+a2+a4+a6+a7+a8;
//	alpha = gaussianTheta(uv.x, xc[4],uv.y,yc[4],a,sigmax,sigmay,gamma,0.7853981634);
	//----------------------------------//

	alpha = alpha>1? 1:alpha;
	alpha = alpha < 1? alpha*1:alpha;
	alpha = alpha < 0.01? 0:alpha;
	vec4 t = vec4(1.0f,1.0f,1.0f,alpha);
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
	//float  P=sqrt(t.r*t.r*0.299+t.g*t.g*0.587+t.b*t.b*0.114 ) ;

	float  P=sqrt(t.r) ;

	t.r=P+((t.r)-P)*(saturation+0.3);
	t.g=0;
	t.b=0; 

	return  t;
}





void main() { 
	if(aBuffer){
		ivec2 coords=ivec2(gl_FragCoord.xy);
		if(coords.x>=0 && coords.y>=0 && coords.x<SCREEN_WIDTH && coords.y<SCREEN_HEIGHT ){

			int abidx=(int)imageAtomicIncWrap(abufferCounterImg, coords, ABUFFER_SIZE );
	
			//Create fragment to be stored
				
			vec4 col = sbrColor6Gaussian();
			imageStore(abufferImg, ivec3(coords, abidx), col);
			imageStore(abufferZImg, ivec3(coords, abidx), fragPos);	
		}
		discard;
	}
	else{
		//finalColor = sbrColor9Gaussian();
		finalColor = sbrColor6Gaussian();
	}
}