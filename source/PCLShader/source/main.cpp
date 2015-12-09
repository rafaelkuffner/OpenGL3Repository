/*
 main
 
 Copyright 2012 Thomas Dalling - http://tomdalling.com/

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include "platform.hpp"

// third-party libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// standard C++ libraries
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <cmath>

// tdogl classes
#include "tdogl/Program.h"
#include "tdogl/Texture.h"
#include "tdogl/Camera.h"
#include "tdogl/FrameBuffer.h"

//PCL
#include <pcl/point_types.h>
#include <pcl/io/vtk_lib_io.h>
#include <pcl/io/ply_io.h>
#include <pcl/features/normal_3d.h>
#include <pcl/conversions.h>
#include <pcl/common/transforms.h>

using namespace pcl;
// constants
const glm::vec2 SCREEN_SIZE(800, 600);

// globals
GLFWwindow* gWindow = NULL;
double gScrollY = 0.0;
tdogl::Texture* gTexture = NULL;
tdogl::Program* gProgram = NULL;
tdogl::Program* blurProgram = NULL;
tdogl::Program* rttProgram = NULL;

tdogl::Camera gCamera;
FrameBuffer fbo;
FrameBuffer fboCanvasV;
FrameBuffer fboCanvasH;
GLuint gVAO = 0;
GLfloat gDegreesRotated = 0.0f;
GLsizei numElements;
glm::vec2 lastMousePos(0, 0);
float resolution = 0;
unsigned int rtt_vbo, rtt_ibo, rtt_vao;
int wWidth, wHeight;
// loads the vertex shader and fragment shader, and links them to make the global gProgram
static void LoadShaders() {
    std::vector<tdogl::Shader> shaders;
	shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath("vertex-shader.vert"), GL_VERTEX_SHADER));
	shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath("fragment-shader.frag"), GL_FRAGMENT_SHADER));
	shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath("normals-shader.geom"), GL_GEOMETRY_SHADER));
    gProgram = new tdogl::Program(shaders);
	glBindFragDataLocation(gProgram->object(), 0, "finalColor");
	
	std::vector<tdogl::Shader> shaders3;
	shaders3.push_back(tdogl::Shader::shaderFromFile(ResourcePath("rtt.vert"), GL_VERTEX_SHADER));
	shaders3.push_back(tdogl::Shader::shaderFromFile(ResourcePath("blur.frag"), GL_FRAGMENT_SHADER));
	blurProgram = new tdogl::Program(shaders3);
	glBindFragDataLocation(blurProgram->object(), 0, "FragmentColor");

	std::vector<tdogl::Shader> shaders2;
	shaders2.push_back(tdogl::Shader::shaderFromFile(ResourcePath("rtt.vert"), GL_VERTEX_SHADER));
	shaders2.push_back(tdogl::Shader::shaderFromFile(ResourcePath("rtt.frag"), GL_FRAGMENT_SHADER));
	rttProgram = new tdogl::Program(shaders2);
	glBindFragDataLocation(rttProgram->object(),0, "outColor");




	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
		std::cerr << "OpenGL Error LoadShaders" << error << std::endl;
}

double computeCloudResolution(const pcl::PointCloud<PointXYZRGB>::ConstPtr &cloud)
{
	double res = 0.0;
	int n_points = 0;
	int nres;
	std::vector<int> indices(2);
	std::vector<float> sqr_distances(2);
	pcl::search::KdTree<PointXYZRGB> tree;
	tree.setInputCloud(cloud);

	for (size_t i = 0; i < cloud->size(); ++i)
	{
		if (!pcl_isfinite((*cloud)[i].x))
		{
			continue;
		}
		//Considering the second neighbor since the first is the point itself.
		nres = tree.nearestKSearch(i, 2, indices, sqr_distances);
		if (nres == 2)
		{
			res += sqrt(sqr_distances[1]);
			++n_points;
		}
	}
	if (n_points != 0)
	{
		res /= n_points;
	}
	return res;
}

// loads a cube into the VAO and VBO globals: gVAO and gVBO
static void LoadCloud() {
    // make and bind the VAO
    glGenVertexArrays(1, &gVAO);
    glBindVertexArray(gVAO);
    
    
	PolygonMesh mesh;
	PointCloud<PointXYZRGB>::Ptr cloud(new PointCloud<PointXYZRGB>);
	PointCloud<Normal>::Ptr normals(new PointCloud<Normal>);

	PointCloud<PointXYZRGBNormal>::Ptr cloud_with_normals(new PointCloud<PointXYZRGBNormal>);
	io::loadPolygonFilePLY(ResourcePath("bear.ply"), mesh);
	
	fromPCLPointCloud2(mesh.cloud, *cloud);

	resolution = computeCloudResolution(cloud);

	NormalEstimation<PointXYZRGB, Normal> ne;
	ne.setInputCloud(cloud);
	search::KdTree<pcl::PointXYZRGB>::Ptr tree(new search::KdTree<pcl::PointXYZRGB>());
	ne.setSearchMethod(tree);
	//ne.setKSearch(2);
	ne.setRadiusSearch(0.005);
	ne.compute(*normals);
	pcl::concatenateFields(*cloud, *normals, *cloud_with_normals);
	numElements = cloud_with_normals->size();
	Eigen::Vector4f centroid;
	Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
	pcl::compute3DCentroid(*cloud_with_normals, centroid);
	transform(0, 3) = -centroid.x();
	transform(1,3) = -centroid.y();
	transform(2, 3) = -centroid.z();
	pcl::transformPointCloudWithNormals(*cloud_with_normals, *cloud_with_normals, transform);

	float *varray = (float*)malloc(sizeof(float)*numElements * 4);
	float *carray = (float*)malloc(sizeof(float)*numElements * 4);
	float *narray = (float*)malloc(sizeof(float)*numElements * 4);
	int i = 0;
	for (PointCloud<PointXYZRGBNormal>::iterator it = cloud_with_normals->begin(); it != cloud_with_normals->end(); it++){
		float r = it->r / 255.0;
		float g = it->g / 255.0;
		float b = it->b / 255.0;

		varray[i] = it->x;
		varray[i + 1] = it->y;
		varray[i + 2] = it->z;
		varray[i + 3] = 1.0;

		carray[i] = r;
		carray[i + 1] = g;
		carray[i + 2] = b;
		carray[i + 3] = 1.0;

		narray[i] = - it->normal_x;
		narray[i + 1] = -it->normal_y;
		narray[i + 2] = -it->normal_z;
		narray[i + 3] = 1.0;

		i += 4;
	}

	GLuint gVBO1,gVBO2,gVBO3;

	// make and bind the VBO
	glGenBuffers(1, &gVBO1);
	glBindBuffer(GL_ARRAY_BUFFER, gVBO1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*numElements*4, varray, GL_STATIC_DRAW);
    // connect the xyz to the "vert" attribute of the vertex shader
    glEnableVertexAttribArray(gProgram->attrib("vert"));
    glVertexAttribPointer(gProgram->attrib("vert"), 4, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), NULL);

	glGenBuffers(1, &gVBO2);
	glBindBuffer(GL_ARRAY_BUFFER, gVBO2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*numElements * 4, carray, GL_STATIC_DRAW);
	// connect the color to the "col" attribute of the vertex shader
	glEnableVertexAttribArray(gProgram->attrib("col"));
	glVertexAttribPointer(gProgram->attrib("col"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), NULL);

	glGenBuffers(1, &gVBO3);
	glBindBuffer(GL_ARRAY_BUFFER, gVBO3);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*numElements * 4, narray, GL_STATIC_DRAW);
	// connect the normal to the "norm" attribute of the vertex shader
	glEnableVertexAttribArray(gProgram->attrib("norm"));
	glVertexAttribPointer(gProgram->attrib("norm"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), NULL);
  
    // unbind the VAO
    glBindVertexArray(0);
}

static void LoadRTTVariables(){
	glGenVertexArrays(1, &rtt_vao);
	glBindVertexArray(rtt_vao);

	// make and bind the VBO
	glGenBuffers(1, &rtt_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, rtt_vbo);

	// Put the three triangle vertices (XYZ) and texture coordinates (UV) into the VBO
	GLfloat vertexData[] = {
		//  X     Y     Z       U     V
		-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

	// connect the xyz to the "vert" attribute of the vertex shader
	glEnableVertexAttribArray(rttProgram->attrib("in_position"));
	glVertexAttribPointer(rttProgram->attrib("in_position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), NULL);

	// connect the uv coords to the "vertTexCoord" attribute of the vertex shader
	glEnableVertexAttribArray(rttProgram->attrib("in_texcoord"));
	glVertexAttribPointer(rttProgram->attrib("in_texcoord"), 2, GL_FLOAT, GL_TRUE, 5 * sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

	// unbind the VAO
	glBindVertexArray(0);
}

// loads the file "wooden-crate.jpg" into gTexture
static void LoadTexture() {
    tdogl::Bitmap bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath("brush3.png"));
    bmp.flipVertically();
    gTexture = new tdogl::Texture(bmp);
}


// draws a single frame
static void Render() {
	GLenum error;
	//pass 1: normal render to texture
	fbo.bind(); 
	{
		// clear everything
		glClearColor(1.0, 1.0, 1.0, 1); // black
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// bind the program (the shaders)
		gProgram->use();

		// set the "camera" uniform
		gProgram->setUniform("camera", gCamera.matrix());
		// set the camera position uniform
		gProgram->setUniform("camPosition", gCamera.position());
		gProgram->setUniform("resolution", resolution);
		// set the "model" uniform in the vertex shader, based on the gDegreesRotated global
		gProgram->setUniform("model", glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0, 1, 0)));

		// bind the texture and set the "tex" uniform in the fragment shader
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gTexture->object());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		gProgram->setUniform("tex", 0); //set to 0 because the texture is bound to GL_TEXTURE0

		// bind the VAO
		glBindVertexArray(gVAO);

		// draw the VAO
		glDrawArrays(GL_POINTS, 0, numElements);

		// unbind the VAO, the program and the texture
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		gProgram->stopUsing();
	}
	fbo.unbind();

	error = glGetError();
	if (error != GL_NO_ERROR)
		std::cerr << "OpenGL Error Render pass 1 " << error << std::endl;
	



	//pass 2: blur 
	{
	
		glBindVertexArray(rtt_vao);
		glActiveTexture(GL_TEXTURE0 + 1);

		blurProgram->use();
		blurProgram->setUniform("image", 1);	
		error = glGetError();
		blurProgram->setUniform("width", (float)wWidth);
		error = glGetError();
		blurProgram->setUniform("height", (float)wHeight);

		fboCanvasH.bind();
		glClearColor(1.0, 1.0, 1.0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//pass 2.1, horiz uses fbo text as input
		glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture());
		blurProgram->setUniform("d", 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		fboCanvasH.unbind();

		fboCanvasV.bind();
		glClearColor(1.0, 1.0, 1.0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//pass 2.2, horiz uses fbo text as input
		glBindTexture(GL_TEXTURE_2D, fboCanvasH.getColorTexture());
		blurProgram->setUniform("d", 1);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		fboCanvasV.unbind();
		
		int iterations = 9;
		for (int i = 0; i < iterations;i++){
			fboCanvasH.bind();
			glClearColor(1.0, 1.0, 1.0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glBindTexture(GL_TEXTURE_2D, fboCanvasV.getColorTexture());
			blurProgram->setUniform("d", 0);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			fboCanvasH.unbind();

			fboCanvasV.bind();
			glClearColor(1.0, 1.0, 1.0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glBindTexture(GL_TEXTURE_2D, fboCanvasH.getColorTexture());
			blurProgram->setUniform("d", 1);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			fboCanvasV.unbind();
		}

		// unbind the VAO, the program and the texture
		glBindVertexArray(0);
		blurProgram->stopUsing();
	}	

	error = glGetError();
	if (error != GL_NO_ERROR)
		std::cerr << "OpenGL Error Render pass 2 " << error << std::endl;

	//pass 3: texture to OGL
	{
		glClearColor(1.0, 1.0, 1.0, 1); 
		// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		rttProgram->use();

		glActiveTexture(GL_TEXTURE0 + 2);
		glBindTexture(GL_TEXTURE_2D, fboCanvasV.getColorTexture());
		rttProgram->setUniform("texture_color", 2);

		glBindVertexArray(rtt_vao);

		// draw the VAO
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// unbind the VAO, the program and the texture

		glBindVertexArray(0);
		rttProgram->stopUsing();
	}
	error = glGetError();
	if (error != GL_NO_ERROR)
		std::cerr << "OpenGL Error Render pass 3 " << error << std::endl;

    // swap the display buffers (displays what was just drawn)
    glfwSwapBuffers(gWindow);

//	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


// update the scene based on the time elapsed since last update
void Update(float secondsElapsed) {
    //rotate the cube
    const GLfloat degreesPerSecond = 45.0f;
  //  gDegreesRotated += secondsElapsed * degreesPerSecond;
  //  while(gDegreesRotated > 360.0f) gDegreesRotated -= 360.0f;

    //move position of camera based on WASD keys, and XZ keys for up and down
    const float moveSpeed = 0.3; //units per second
    if(glfwGetKey(gWindow, 'S')){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * -gCamera.forward());
    } else if(glfwGetKey(gWindow, 'W')){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * gCamera.forward());
    }
    if(glfwGetKey(gWindow, 'A')){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * -gCamera.right());
    } else if(glfwGetKey(gWindow, 'D')){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * gCamera.right());
    }
    if(glfwGetKey(gWindow, 'Z')){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * -glm::vec3(0,1,0));
    } else if(glfwGetKey(gWindow, 'X')){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * glm::vec3(0,1,0));
    }

    //rotate camera based on mouse movement
    const float mouseSensitivity = 0.1f;
    double mouseX, mouseY;
	int state = glfwGetMouseButton(gWindow, GLFW_MOUSE_BUTTON_LEFT);
	glfwGetCursorPos(gWindow, &mouseX, &mouseY);
	if (state == GLFW_PRESS){
		double deltaY = mouseY - lastMousePos.y;
		double deltaX = mouseX - lastMousePos.x;
		gCamera.offsetOrientation(mouseSensitivity * (float)deltaY, mouseSensitivity * (float)deltaX);
	}
	lastMousePos.x = mouseX;
	lastMousePos.y = mouseY;

    //increase or decrease field of view based on mouse wheel
    const float zoomSensitivity = -0.2f;
    float fieldOfView = gCamera.fieldOfView() + zoomSensitivity * (float)gScrollY;
    if(fieldOfView < 5.0f) fieldOfView = 5.0f;
    if(fieldOfView > 130.0f) fieldOfView = 130.0f;
    gCamera.setFieldOfView(fieldOfView);
    gScrollY = 0;
}

// records how far the y axis has been scrolled
void OnScroll(GLFWwindow* window, double deltaX, double deltaY) {
    gScrollY += deltaY;
}

void OnError(int errorCode, const char* msg) {
    throw std::runtime_error(msg);
}

void OnResize(GLFWwindow* window, int width, int height){
	glViewport(0, 0, width, height);
	gCamera.setViewportAspectRatio((float)width / (float)height);
	fbo.GenerateFBO(width, height);
	fboCanvasV.GenerateFBO(width, height);
	fboCanvasH.GenerateFBO(width, height);
	wWidth = width;
	wHeight = height;
}

// the program starts here
void AppMain() {
    // initialise GLFW
    glfwSetErrorCallback(OnError);
    if(!glfwInit())
        throw std::runtime_error("glfwInit failed");
    
    // open a window with GLFW
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    gWindow = glfwCreateWindow((int)SCREEN_SIZE.x, (int)SCREEN_SIZE.y, "OpenGL Tutorial", NULL, NULL);
    if(!gWindow)
        throw std::runtime_error("glfwCreateWindow failed. Can your hardware handle OpenGL 3.2?");

    // GLFW settings
  //  glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(gWindow, 0, 0);
    glfwSetScrollCallback(gWindow, OnScroll);
	glfwMakeContextCurrent(gWindow);
	glfwSetFramebufferSizeCallback(gWindow, OnResize);
    // initialise GLEW
    glewExperimental = GL_TRUE; //stops glew crashing on OSX :-/
    if(glewInit() != GLEW_OK)
        throw std::runtime_error("glewInit failed");
    
    // GLEW throws some errors, so discard all the errors so far
    while(glGetError() != GL_NO_ERROR) {}

    // print out some info about the graphics drivers
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // make sure OpenGL version 3.2 API is available
    if(!GLEW_VERSION_3_2)
        throw std::runtime_error("OpenGL 3.2 API is not available.");

    // OpenGL settings
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
	glEnable(GL_BLEND); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // load vertex and fragment shaders into opengl
    LoadShaders();

    // load the texture
    LoadTexture();

    // create buffer and fill it with the points of the triangle
    LoadCloud();

	LoadRTTVariables();
	//create frame buffer
	wWidth = SCREEN_SIZE.x;
	wHeight = SCREEN_SIZE.y;
	fbo.GenerateFBO(wWidth, wHeight);
	fboCanvasH.GenerateFBO(wWidth, wHeight);
	fboCanvasV.GenerateFBO(wWidth, wHeight);
    // setup gCamera
    gCamera.setPosition(glm::vec3(0,0,2));
	
    gCamera.setViewportAspectRatio(SCREEN_SIZE.x / SCREEN_SIZE.y);

    // run while the window is open
    double lastTime = glfwGetTime();
    while(!glfwWindowShouldClose(gWindow)){
        // process pending events
        glfwPollEvents();
        
        // update the scene based on the time elapsed since last update
        double thisTime = glfwGetTime();
        Update((float)(thisTime - lastTime));
        lastTime = thisTime;
        
        // draw one frame
        Render();

        // check for errors
        GLenum error = glGetError();
        if(error != GL_NO_ERROR)
            std::cerr << "OpenGL Error " << error << std::endl;

        //exit program if escape key is pressed
        if(glfwGetKey(gWindow, GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(gWindow, GL_TRUE);
    }

    // clean up and exit
    glfwTerminate();
}


int main(int argc, char *argv[]) {
    try {
        AppMain();
    } catch (const std::exception& e){
        std::cerr << "ERROR: " << e.what() << std::endl;
		getchar();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
