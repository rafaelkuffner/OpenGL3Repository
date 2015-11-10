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


//PCL
#include <pcl/point_types.h>
#include <pcl/io/vtk_lib_io.h>
#include <pcl/io/ply_io.h>
#include <pcl/features/normal_3d.h>
#include <pcl/conversions.h>

using namespace pcl;
// constants
const glm::vec2 SCREEN_SIZE(800, 600);

// globals
GLFWwindow* gWindow = NULL;
double gScrollY = 0.0;
tdogl::Texture* gTexture = NULL;
tdogl::Program* gProgram = NULL;
tdogl::Camera gCamera;
GLuint gVAO = 0;
GLfloat gDegreesRotated = 0.0f;
GLsizei numElements;
glm::vec2 lastMousePos(0, 0);

// loads the vertex shader and fragment shader, and links them to make the global gProgram
static void LoadShaders() {
    std::vector<tdogl::Shader> shaders;
	shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath("vertex-shader.vert"), GL_VERTEX_SHADER));
	shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath("fragment-shader.frag"), GL_FRAGMENT_SHADER));
	shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath("geometry-shader.geom"), GL_GEOMETRY_SHADER));
    gProgram = new tdogl::Program(shaders);
	glBindFragDataLocation(gProgram->object(), 0, "finalColor");
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
	io::loadPolygonFilePLY("bun_love.ply", mesh);
	
	fromPCLPointCloud2(mesh.cloud, *cloud);

	NormalEstimation<PointXYZRGB, Normal> ne;
	ne.setInputCloud(cloud);
	search::KdTree<pcl::PointXYZRGB>::Ptr tree(new search::KdTree<pcl::PointXYZRGB>());
	ne.setSearchMethod(tree);
	ne.setRadiusSearch(0.005);
	ne.compute(*normals);
	pcl::concatenateFields(*cloud, *normals, *cloud_with_normals);
	numElements = cloud_with_normals->size();

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

		narray[i] = it->normal_x;
		narray[i + 1] = it->normal_y;
		narray[i + 2] = it->normal_z;
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
        
    //// connect the uv coords to the "vertTexCoord" attribute of the vertex shader
    //glEnableVertexAttribArray(gProgram->attrib("vertTexCoord"));
    //glVertexAttribPointer(gProgram->attrib("vertTexCoord"), 2, GL_FLOAT, GL_TRUE,  5*sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

    // unbind the VAO
    glBindVertexArray(0);
}


// loads the file "wooden-crate.jpg" into gTexture
static void LoadTexture() {
    tdogl::Bitmap bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath("wooden-crate.jpg"));
    bmp.flipVertically();
    gTexture = new tdogl::Texture(bmp);
}


// draws a single frame
static void Render() {
    // clear everything
    glClearColor(0, 0, 0, 1); // black
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // bind the program (the shaders)
    gProgram->use();

    // set the "camera" uniform
    gProgram->setUniform("camera", gCamera.matrix());

    // set the "model" uniform in the vertex shader, based on the gDegreesRotated global
    gProgram->setUniform("model", glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0,1,0)));
        
    // bind the texture and set the "tex" uniform in the fragment shader
    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, gTexture->object());
    //gProgram->setUniform("tex", 0); //set to 0 because the texture is bound to GL_TEXTURE0

    // bind the VAO (the triangle)
    glBindVertexArray(gVAO);
    
    // draw the VAO
    glDrawArrays(GL_POINTS, 0,numElements);
    
    // unbind the VAO, the program and the texture
    glBindVertexArray(0);
  //glBindTexture(GL_TEXTURE_2D, 0);
    gProgram->stopUsing();
    
    // swap the display buffers (displays what was just drawn)
    glfwSwapBuffers(gWindow);
}


// update the scene based on the time elapsed since last update
void Update(float secondsElapsed) {
    //rotate the cube
    const GLfloat degreesPerSecond = 45.0f;
    gDegreesRotated += secondsElapsed * degreesPerSecond;
    while(gDegreesRotated > 360.0f) gDegreesRotated -= 360.0f;

    //move position of camera based on WASD keys, and XZ keys for up and down
    const float moveSpeed = 2.0; //units per second
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
