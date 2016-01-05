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
#include <sstream>

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

// region growing segmentation
#include <pcl/filters/passthrough.h>
#include <pcl/segmentation/region_growing.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/segmentation/region_growing_rgb.h>

using namespace pcl;
// constants
const glm::vec2 SCREEN_SIZE(800, 600);

// globals
GLFWwindow* gWindow = NULL;
double gScrollY = 0.0;
std::vector<tdogl::Texture*> htextures;
std::vector<tdogl::Texture*> vtextures;
std::vector<tdogl::Texture*> rtextures;
tdogl::Program* gProgram = NULL;
tdogl::Program* blurProgram = NULL;
tdogl::Program* rttProgram = NULL;
tdogl::Program* diffProgram = NULL;
tdogl::Program* rttArtProgram = NULL;
tdogl::Program* g2Program = NULL;

tdogl::Camera gCamera;
FrameBuffer fbo;
FrameBuffer fboGridBig;
FrameBuffer fboGridSmall;

std::vector<GLuint> gVAOs;
std::vector<GLsizei> numElements;
std::vector<int> brushtype;
GLfloat gDegreesRotated = 0.0f;
glm::vec2 lastMousePos(0, 0);
std::vector<float> resolutions;
unsigned int rtt_vbo, rtt_ibo, rtt_vao;
int wWidth, wHeight;
int gridSizeBig, gridSizeSmall;

int cluster = 0;
// loads the vertex shader and fragment shader, and links them to make the global gProgram
static void LoadShaders() {
    std::vector<tdogl::Shader> shaders;
	shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath("vertex-shader.vert"), GL_VERTEX_SHADER));
	shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath("fragment-shader.frag"), GL_FRAGMENT_SHADER));
	shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath("normals-shader.geom"), GL_GEOMETRY_SHADER));
    gProgram = new tdogl::Program(shaders);
	glBindFragDataLocation(gProgram->object(), 0, "finalColor");
	



	std::vector<tdogl::Shader> shaders2;
	shaders2.push_back(tdogl::Shader::shaderFromFile(ResourcePath("rtt.vert"), GL_VERTEX_SHADER));
	shaders2.push_back(tdogl::Shader::shaderFromFile(ResourcePath("rtt.frag"), GL_FRAGMENT_SHADER));
	rttProgram = new tdogl::Program(shaders2);
	glBindFragDataLocation(rttProgram->object(), 0, "outColor");


	std::vector<tdogl::Shader> shaders3;
	shaders3.push_back(tdogl::Shader::shaderFromFile(ResourcePath("rtt.vert"), GL_VERTEX_SHADER));
	shaders3.push_back(tdogl::Shader::shaderFromFile(ResourcePath("blur.frag"), GL_FRAGMENT_SHADER));
	blurProgram = new tdogl::Program(shaders3);
	glBindFragDataLocation(blurProgram->object(), 0, "FragmentColor");

	std::vector<tdogl::Shader> shaders4;
	shaders4.push_back(tdogl::Shader::shaderFromFile(ResourcePath("rtt.vert"), GL_VERTEX_SHADER));
	shaders4.push_back(tdogl::Shader::shaderFromFile(ResourcePath("diff.frag"), GL_FRAGMENT_SHADER));
	diffProgram = new tdogl::Program(shaders4);
	glBindFragDataLocation(diffProgram->object(), 0, "BrushColor");

	std::vector<tdogl::Shader> shaders5;
	shaders5.push_back(tdogl::Shader::shaderFromFile(ResourcePath("rtt.vert"), GL_VERTEX_SHADER));
	shaders5.push_back(tdogl::Shader::shaderFromFile(ResourcePath("rttArt.frag"), GL_FRAGMENT_SHADER));
	rttArtProgram = new tdogl::Program(shaders5);
	glBindFragDataLocation(rttArtProgram->object(), 0, "finalColor");

	std::vector<tdogl::Shader> shaders6;
	shaders6.push_back(tdogl::Shader::shaderFromFile(ResourcePath("vertex-shader.vert"), GL_VERTEX_SHADER));
	shaders6.push_back(tdogl::Shader::shaderFromFile(ResourcePath("fragment-shader.frag"), GL_FRAGMENT_SHADER));
	shaders6.push_back(tdogl::Shader::shaderFromFile(ResourcePath("normals-shader.geom"), GL_GEOMETRY_SHADER));
	g2Program = new tdogl::Program(shaders6);
	glBindFragDataLocation(g2Program->object(), 0, "finalColor");

	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
		std::cerr << "OpenGL Error LoadShaders" << error << std::endl;
}

