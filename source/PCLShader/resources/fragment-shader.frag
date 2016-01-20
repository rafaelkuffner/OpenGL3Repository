#version 150

in vec4 VertexColor;
in vec2 VertexUV;
flat in int tex;
in float rr;
in float rg;
in float rb;

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;
uniform sampler2D tex5;
uniform sampler2D tex6;
uniform sampler2D tex7;
uniform sampler2D tex8;
uniform sampler2D tex9;
uniform sampler2D tex10;

uniform float alph;
uniform float saturation;
out vec4 finalColor;


void main() { 
	vec2 uv = VertexUV.xy;
	vec4 t;
	if(tex==0) t= texture(tex0,uv);
	else if(tex == 1) t= texture(tex1,uv);
	else if(tex == 2) t= texture(tex2,uv);
	else if(tex == 3) t= texture(tex3,uv);
	else if(tex == 4) t= texture(tex4,uv);
	else if(tex == 5) t= texture(tex5,uv);
	else if(tex == 6) t= texture(tex6,uv);
	else if(tex == 7) t= texture(tex7,uv);
	else if(tex == 8) t= texture(tex8,uv);
	else if(tex == 9) t= texture(tex9,uv);

	vec3 normal;
	if(t.a != 0){
		t.a = t.a <0.2? 0.2:t.a*0.7;
		normal.x = dFdx(t.a);
		normal.y = dFdy(t.a);
		normal.z = sqrt(1 - normal.x*normal.x - normal.y * normal.y); // Reconstruct z component to get a unit normal.
		t.a = 1.0;
	}else{
		discard;
	}
	
	t = t*VertexColor;
	
	float  P=sqrt(t.r*t.r*0.299+t.g*t.g*0.587+t.b*t.b*0.114 ) ;

	t.r=P+((t.r)-P)*(saturation+0.3);
	t.g=P+((t.g)-P)*(saturation+0.3);
	t.b=P+((t.b)-P)*(saturation+0.3); 
	float cVr = (rr -0.5)*0.01;
	float cVg = (rg -0.5)*0.01;
	float cVb = (rb -0.5)*0.01;
	if(saturation != 1){
		t = vec4(alph*(t.r+cVr),alph*(t.g+cVg),alph*(t.b+cVb),alph);
		//pointilism orange
		//t = vec4(1+cVr,0.27+cVg,0+cVb,alph);
		//if(t.r > 1) t.r = 1;
		//if(t.b< 0) t.b = 0;
	}
	
	
	vec3 light_pos = normalize(vec3(1.0, 0.0, 1.5));  
	    // Calculate the lighting diffuse value  
    float diffuse = max(dot(normal, light_pos), 0.0);  
    vec3 color = diffuse * t.rgb;      
    // Set the output color of our current pixel   
	
	//finalColor = vec4(color,1.0);
	finalColor = t;
	
}