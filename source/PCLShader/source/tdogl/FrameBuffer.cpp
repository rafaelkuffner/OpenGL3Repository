#include "FrameBuffer.h"
//delete objects
void FrameBuffer::destroy(){
	glDeleteFramebuffers(1, &FBO);
	glDeleteTextures(1, &texture_color);
	glDeleteTextures(1, &texture_depth);
}


//generate an empty color texture with 4 channels (RGBA8) using bilinear filtering
void FrameBuffer::GenerateColorTexture(unsigned int width, unsigned int height){
	GLenum error;
	glGenTextures(1, &texture_color);
	glBindTexture(GL_TEXTURE_2D, texture_color);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
}



//generate an empty depth texture with 1 depth channel using bilinear filtering
void FrameBuffer::GenerateDepthTexture(unsigned int width, unsigned int height){

	glGenTextures(1, &texture_depth);
	glBindTexture(GL_TEXTURE_2D, texture_depth);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

}

//Generate FBO and two empty textures
void FrameBuffer::GenerateFBO(unsigned int width, unsigned int height){

	
	
	//Generate a framebuffer object(FBO) and bind it to the pipeline
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	

	GenerateColorTexture(width, height);//generate empty texture
	GenerateDepthTexture(width, height);//generate empty texture
	

	//to keep track of our textures
	unsigned int attachment_index_color_texture = 0;

	//bind textures to pipeline. texture_depth is optional
	//0 is the mipmap level. 0 is the heightest

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment_index_color_texture, texture_color, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture_depth, 0);//optional

	//add attachements
	drawbuffer.push_back(GL_COLOR_ATTACHMENT0 + attachment_index_color_texture);
	glDrawBuffers(drawbuffer.size(), &drawbuffer[0]);
	//Check for FBO completeness
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
		std::cout << "Error! FrameBuffer is not complete" << std::endl;
		std::cin.get();
		std::terminate();
	}

	//unbind framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//return color texture from the framebuffer
unsigned int FrameBuffer::getColorTexture(){
	return texture_color;
}

//return depth texture from the framebuffer
unsigned int FrameBuffer::getDepthTexture(){
	return texture_depth;
}

//resize window
void FrameBuffer::resize(unsigned int width, unsigned int height){
	destroy();
	GenerateFBO(width, height);
}


//bind framebuffer to pipeline. We will  call this method in the render loop
void FrameBuffer::bind(){

	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	//this can be moved in GenerateFBO function but
	//we have to put here in case of a MRT functionality

}

//unbind framebuffer from pipeline. We will call this method in the render loop
void FrameBuffer::unbind(){
	//0 is the default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}