double computeCloudResolution(const pcl::PointCloud<PointXYZRGBNormal>::ConstPtr &cloud)
{
	double res = 0.0;
	int n_points = 0;
	int nres;
	std::vector<int> indices(2);
	std::vector<float> sqr_distances(2);
	pcl::search::KdTree<PointXYZRGBNormal> tree;
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



static std::vector<PointCloud<PointXYZRGBNormal>::Ptr> segmentPointCloud(PointCloud<PointXYZRGBNormal>::Ptr cloud_with_normals){

	pcl::search::Search <pcl::PointXYZRGBNormal>::Ptr tree = boost::shared_ptr<pcl::search::Search<pcl::PointXYZRGBNormal> >(new pcl::search::KdTree<pcl::PointXYZRGBNormal>);
	
	
	 pcl::RegionGrowingRGB<pcl::PointXYZRGBNormal> reg;
	 reg.setInputCloud(cloud_with_normals);

	 reg.setSearchMethod(tree);
	 reg.setDistanceThreshold(1000);
	 reg.setPointColorThreshold(4.7);
	 reg.setRegionColorThreshold(7);
	 reg.setMinClusterSize(500);
	
	 std::vector <pcl::PointIndices> clusters;
	 reg.extract(clusters);
	 std::vector<PointCloud<PointXYZRGBNormal>::Ptr> results;
	 for (std::vector<pcl::PointIndices>::const_iterator it = clusters.begin(); it != clusters.end(); ++it)
	 {
		 pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud_cluster(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
		 for (std::vector<int>::const_iterator pit = it->indices.begin(); pit != it->indices.end(); ++pit)
			 cloud_cluster->points.push_back(cloud_with_normals->points[*pit]);
		 cloud_cluster->width = cloud_cluster->points.size();
		 cloud_cluster->height = 1;
		 cloud_cluster->is_dense = true;
		 results.push_back(cloud_cluster);
	 }
	 return results;
}

void loadPly(PointCloud<PointXYZRGB>::Ptr cloud, std::string path){
	PolygonMesh mesh; 
	io::loadPolygonFilePLY(ResourcePath(path), mesh);
	fromPCLPointCloud2(mesh.cloud, *cloud);
}

void readKinectCloudDisk(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud, std::string pointCloudPath,std::string outpath){
	std::ifstream f;
	f.open(ResourcePath(pointCloudPath), fstream::in);
	std::string line;
	std::string delimiter1 = " ";

	std::string *pos = new std::string[6];
	while (std::getline(f, line))
	{
		replace(line.begin(), line.end(), ',', '.');
		//int sep = line.find(delimiter1);
		///*std::string s1 = line.substr(0, sep);
		//std::string s2 = line.substr(sep + 1, line.length());*/
		size_t p;
		int i = 0;
		while ((p = line.find(delimiter1)) != std::string::npos) {
			pos[i] = line.substr(0, p);
			line.erase(0, p + delimiter1.length());
			i++;
		}
		pos[i] = line;
		pcl::PointXYZRGB point;
		point.x = stof(pos[0]);
		point.y = stof(pos[1]);
		point.z = stof(pos[2]);
		point.b = stof(pos[3]);
		point.g = stof(pos[4]);
		point.r = stof(pos[5]);
		if (point.x != 0 && point.y != 0 && point.z != 0)
			cloud->points.push_back(point);
	}
	pcl::PLYWriter wt;
	wt.write(outpath, *cloud);
}



static void CloudPreprocess(PointCloud<PointXYZRGBNormal>::Ptr cloud_with_normals){
	
	PointCloud<PointXYZRGB>::Ptr cloud(new PointCloud<PointXYZRGB>);
	PointCloud<Normal>::Ptr normals(new PointCloud<Normal>);

	//for (int i = 0; i < 23; i++){
	//	std::stringstream s;
	//	s << "partial\\outputCloud" << i;
	//	std::stringstream s2;
	//	s2 << "outputCloud" << i << ".ply";
	//	cloud->clear();
	//	readKinectCloudDisk(cloud, s.str(), s2.str());
	//	
	//}
	//readKinectCloudDisk(cloud, "RafaScan","RafaScan2.ply");
	loadPly(cloud, "simple100k.ply");
	


	Eigen::Vector4f centroid;
	Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
	pcl::compute3DCentroid(*cloud, centroid);
	transform(0, 3) = -centroid.x();
	transform(1, 3) = -centroid.y();
	transform(2, 3) = -centroid.z();
	pcl::transformPointCloud(*cloud, *cloud, transform);

	NormalEstimation<PointXYZRGB, Normal> ne;
	ne.setInputCloud(cloud);
	ne.setViewPoint(0, 0, 0);
	search::KdTree<pcl::PointXYZRGB>::Ptr tree(new search::KdTree<pcl::PointXYZRGB>());
	ne.setSearchMethod(tree);
	//ne.setKSearch(2);
	ne.setRadiusSearch(0.005);
	ne.compute(*normals);
	
	pcl::concatenateFields(*cloud, *normals, *cloud_with_normals);

}


// loads a cube into the VAO and VBO globals: gVAO and gVBO
static void LoadCloud() {
    
	PointCloud<PointXYZRGBNormal>::Ptr cloud_with_normals(new PointCloud<PointXYZRGBNormal>);
	CloudPreprocess(cloud_with_normals);
	std::vector<PointCloud<PointXYZRGBNormal>::Ptr> cloud_clusters = segmentPointCloud(cloud_with_normals);

	for (int k = 0; k < cloud_clusters.size(); k++){
		GLuint gVAO;
		glGenVertexArrays(1, &gVAO);
		glBindVertexArray(gVAO);

		gVAOs.push_back(gVAO);
		numElements.push_back(cloud_clusters[k]->size());
		// make and bind the VAO
		PointXYZRGBNormal min, max;
		pcl::getMinMax3D(*cloud_clusters[k], min, max);
		float sizex = fabs(max.x - min.x);
		float sizey = fabs(max.y - min.y);
		float sizez = fabs(max.z - min.z);
		int brush;
		//vertical brush
		if (sizey > 1.5 * sizex && sizey > 1.5 * sizez)
			brush = 1;
		else if (sizex > 1.5* sizey || sizez > 1.5*sizey){
			brush = 0;
		}
		else{
			brush = 2;
		}
		
		brushtype.push_back(brush);
		
		resolutions.push_back(computeCloudResolution(cloud_clusters[k]));


		float *varray = (float*)malloc(sizeof(float)*numElements[k] * 4);
		float *carray = (float*)malloc(sizeof(float)*numElements[k] * 4);
		float *narray = (float*)malloc(sizeof(float)*numElements[k] * 4);
		int i = 0;
		for (PointCloud<PointXYZRGBNormal>::iterator it = cloud_clusters[k]->begin(); it != cloud_clusters[k]->end(); it++){
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
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*numElements[k]*4, varray, GL_STATIC_DRAW);
		// connect the xyz to the "vert" attribute of the vertex shader
		glEnableVertexAttribArray(gProgram->attrib("vert"));
		glVertexAttribPointer(gProgram->attrib("vert"), 4, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), NULL);

		glGenBuffers(1, &gVBO2);
		glBindBuffer(GL_ARRAY_BUFFER, gVBO2);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*numElements[k] * 4, carray, GL_STATIC_DRAW);
		// connect the color to the "col" attribute of the vertex shader
		glEnableVertexAttribArray(gProgram->attrib("col"));
		glVertexAttribPointer(gProgram->attrib("col"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), NULL);

		glGenBuffers(1, &gVBO3);
		glBindBuffer(GL_ARRAY_BUFFER, gVBO3);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*numElements[k] * 4, narray, GL_STATIC_DRAW);
		// connect the normal to the "norm" attribute of the vertex shader
		glEnableVertexAttribArray(gProgram->attrib("norm"));
		glVertexAttribPointer(gProgram->attrib("norm"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), NULL);

		free(varray);
		free(narray);
		free(carray);

	}
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
	for (int i = 1; i <= 10; i++){
		std::stringstream s1, s2, s3;
		s1 << "vert3brush\\b" << i<<".png";
		s2 << "horiz3brush\\b" << i << ".png";
		s3 << "round3brush\\b" << i << ".png";
		tdogl::Bitmap bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(s1.str()));
		bmp.flipVertically();
		htextures.push_back(new tdogl::Texture(bmp));
		bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(s2.str()));
		bmp.flipVertically();
		vtextures.push_back(new tdogl::Texture(bmp));
		bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(s3.str()));
		bmp.flipVertically();
		rtextures.push_back(new tdogl::Texture(bmp));
	}
}


