#version 450

in vec4 vertexPos;

smooth out vec4 fragPos;

void main(){
	fragPos=vertexPos;
	gl_Position = vertexPos;
}