static void checkError(const char* msg){
	GLenum error;
	error = glGetError();
	if (error != GL_NO_ERROR)
		std::cerr << "OpenGL Error " << msg << " " << error << std::endl;

}

static void firstPass(float resolutionMult, int outbuf){

	fbo.bind();
	{
		GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 +outbuf};
		glDrawBuffers(1, DrawBuffers);

		// clear everything
		glClearColor(0.0,0.0, 0.0, 1); // black
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// bind the program (the shaders)
		gProgram->use();
		// set the "camera" uniform
		gProgram->setUniform("camera", gCamera.matrix());
		// set the camera position uniform
		//gProgram->setUniform("camPosition", gCamera.position());
		// set the "model" uniform in the vertex shader, based on the gDegreesRotated global
		gProgram->setUniform("model", glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0, 1, 0)));

		// bind the texture and set the "tex" uniform in the fragment shader
	

		for (int j = 0; j < gVAOs.size(); j++){
			// bind the VAO
			glBindVertexArray(gVAOs[j]);

			gProgram->setUniform("resolution", resolutions[j]*resolutionMult);
			for (int i = 0; i < 10; i++){
				glActiveTexture(GL_TEXTURE0 + i);
				if (brushtype[j] == 0)
					glBindTexture(GL_TEXTURE_2D, htextures[i]->object());
				if (brushtype[j] == 1)
					glBindTexture(GL_TEXTURE_2D, vtextures[i]->object());
				if (brushtype[j] == 2)
					glBindTexture(GL_TEXTURE_2D, rtextures[i]->object());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				std::stringstream s;
				s << "tex" << i;
				gProgram->setUniform(s.str().c_str(), i);
			}

			// draw the VAO
			glDrawArrays(GL_POINTS, 0, numElements[j]);
		}

		

		// unbind the VAO, the program and the texture
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		gProgram->stopUsing();
	}
	fbo.unbind();

}

static void secondPass(int iterations){

	fbo.bind();
	glBindVertexArray(rtt_vao);
	glActiveTexture(GL_TEXTURE0 + 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	blurProgram->use();
	blurProgram->setUniform("image", 1);
	blurProgram->setUniform("width", (float)wWidth);
	blurProgram->setUniform("height", (float)wHeight);
	
	GLenum db1[1] = { GL_COLOR_ATTACHMENT1 };
	GLenum db2[1] = { GL_COLOR_ATTACHMENT2 };
	//pass 2.1, horiz uses fbo text as input
	glDrawBuffers(1, db1);
	glClearColor(0.0, 0.0, 0.0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture(0));
	blurProgram->setUniform("d", 0);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	
	//pass 2.2, horiz uses fbo text as input
	glDrawBuffers(1, db2);
	glClearColor(0.0, 0.0, 0.0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture(1));
	blurProgram->setUniform("d", 1);
	glDrawArrays(GL_TRIANGLES, 0, 6);


	for (int i = 0; i < iterations; i++){

		glDrawBuffers(1, db1);
		glClearColor(0.0, 0.0, 0.0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture(2));
		blurProgram->setUniform("d", 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		

		glDrawBuffers(1, db2);
		glClearColor(0.0,0.0, 0.0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture(1));
		blurProgram->setUniform("d", 1);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	
	}

	// unbind the VAO, the program and the texture
	glBindVertexArray(0);
	blurProgram->stopUsing();
	fbo.unbind();
}


static void thirdPass(){
	int w = (int)wWidth / ((float)gridSizeBig);
	int h = (int)wHeight / ((float)gridSizeBig);

	glViewport(0, 0,w,h);
	fboGridBig.bind();	
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);
	glClearColor(0.0,0.0, 0.0, 1);
	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	diffProgram->use();
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture(0));
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture(2));

	diffProgram->setUniform("origImage", 1);
	diffProgram->setUniform("blurImage", 2);
	diffProgram->setUniform("gridSize", gridSizeBig);
	diffProgram->setUniform("width", (float)wWidth);
	diffProgram->setUniform("height", (float)wHeight);

	glBindVertexArray(rtt_vao);

	// draw the VAO
	glDrawArrays(GL_TRIANGLES, 0, 6);
	checkError("4");
	// unbind the VAO, the program and the texture
	glBindVertexArray(0);
	diffProgram->stopUsing();
	fboGridBig.unbind();
	glViewport(0, 0, wWidth, wHeight);
}

static void fourthPass(){
	int w = (int)wWidth / ((float)gridSizeSmall);
	int h = (int)wHeight / ((float)gridSizeSmall);

	glViewport(0, 0, w, h);
	fboGridSmall.bind();
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);
	glClearColor(0.0,0.0, 0.0, 1);
	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	diffProgram->use();
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture(0));
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture(2));

	diffProgram->setUniform("origImage", 1);
	diffProgram->setUniform("blurImage", 2);
	diffProgram->setUniform("gridSize", gridSizeSmall);
	diffProgram->setUniform("width", (float)wWidth);
	diffProgram->setUniform("height", (float)wHeight);

	glBindVertexArray(rtt_vao);

	// draw the VAO
	glDrawArrays(GL_TRIANGLES, 0, 6);
	checkError("4");
	// unbind the VAO, the program and the texture
	glBindVertexArray(0);
	diffProgram->stopUsing();
	fboGridSmall.unbind();
	glViewport(0, 0, wWidth, wHeight);
}
static void finalArtPass(){
	fbo.bind();
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);

	glClearColor(0.0,0.0, 0.0, 1);
	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	rttArtProgram->use();
	glActiveTexture(GL_TEXTURE0 );
	glBindTexture(GL_TEXTURE_2D, htextures[0]->object());
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, fboGridBig.getColorTexture(0));
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture(2));

	rttArtProgram->setUniform("tex", 0);
	rttArtProgram->setUniform("brushMap", 1);
	rttArtProgram->setUniform("blurtex", 2);
	rttArtProgram->setUniform("gridSize", gridSizeBig);
	rttArtProgram->setUniform("width", (float)wWidth);
	rttArtProgram->setUniform("height", (float)wHeight);
	glBindVertexArray(rtt_vao);

	// draw the VAO
	glDrawArrays(GL_TRIANGLES, 0, 6);
	fbo.unbind();

	glClearColor(0.0,0.0, 0.0, 1);
	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	rttArtProgram->setUniform("gridSize", gridSizeSmall);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, htextures[1]->object());
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, fboGridSmall.getColorTexture(0));
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture(0));


	glDrawArrays(GL_TRIANGLES, 0, 6);


	// unbind the VAO, the program and the texture

	glBindVertexArray(0);
	rttArtProgram->stopUsing();
	
}
static void finalPass(){
	glClearColor(0.0,0.0, 0.0, 1);
	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	
	rttProgram->use();
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture(0));
	rttProgram->setUniform("texture_color", 2); 
	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture(2));
	rttProgram->setUniform("texture_blur", 3);

	glBindVertexArray(rtt_vao);

	// draw the VAO
	glDrawArrays(GL_TRIANGLES, 0, 6);

	// unbind the VAO, the program and the texture

	glBindVertexArray(0);
	rttProgram->stopUsing();
}

static void twoDRender(){
	//pass 1: normal render to texture	
	firstPass(1.5,0);
	checkError("first pass");

	//pass 2: blur 
	secondPass(3);
	checkError("second pass");

	//pass 3: difference 
	thirdPass();
	checkError("third pass");

	fourthPass();
	checkError("fourth pass");

	//pass 4: texture to OGL
	finalArtPass();
	checkError("final pass");
}

static void threeDRender(){
	firstPass(1.2,0);
	checkError("first pass");
	//pass 2: blur 
	secondPass(9);
	checkError("third pass");

	finalPass();
	//debugPass();
}
// draws a single frame
static void Render() {
	//twoDRender();
	threeDRender();
	

    // swap the display buffers (displays what was just drawn)
    glfwSwapBuffers(gWindow);

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
	}else if (glfwGetKey(gWindow, 'C')){
		cluster++;
		if (cluster == gVAOs.size()) cluster = 0;
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
	fbo.resize(width, height);
	fboGridBig.resize(width / gridSizeBig, height / gridSizeBig);
	fboGridSmall.resize(width / gridSizeSmall, height / gridSizeSmall);
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
	//glEnable(GL_BLEND); 
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
	gridSizeBig = 25;
	gridSizeSmall =18;
	fbo.GenerateFBO(wWidth, wHeight,4);
	fboGridBig.GenerateFBO(wWidth / gridSizeBig, wHeight / gridSizeBig,1);
	fboGridSmall.GenerateFBO(wWidth / gridSizeSmall, wHeight / gridSizeSmall,1);

    // setup gCamera
    gCamera.setPosition(glm::vec3(0,0,0));
	
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
