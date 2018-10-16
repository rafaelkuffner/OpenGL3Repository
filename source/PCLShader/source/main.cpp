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

#define pcl180
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
#ifdef pcl180
	#include <pcl/conversions.h>
#endif 

#include <pcl/common/transforms.h>

// region growing segmentation
#include <pcl/filters/passthrough.h>
#ifdef pcl180
	#include <pcl/segmentation/region_growing.h>
	#include <pcl/segmentation/region_growing_rgb.h>
#endif
#include <pcl/filters/extract_indices.h>
#ifndef pcl180
	#include <pcl/segmentation/extract_clusters.h>
#endif
using namespace pcl;
// constants
const glm::vec2 SCREEN_SIZE(1280, 720);
#define ABUFFER_SIZE			70
#define ABUFFER_PAGE_SIZE		4

//Because current glew does not define it
#ifndef GL_SHADER_GLOBAL_ACCESS_BARRIER_BIT_NV
#define GL_SHADER_GLOBAL_ACCESS_BARRIER_BIT_NV             0x00000010
#endif

// globals
GLFWwindow* gWindow = NULL;
double gScrollY = 0.0;
std::vector<tdogl::Texture*> hBtextures;
std::vector<tdogl::Texture*> vBtextures;
std::vector<tdogl::Texture*> rBtextures;
std::vector<tdogl::Texture*> htextures;
std::vector<tdogl::Texture*> vtextures;
std::vector<tdogl::Texture*> rtextures;
tdogl::Program* gProgram = NULL;
tdogl::Program* preAbuffProgram = NULL;
tdogl::Program* postAbuffProgram = NULL;
tdogl::Program* blurProgram = NULL;
tdogl::Program* rttProgram = NULL;
tdogl::Program* diffProgram = NULL;
tdogl::Program* rttArtProgram = NULL;
tdogl::Program* g2Program = NULL;
tdogl::Program* brushStrokeProgram = NULL;

tdogl::Camera gCamera;
FrameBuffer fbo;
FrameBuffer fboGridBig;
FrameBuffer fboGridSmall;

std::vector<std::vector<GLuint>> gVAOs;
std::vector<GLsizei> numElements;
std::vector<int> brushtype;
GLfloat gDegreesRotated = 0.0f;
glm::vec2 lastMousePos(0, 0);
std::vector<float> resolutions;
unsigned int rtt_vbo, rtt_ibo, rtt_vao;
int wWidth, wHeight;
int gridSizeBig, gridSizeSmall;
bool paint = false;
int cloud = 0;
std::vector<glm::vec4> defColors;
glm::vec4 pBackgroundColor(0.1, 0.1,0.1, 1);
float alph = 1.0;
float saturation = 1.2;
float epsilon = 0.3;
float patchScale = 0.7;
// loads the vertex shader and fragment shader, and links them to make the global gProgram
std::string cloudName;
int style = 0;
using namespace std;
int normalMethod = 5;
//ABuffer textures (pABufferUseTextures)
GLuint abufferID = 0;
GLuint abufferZID = 0;
GLuint abufferCounterID = 0;
GLuint abufferCounterIncID = 0;

int pResolveAlphaCorrection = 0;
int pABufferUseSorting =1;
GLuint vertexBufferName = 0;
GLuint vertexAttribName = 0;
std::vector<tdogl::ShaderMacroStruct>	shadersMacroList;

bool dirty = false;
int cleanframes = 0;

bool debug = false;
float debugCount = 0.0;
bool dualLayer = false;

// Brush Stroke Gaussians
float a = 1.07f;
float sigmax = 0.142f; float sigmay = 0.0709f;
float gamma = 2.7;
float dev = 0.38;

void resetShadersGlobalMacros(){
	shadersMacroList.clear();
}

void setShadersGlobalMacro(const char *macro, int val){
	tdogl::ShaderMacroStruct ms;
	ms.macro = std::string(macro);

	char buff[128];
	sprintf(buff, "%d", val);
	ms.value = std::string(buff);

	shadersMacroList.push_back(ms);
}
void setShadersGlobalMacro(const char *macro, float val){
	tdogl::ShaderMacroStruct ms;
	ms.macro = std::string(macro);

	char buff[128];
	sprintf(buff, "%ff", val);

	ms.value = std::string(buff);

	shadersMacroList.push_back(ms);
}

static void checkError(const char* msg){
	GLenum error;
	error = glGetError();
	if (error != GL_NO_ERROR)
		std::cerr << "OpenGL Error " << msg << " " << error << std::endl;

}
vector<string> GetFilesInDirectory(const string directory)
{
	HANDLE dir;
	WIN32_FIND_DATA file_data;
	vector<string> out;
	string direc = directory + "/*";
	const char* temp = direc.c_str();

	if ((dir = FindFirstFile(temp, &file_data)) == INVALID_HANDLE_VALUE)
		return out;

	do {
		const string file_name = file_data.cFileName;
		const string full_file_name = directory + "\\" + file_name;
		const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

		if (file_name[0] == '.')
			continue;

		if (is_directory)
			continue;

		out.push_back(full_file_name);
	} while (FindNextFile(dir, &file_data));

	FindClose(dir);
	return out;
}


tdogl::Program *createProgram(std::string vertexShader, std::string fragmentShader, string output, std::string geometryShader=""){
	std::vector<tdogl::Shader> shaders;
	shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath(vertexShader), GL_VERTEX_SHADER,shadersMacroList));
	shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath(fragmentShader), GL_FRAGMENT_SHADER, shadersMacroList));
	if (geometryShader != "")
		shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath(geometryShader), GL_GEOMETRY_SHADER, shadersMacroList));

	tdogl::Program *res = new tdogl::Program(shaders);
	glBindFragDataLocation(res->object(), 0, output.c_str());
	return res;
}

static void LoadShaders_2D(){

	diffProgram = createProgram("rtt.vert", "diff.frag","BrushColor");
	
	rttArtProgram = createProgram("rtt.vert", "rttArt.frag","FinalColor");
}

static void LoadShaders_3D(){

	preAbuffProgram = createProgram("passThrough.vert", "clearABuffer.frag", "");
	
	gProgram = createProgram("vertex-shader.vert", "fragment-shader.frag", "finalColor", "normals-shader.geom");
	
	postAbuffProgram = createProgram("passThrough.vert", "dispABuffer.frag", "outFragColor");

	rttProgram = createProgram("rtt.vert", "rtt.frag", "outColor");

	blurProgram = createProgram("rtt.vert", "blur.frag", "FragmentColor");

	g2Program = createProgram("simp.vert", "simp.frag", "outColor");

	brushStrokeProgram = createProgram("vertex-shader.vert", "frag-brush-shader.frag", "finalColor", "normals-shader.geom");
	
}
static void LoadABuffer(){
	//Full screen quad initialization
	glGenVertexArrays(1, &vertexAttribName);
	glBindVertexArray(vertexAttribName);

	glGenBuffers(1, &vertexBufferName);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferName);
	//Full screen quad vertices definition
	const GLfloat quadVArray[] = {
		-1.0f, -1.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f
	};

	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVArray), quadVArray, GL_STATIC_DRAW);
	glEnableVertexAttribArray(preAbuffProgram->attrib("vertexPos"));
	glVertexAttribPointer(preAbuffProgram->attrib("vertexPos"), 4, GL_FLOAT, GL_FALSE,
		sizeof(GLfloat) * 4, 0);

	checkError("initBuffer");


	///ABuffer storage///
	if (!abufferID)
		glGenTextures(1, &abufferID);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, abufferID);

	// Set filter
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//Texture creation
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowSize.x, windowSize.y*ABUFFER_SIZE, 0,  GL_RGBA, GL_FLOAT, 0);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, wWidth, wHeight, ABUFFER_SIZE, 0, GL_RGBA, GL_FLOAT, 0);
	glBindImageTextureEXT(0, abufferID, 0, true, 0, GL_READ_WRITE, GL_RGBA32F);

	checkError("AbufferTex");

	///ABuffer per-pixel counter///
	if (!abufferCounterID)
		glGenTextures(1, &abufferCounterID);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, abufferCounterID);

	// Set filter
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//Texture creation
	//Uses GL_R32F instead of GL_R32I that is not working in R257.15
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, wWidth, wHeight, 0, GL_RED, GL_FLOAT, 0);
	glBindImageTextureEXT(1, abufferCounterID, 0, false, 0, GL_READ_WRITE, GL_R32UI);

	checkError("Abuffer");

	///ABuffer per-pixel counter///
	if (!abufferCounterIncID)
		glGenTextures(1, &abufferCounterIncID);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, abufferCounterIncID);

	// Set filter
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//Texture creation
	//Uses GL_R32F instead of GL_R32I that is not working in R257.15
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, wWidth, wHeight, 0, GL_RED, GL_FLOAT, 0);
	glBindImageTextureEXT(2, abufferCounterIncID, 0, false, 0, GL_READ_WRITE, GL_R32UI);

	checkError("Abuffer");

	///ABuffer storage///
	if (!abufferZID)
		glGenTextures(1, &abufferZID);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D_ARRAY, abufferZID);

	// Set filter
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//Texture creation
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowSize.x, windowSize.y*ABUFFER_SIZE, 0,  GL_RGBA, GL_FLOAT, 0);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, wWidth, wHeight, ABUFFER_SIZE, 0, GL_RGBA, GL_FLOAT, 0);
	glBindImageTextureEXT(3, abufferZID, 0, true, 0, GL_READ_WRITE, GL_RGBA32F);

	checkError("AbufferZTex");
}


static void LoadShaders() {

	resetShadersGlobalMacros();

	setShadersGlobalMacro("ABUFFER_SIZE", ABUFFER_SIZE);
	setShadersGlobalMacro("SCREEN_WIDTH", wWidth);
	setShadersGlobalMacro("SCREEN_HEIGHT", wHeight);

	setShadersGlobalMacro("BACKGROUND_COLOR_R", pBackgroundColor.r);
	setShadersGlobalMacro("BACKGROUND_COLOR_G", pBackgroundColor.g);
	setShadersGlobalMacro("BACKGROUND_COLOR_B", pBackgroundColor.b);
	setShadersGlobalMacro("ABUFFER_RESOLVE_ALPHA_CORRECTION", pResolveAlphaCorrection);

	LoadShaders_3D();
	//LoadShaders_2D();

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



static std::vector<PointCloud<PointXYZRGB>::Ptr> segmentPointCloud(PointCloud<PointXYZRGB>::Ptr cloud){

	pcl::search::Search <pcl::PointXYZRGB>::Ptr tree = boost::shared_ptr<pcl::search::Search<pcl::PointXYZRGB> >(new pcl::search::KdTree<pcl::PointXYZRGB>);
	std::vector<PointCloud<PointXYZRGB>::Ptr> results;
#ifdef pcl180
	pcl::RegionGrowingRGB<pcl::PointXYZRGB> reg;
	reg.setDistanceThreshold(100);
	reg.setPointColorThreshold(5.0);
	reg.setRegionColorThreshold(14);
	reg.setInputCloud(cloud);
	reg.setSearchMethod(tree);
	reg.setMinClusterSize(200);
	 //reg.setSearchMethod(tree);
	 //reg.setDistanceThreshold(1000);
	 //reg.setPointColorThreshold(5.7);
	 //reg.setRegionColorThreshold(7);
	 //reg.setMinClusterSize(500);

	 std::vector <pcl::PointIndices> clusters;
	 reg.extract(clusters);
	 for (std::vector<pcl::PointIndices>::const_iterator it = clusters.begin(); it != clusters.end(); ++it)
	 {
		 pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_cluster(new pcl::PointCloud<pcl::PointXYZRGB>);
		 for (std::vector<int>::const_iterator pit = it->indices.begin(); pit != it->indices.end(); ++pit)
			 cloud_cluster->points.push_back(cloud->points[*pit]);
		 cloud_cluster->width = cloud_cluster->points.size();
		 cloud_cluster->height = 1;
		 cloud_cluster->is_dense = true;
		 results.push_back(cloud_cluster);
		 float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		 float g = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		 float b = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		 defColors.push_back(glm::vec4(r, g, b, 1.0));
	 }
#else
	results.push_back(cloud);
#endif
	 return results;
}

void loadPly(PointCloud<PointXYZRGB>::Ptr cloud, std::string path){
	PolygonMesh mesh; 
	string str2 = "D:";
	std::size_t found = path.find(str2);
	if (found == std::string::npos)
		io::loadPolygonFilePLY(ResourcePath(path), mesh);
	else
		io::loadPolygonFilePLY(path, mesh);
#ifdef pcl180
	fromPCLPointCloud2(mesh.cloud, *cloud);
#endif
#ifndef pcl180
	pcl::fromROSMsg(mesh.cloud, *cloud);
#endif
}

void readKinectCloudDisk(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud, std::string pointCloudPath,std::string outpath){
	std::ifstream f;
	f.open(pointCloudPath, fstream::in);
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



static float CloudPreprocess(std::vector<PointCloud<PointXYZRGB>::Ptr> &cloud_clusters,string cname){

	PointCloud<PointXYZRGB>::Ptr cloud(new PointCloud<PointXYZRGB>);
	//readKinectCloudDisk(cloud, cname, "test.ply");
	loadPly(cloud,cname );

	Eigen::Vector4f centroid;
	Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
	pcl::compute3DCentroid(*cloud, centroid);
	transform(0, 3) = -centroid.x();
	transform(1, 3) = -centroid.y();
	transform(2, 3) = -centroid.z();
	pcl::transformPointCloud(*cloud, *cloud, transform);

	cloud_clusters= segmentPointCloud(cloud);
	PointXYZRGB min, max;
	pcl::getMinMax3D(*cloud, min, max);
	float size = sqrt(pow(max.x - min.x, 2) + pow(max.z - min.z, 2) + pow(max.z - min.z, 2));
	return size;
}


// loads a cube into the VAO and VBO globals: gVAO and gVBO
static void LoadCloud(string cname) {

	std::vector<PointCloud<PointXYZRGB>::Ptr> cloud_clusters;
	float totalSize = CloudPreprocess(cloud_clusters,cname);
	std::vector<GLuint> arrayobjs;
	int max = 0;
	for (int i = 0; i < cloud_clusters.size(); i++){
		max = max > cloud_clusters[i]->size() ? max : cloud_clusters[i]->size();
	}
	float *varray = (float*)malloc(sizeof(float)*max * 4);
	float *carray = (float*)malloc(sizeof(float)*max * 4);
	float *narray = (float*)malloc(sizeof(float)*max * 4);
	for (int k = 0; k < cloud_clusters.size(); k++){
		GLuint gVAO;
		glGenVertexArrays(1, &gVAO);
		glBindVertexArray(gVAO);

		arrayobjs.push_back(gVAO);
		numElements.push_back(cloud_clusters[k]->size());
		// make and bind the VAO


		resolutions.push_back(computeCloudResolution(cloud_clusters[k]));
		PointCloud<PointXYZRGBNormal>::Ptr cloud_with_normals(new PointCloud<PointXYZRGBNormal>);
		PointCloud<Normal>::Ptr normals(new PointCloud<Normal>);
		NormalEstimation<PointXYZRGB, Normal> ne;

		search::KdTree<pcl::PointXYZRGB>::Ptr tree(new search::KdTree<pcl::PointXYZRGB>());
		ne.setSearchMethod(tree);
		ne.setViewPoint(0, 0, 20);
		//ne.setKSearch(2);
		ne.setRadiusSearch(5*resolutions[k]);
		ne.setInputCloud(cloud_clusters[k]);
		ne.compute(*normals);
		pcl::concatenateFields(*cloud_clusters[k], *normals, *cloud_with_normals);

		PointXYZRGBNormal min, max;
		pcl::getMinMax3D(*cloud_with_normals, min, max);
		float sizecluster = sqrt(pow(max.x - min.x, 2) + pow(max.z - min.z, 2) + pow(max.z - min.z, 2));

		Eigen::Vector4f centroid;
		pcl::compute3DCentroid(*cloud_with_normals, centroid);

		


		float sizex = fabs(max.x - min.x);
		float sizey = fabs(max.y - min.y);
		float sizez = fabs(max.z - min.z);
		int brush;
		//vertical brush
		if (sizecluster < totalSize ){
			if (sizey > sizex && sizey > sizez)
				brush = 1;
			if (sizex > sizey || sizez > sizey){
				brush = 0;
			}
			else{
				brush = 2;
			}
		}
		else{
			brush = 3;
		}
		
		brushtype.push_back(brush);
		


		
		int i = 0;
		for (PointCloud<PointXYZRGBNormal>::iterator it = cloud_with_normals->begin(); it != cloud_with_normals->end(); it++){
			float r = it->r / 255.0;
			float g = it->g / 255.0;
			float b = it->b / 255.0;

			float nx, ny, nz;
			//flipNormalTowardsViewpoint(*it, centroid.x(), centroid.y(), centroid.z(), it->normal_x, it->normal_y, it->normal_z);
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
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*numElements[k]*4, varray, GL_STATIC_DRAW);
		// connect the xyz to the "vert" attribute of the vertex shader
		glEnableVertexAttribArray(gProgram->attrib("vert"));
		glVertexAttribPointer(gProgram->attrib("vert"), 4, GL_FLOAT, GL_FALSE,0, NULL);


		glGenBuffers(1, &gVBO2);
		glBindBuffer(GL_ARRAY_BUFFER, gVBO2);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*numElements[k] * 4, carray, GL_STATIC_DRAW);
		// connect the color to the "col" attribute of the vertex shader
		glEnableVertexAttribArray(gProgram->attrib("col"));
		glVertexAttribPointer(gProgram->attrib("col"), 4, GL_FLOAT, GL_FALSE,0, NULL);
		

		glGenBuffers(1, &gVBO3);
		glBindBuffer(GL_ARRAY_BUFFER, gVBO3);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*numElements[k] * 4, narray, GL_STATIC_DRAW);
		// connect the normal to the "norm" attribute of the vertex shader
		glEnableVertexAttribArray(gProgram->attrib("norm"));
		glVertexAttribPointer(gProgram->attrib("norm"), 4, GL_FLOAT, GL_FALSE, 0, NULL);


	}
	free(varray);
	free(narray);
	free(carray);
	gVAOs.push_back(arrayobjs);
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
		std::stringstream s1, s2, s3,s1b,s2b,s3b;
		
		//pointilism
		/*s1 << "vertbrush\\b" << i << ".png";
		s1b << "vertbrush\\b" << i << ".png";
		s2 << "horizbrush\\b" << i << ".png";
		s2b << "horizbrush\\b" << i << ".png";
		s3 << "roundbrush\\b" << i << ".png";
		s3b << "roundbrush\\b" << i << ".png"; */
		//impressionism
		s1 << "vert3brush\\b" << i << ".png";
		s1b << "vertbrushbig\\b" << i << ".png";
		s2 << "vert3brush\\b" << i << ".png";
		s2b << "vertbrushbig\\b" << i << ".png";
		s3 << "vert3brush\\b" << i << ".png";
		s3b << "vertbrushbig\\b" << i << ".png"; 

		tdogl::Bitmap bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(s1.str()));
		bmp.flipVertically();
		htextures.push_back(new tdogl::Texture(bmp));
		
		bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(s1b.str()));
		bmp.flipVertically();
		hBtextures.push_back(new tdogl::Texture(bmp));
		
		bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(s2.str()));
		bmp.flipVertically();
		vtextures.push_back(new tdogl::Texture(bmp));
		
		bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(s2b.str()));
		bmp.flipVertically();
		vBtextures.push_back(new tdogl::Texture(bmp));

		bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(s3.str()));
		bmp.flipVertically();
		rtextures.push_back(new tdogl::Texture(bmp));

		bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(s3b.str()));
		bmp.flipVertically();
		rBtextures.push_back(new tdogl::Texture(bmp));
		std::stringstream s1a, s2a, s3a, s1ba, s2ba, s3ba;
		//expressionism
		s1a << "roundbrush\\b" << i << ".png";
		s1ba << "roundbrush\\b" << i << ".png";
		s2a << "roundbrush\\b" << i << ".png";
		s2ba << "roundbrush\\b" << i << ".png";
		s3a << "roundbrush\\b" << i << ".png";
		s3ba << "roundbrush\\b" << i << ".png";


		bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(s1a.str()));
		bmp.flipVertically();
		htextures.push_back(new tdogl::Texture(bmp));

		bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(s1ba.str()));
		bmp.flipVertically();
		hBtextures.push_back(new tdogl::Texture(bmp));

		bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(s2a.str()));
		bmp.flipVertically();
		vtextures.push_back(new tdogl::Texture(bmp));

		bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(s2ba.str()));
		bmp.flipVertically();
		vBtextures.push_back(new tdogl::Texture(bmp));

		bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(s3a.str()));
		bmp.flipVertically();
		rtextures.push_back(new tdogl::Texture(bmp));

		bmp = tdogl::Bitmap::bitmapFromFile(ResourcePath(s3ba.str()));
		bmp.flipVertically();
		rBtextures.push_back(new tdogl::Texture(bmp));
	}
}




static void firstPass(float resolutionMult, int outbuf){

	fbo.bind();
	{
		GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 +outbuf};
		glDrawBuffers(1, DrawBuffers);

		// clear everything
		glClearColor(pBackgroundColor.r, pBackgroundColor.g, pBackgroundColor.b, pBackgroundColor.a);
		// black
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// bind the program (the shaders)
		gProgram->use();
		// set the "camera" uniform
		gProgram->setUniform("camera", gCamera.matrix());
		// set the camera position uniform
		gProgram->setUniform("cameraPosition", gCamera.position());
		// set the "model" uniform in the vertex shader, based on the gDegreesRotated global
		gProgram->setUniform("model", glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0, 1, 0)));

		gProgram->setUniform("aBuffer", false);
		// bind the texture and set the "tex" uniform in the fragment shader
	
		
		for (int j = 0; j < gVAOs[cloud].size(); j++){
			// bind the VAO
			glBindVertexArray(gVAOs[cloud][j]);

			for (int i = 0; i < 10; i++){
				
				glActiveTexture(GL_TEXTURE0 + i);
				int idx = style > 15? 1 : 0;
				if (brushtype[j] == 0)
					glBindTexture(GL_TEXTURE_2D, vBtextures[2 * i+idx ]->object());
				if (brushtype[j] == 1)
					glBindTexture(GL_TEXTURE_2D, hBtextures[2 * i + idx]->object());
				if (brushtype[j] == 2)
					glBindTexture(GL_TEXTURE_2D, rBtextures[2 * i + idx]->object());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				std::stringstream s;
				s << "tex" << i;
				gProgram->setUniform(s.str().c_str(), i);
			}

			gProgram->setUniform("resolution", resolutions[j] * resolutionMult);
			float a = 1.0;
			float s = 1.0;
			float ps = 1.0;
			gProgram->setUniform("normalMethod", normalMethod);
			gProgram->setUniform("saturation", s);
			gProgram->setUniform("alph", a);
			gProgram->setUniform("scale", ps);
			// draw the VAO
			glDrawArrays(GL_POINTS, 0, numElements[j]);

			checkError("first pass 1");
			for (int i = 0; i < 10; i++){
				glActiveTexture(GL_TEXTURE0 + i);
				int idx = style > 15 ? 1 : 0;
				if (brushtype[j] == 0)
					glBindTexture(GL_TEXTURE_2D, vtextures[2 * i + idx]->object());
				if (brushtype[j] == 1)
					glBindTexture(GL_TEXTURE_2D, htextures[2 * i + idx]->object());
				if (brushtype[j] == 2)
					glBindTexture(GL_TEXTURE_2D, rtextures[2 * i + idx]->object());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				std::stringstream s;
				s << "tex" << i;
				gProgram->setUniform(s.str().c_str(), i);
			}
			float res = resolutionMult * patchScale;

			a = alph;
			s = saturation;
			ps = 1.0005;
			gProgram->setUniform("saturation", s);
			gProgram->setUniform("resolution", resolutions[j] * res);
			gProgram->setUniform("alph", a);
			gProgram->setUniform("scale", ps);
			// draw the VAO
			//glDrawArrays(GL_POINTS, 0, numElements[j]);

			checkError("first pass 2");

		}

		// unbind the VAO, the program and the texture
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		gProgram->stopUsing();
	}
	fbo.unbind();

}


static void firstPass_genBrush(float resolutionMult, int outbuf){

	fbo.bind();
	{
		GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 + outbuf };
		glDrawBuffers(1, DrawBuffers);

		// clear everything
		glClearColor(pBackgroundColor.r, pBackgroundColor.g, pBackgroundColor.b, pBackgroundColor.a);
		// black
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// bind the program (the shaders)
		brushStrokeProgram->use();
		// set the "camera" uniform
		glm::mat4 matrix = gCamera.matrix();
		brushStrokeProgram->setUniform("camera", matrix);
		// set the "model" uniform in the vertex shader, based on the gDegreesRotated global
		brushStrokeProgram->setUniform("model", glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0, 1, 0)));

		brushStrokeProgram->setUniform("aBuffer", false);
		// bind the texture and set the "tex" uniform in the fragment shader


		for (int j = 0; j < gVAOs[cloud].size(); j++){
			// bind the VAO
			glBindVertexArray(gVAOs[cloud][j]);
			brushStrokeProgram->setUniform("brushType", brushtype[j]);
			brushStrokeProgram->setUniform("dev", dev);
			brushStrokeProgram->setUniform("gamma", gamma);
			brushStrokeProgram->setUniform("sigmax", sigmax);
			brushStrokeProgram->setUniform("sigmay", sigmay);
			brushStrokeProgram->setUniform("a", a);
			brushStrokeProgram->setUniform("resolution", resolutions[j] * resolutionMult);
			float a = 1.0;
			float s = 1.0;
			float ps = 1.0;
			brushStrokeProgram->setUniform("normalMethod", normalMethod);
			brushStrokeProgram->setUniform("saturation", s);
			brushStrokeProgram->setUniform("alph", a);
			brushStrokeProgram->setUniform("scale", ps);
			// draw the VAO
			glDrawArrays(GL_POINTS, 0, numElements[j]);

			checkError("first pass 1");
			
			//float res = resolutionMult * patchScale;
			//a = alph;
			//s = saturation;
			//ps = 1.0005;
			//gProgram->setUniform("saturation", s);
			//gProgram->setUniform("resolution", resolutions[j] * res);
			//gProgram->setUniform("alph", a);
			//gProgram->setUniform("scale", ps);
			// draw the VAO
			//glDrawArrays(GL_POINTS, 0, numElements[j]);

			//checkError("first pass 2");

		}

		// unbind the VAO, the program and the texture
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		brushStrokeProgram->stopUsing();
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
	glClearColor(pBackgroundColor.r, pBackgroundColor.g, pBackgroundColor.b, pBackgroundColor.a);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture(0));
	blurProgram->setUniform("d", 0);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	
	//pass 2.2, horiz uses fbo text as input
	glDrawBuffers(1, db2);
	glClearColor(pBackgroundColor.r, pBackgroundColor.g, pBackgroundColor.b, pBackgroundColor.a);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture(1));
	blurProgram->setUniform("d", 1);
	glDrawArrays(GL_TRIANGLES, 0, 6);


	for (int i = 0; i < iterations; i++){

		glDrawBuffers(1, db1);
		glClearColor(pBackgroundColor.r, pBackgroundColor.g, pBackgroundColor.b, pBackgroundColor.a);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture(2));
		blurProgram->setUniform("d", 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		

		glDrawBuffers(1, db2);
		glClearColor(pBackgroundColor.r, pBackgroundColor.g, pBackgroundColor.b, pBackgroundColor.a);

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
	glClearColor(pBackgroundColor.r, pBackgroundColor.g, pBackgroundColor.b, pBackgroundColor.a);

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
	glClearColor(pBackgroundColor.r, pBackgroundColor.g, pBackgroundColor.b, pBackgroundColor.a);

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

	glClearColor(pBackgroundColor.r, pBackgroundColor.g, pBackgroundColor.b, pBackgroundColor.a);

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

	glClearColor(pBackgroundColor.r, pBackgroundColor.g, pBackgroundColor.b, pBackgroundColor.a);

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
static void finalPass(int blend){
	glClearColor(pBackgroundColor.r, pBackgroundColor.g, pBackgroundColor.b, pBackgroundColor.a);

	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	
	rttProgram->use();
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture(0));
	rttProgram->setUniform("texture_color", 2); 
	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_2D, fbo.getColorTexture(blend));
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
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_CULL_FACE);
	////glEnable(GL_STENCIL_TEST);
	glDepthMask(GL_TRUE);

	firstPass(1 + epsilon,0);
	checkError("first pass");
	//pass 2: blur 
	secondPass(1);
	checkError("third pass");

	finalPass(2);
	//debugPass();
}

static void threeDRenderNoBlur(){
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_CULL_FACE);
	////glEnable(GL_STENCIL_TEST);
	glDepthMask(GL_TRUE);
	
	firstPass(1 + epsilon, 0);
	checkError("first pass");
	//pass 2: blur 

	finalPass(0);
	//debugPass();
}

static void threeDRenderNoBlur_genBrush(){
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_CULL_FACE);
	////glEnable(GL_STENCIL_TEST);
	glDepthMask(GL_TRUE);

	firstPass_genBrush(1 + epsilon, 0);
	checkError("first pass");
	//pass 2: blur 
	secondPass(1);
	checkError("third pass");

	finalPass(2);
	//debugPass();
}

static void cloudRender(){
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	checkError("begin2");
	//glEnable(GL_CULL_FACE);
	////glEnable(GL_STENCIL_TEST);
	glDepthMask(GL_TRUE);
	glPointSize(2.0);
	glLineWidth(3.0);
	// clear everything
	glClearColor(pBackgroundColor.r, pBackgroundColor.g, pBackgroundColor.b, pBackgroundColor.a);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// bind the program (the shaders)
	g2Program->use();

	// set the "camera" uniform
	g2Program->setUniform("camera", gCamera.matrix());


	// set the camera position uniform
	//gProgram->setUniform("camPosition", gCamera.position());
	// set the "model" uniform in the vertex shader, based on the gDegreesRotated global
	g2Program->setUniform("model", glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0, 1, 0)));


	// bind the texture and set the "tex" uniform in the fragment shader

	for (int j = 0; j < gVAOs[cloud].size(); j++){
		// bind the VAO

		g2Program->setUniform("defaultColor", defColors[j]);
		glBindVertexArray(gVAOs[cloud][j]);
		// draw the VAO
		glDrawArrays(GL_POINTS, 0, numElements[j]);
	}

	checkError("draw");
	// unbind the VAO, the program and the texture
	glBindVertexArray(0);
	g2Program->stopUsing();
}


void drawQuad(tdogl::Program *prog) {

	glUseProgram(prog->object());

	glBindVertexArray(vertexAttribName);
	checkError("error pointer");
	glDrawArrays(GL_TRIANGLES, 0, 24);

	checkError ("drawQuad");
}

void clearABuffer(){
	//Assign uniform parameters
	glProgramUniform1iEXT(preAbuffProgram->object(), preAbuffProgram->uniform("abufferImg"), 0);
	glProgramUniform1iEXT(preAbuffProgram->object(), preAbuffProgram->uniform("abufferCounterImg"), 1);
	glProgramUniform1iEXT(preAbuffProgram->object(), preAbuffProgram->uniform("abufferCounterIncImg"), 2);
	glProgramUniform1iEXT(preAbuffProgram->object(), preAbuffProgram->uniform("abufferZImg"), 2);


	//Render the full screen quad
	drawQuad(preAbuffProgram);

	//Ensure that all global memory write are done before starting to render
	glMemoryBarrierEXT(GL_SHADER_GLOBAL_ACCESS_BARRIER_BIT_NV);
	
	checkError("clear a buffer ");
}

void resolveABuffer(){
	//Ensure that all global memory write are done before resolving
	glMemoryBarrierEXT(GL_SHADER_GLOBAL_ACCESS_BARRIER_BIT_NV);

	glProgramUniform1iEXT(postAbuffProgram->object(), postAbuffProgram->uniform("abufferImg"), 0);
	glProgramUniform1iEXT(postAbuffProgram->object(), postAbuffProgram->uniform("abufferCounterImg"), 1);
	glProgramUniform1iEXT(postAbuffProgram->object(), postAbuffProgram->uniform("abufferCounterIncImg"), 2);
	glProgramUniform1iEXT(postAbuffProgram->object(), postAbuffProgram->uniform("abufferZImg"), 3);

	drawQuad(postAbuffProgram);
}

static void aBufferRender(float resolutionMult){

	glDisable(GL_CULL_FACE);
	//Disable depth test
	glDisable(GL_DEPTH_TEST);
	//Disable stencil test
	glDisable(GL_STENCIL_TEST);
	//Disable blending
	glDisable(GL_BLEND);

	glDepthMask(GL_FALSE);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor(pBackgroundColor.r, pBackgroundColor.g, pBackgroundColor.b, pBackgroundColor.a);
	glClear(GL_COLOR_BUFFER_BIT);

	if (cleanframes ==0){
		clearABuffer();
		// clear everything

		//stroke based rendering
		// bind the program (the shaders)
		gProgram->use();
		gProgram->setUniform("camera", gCamera.matrix());
		gProgram->setUniform("model", glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0, 1, 0)));
		gProgram->setUniform("aBuffer", true);

		// bind the texture and set the "tex" uniform in the fragment shader


		glProgramUniform1iEXT(gProgram->object(), gProgram->uniform("abufferImg"), 0);
		glProgramUniform1iEXT(gProgram->object(), gProgram->uniform("abufferCounterImg"), 1);
		glProgramUniform1iEXT(gProgram->object(), gProgram->uniform("abufferZImg"), 3);


		for (int j = 0; j < gVAOs[cloud].size(); j++){
			glBindVertexArray(gVAOs[cloud][j]);

			//loading textures
			for (int i = 0; i < 10; i++){
				glActiveTexture(GL_TEXTURE4 + i);
				int idx = style > 15 ? 1 : 0;
				if (brushtype[j] == 0)
					glBindTexture(GL_TEXTURE_2D, vBtextures[2 * i + idx]->object());
				if (brushtype[j] == 1)
					glBindTexture(GL_TEXTURE_2D, hBtextures[2 * i + idx]->object());
				if (brushtype[j] == 2)
					glBindTexture(GL_TEXTURE_2D, rBtextures[2 * i + idx]->object());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				std::stringstream s;
				s << "tex" << i;
				gProgram->setUniform(s.str().c_str(), i + 4);
			}


			gProgram->setUniform("resolution", resolutions[j] * resolutionMult);
			float a = 1.0;
			float s = 1.0;
			float ps = 1.0;
			gProgram->setUniform("normalMethod",normalMethod);
			gProgram->setUniform("saturation", s);
			gProgram->setUniform("alph", a);
			gProgram->setUniform("scale", ps);
			// draw the VAO
			glDrawArrays(GL_POINTS, 0, numElements[j]);

			checkError("first pass 1");
			for (int i = 0; i < 10; i++){
				glActiveTexture(GL_TEXTURE4 + i);
				int idx = style > 15 ? 1 : 0;
				if (brushtype[j] == 0)
					glBindTexture(GL_TEXTURE_2D, vtextures[2 * i + idx]->object());
				if (brushtype[j] == 1)
					glBindTexture(GL_TEXTURE_2D, htextures[2 * i + idx]->object());
				if (brushtype[j] == 2)
					glBindTexture(GL_TEXTURE_2D, rtextures[2 * i + idx]->object());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				std::stringstream s;
				s << "tex" << i;
				gProgram->setUniform(s.str().c_str(), i + 4);
			}
			float res = resolutionMult * patchScale;

			a = alph;
			s = saturation;
			ps = 1.0005;
			gProgram->setUniform("saturation", s);
			gProgram->setUniform("resolution", resolutions[j] * res);
			gProgram->setUniform("alph", a);
			gProgram->setUniform("scale", ps);
			// draw the VAO
			//glDrawArrays(GL_POINTS, 0, numElements[j]);

			checkError("first pass 2");

		}

		// unbind the VAO, the program and the texture
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		gProgram->stopUsing();
	}

	resolveABuffer();
	if (cleanframes < 6){
		finalPass(0);
	}
	cleanframes++;
}


static void aBufferRender_genBrush(float resolutionMult){

	glDisable(GL_CULL_FACE);
	//Disable depth test
	glDisable(GL_DEPTH_TEST);
	//Disable stencil test
	glDisable(GL_STENCIL_TEST);
	//Disable blending
	glDisable(GL_BLEND);

	glDepthMask(GL_FALSE);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor(pBackgroundColor.r, pBackgroundColor.g, pBackgroundColor.b, pBackgroundColor.a);
	glClear(GL_COLOR_BUFFER_BIT);

	if (cleanframes == 0){
		clearABuffer();
		// clear everything

		// bind the program (the shaders)
		brushStrokeProgram->use();
		brushStrokeProgram->setUniform("camera", gCamera.matrix());
		brushStrokeProgram->setUniform("model", glm::rotate(glm::mat4(), glm::radians(gDegreesRotated), glm::vec3(0, 1, 0)));
		brushStrokeProgram->setUniform("aBuffer", true);
		
		//stroke based rendering parameters for frag-brush-shaders
	

		// bind the texture and set the "tex" uniform in the fragment shader


		glProgramUniform1iEXT(brushStrokeProgram->object(), brushStrokeProgram->uniform("abufferImg"), 0);
		glProgramUniform1iEXT(brushStrokeProgram->object(), brushStrokeProgram->uniform("abufferCounterImg"), 1);
		glProgramUniform1iEXT(brushStrokeProgram->object(), brushStrokeProgram->uniform("abufferZImg"), 3);


		for (int j = 0; j < gVAOs[cloud].size(); j++){
			glBindVertexArray(gVAOs[cloud][j]);
			
			brushStrokeProgram->setUniform("brushType", brushtype[j]);
			brushStrokeProgram->setUniform("dev", dev);
			brushStrokeProgram->setUniform("gamma", gamma);
			brushStrokeProgram->setUniform("sigmax", sigmax);
			brushStrokeProgram->setUniform("sigmay", sigmay);
			brushStrokeProgram->setUniform("a", a);

			bool dual = dualLayer;
			brushStrokeProgram->setUniform("dualPaint", dual);

			brushStrokeProgram->setUniform("resolution", resolutions[j] * resolutionMult);
			float a = 1.0;
			float s = saturation;
			float ps = 1.0;
			brushStrokeProgram->setUniform("normalMethod", normalMethod);
			brushStrokeProgram->setUniform("saturation", s);
			brushStrokeProgram->setUniform("alph", a);
			brushStrokeProgram->setUniform("scale", ps);
			// draw the VAO
			glDrawArrays(GL_POINTS, 0, numElements[j]);

			checkError("first pass 1");
			
		}

		// unbind the VAO, the program and the texture
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		brushStrokeProgram->stopUsing();
	}

	resolveABuffer();
	if (cleanframes < 6){
		finalPass(0);
	}
	cleanframes++;
}
// draws a single frame
static void Render() {
	if (paint ){
		if (dirty){
			threeDRenderNoBlur_genBrush();
			dirty = false;
			cleanframes = 0;
		}
		else{
			aBufferRender_genBrush(1 + epsilon);
			if (debug)
				cout << "path size = " << 1 + epsilon << endl;
		}
	}
	else{
		threeDRenderNoBlur_genBrush();
		/*threeDRender();
		cloudRender();*/
	}

    // swap the display buffers (displays what was just drawn)
    glfwSwapBuffers(gWindow);

}

float paintCount = 0;
float dualCount = 0;
// update the scene based on the time elapsed since last update
void Update(float secondsElapsed) {
    //rotate the cube
    const GLfloat degreesPerSecond = -90.0f;
   /* gDegreesRotated += secondsElapsed * degreesPerSecond;
    while(gDegreesRotated > 0.0f) gDegreesRotated += 360.0f;
*/
    //move position of camera based on WASD keys, and XZ keys for up and down
    const float moveSpeed = 0.5; //units per second
    if(glfwGetKey(gWindow, 'S')){
        gCamera.offsetPosition(secondsElapsed * moveSpeed * -gCamera.forward());
		dirty = true;
    } else if(glfwGetKey(gWindow, 'W')){
		gCamera.offsetPosition(secondsElapsed * moveSpeed * gCamera.forward());
		dirty = true;
    }
    if(glfwGetKey(gWindow, 'A')){
		gCamera.offsetPosition(secondsElapsed * moveSpeed * -gCamera.right());
		dirty = true;
    } else if(glfwGetKey(gWindow, 'D')){
		gCamera.offsetPosition(secondsElapsed * moveSpeed * gCamera.right());
		dirty = true;
    }
    if(glfwGetKey(gWindow, 'Q')){
		gCamera.offsetPosition(secondsElapsed * moveSpeed * -glm::vec3(0, 1, 0));
		dirty = true;
    } else if(glfwGetKey(gWindow, 'E')){
		gCamera.offsetPosition(secondsElapsed * moveSpeed * glm::vec3(0, 1, 0));
		dirty = true;
	}else if (glfwGetKey(gWindow, 'C')){
		cloud++;
		if (cloud == gVAOs.size()) cloud = 0;
	}else if (glfwGetKey(gWindow, 'P')){
		paintCount += secondsElapsed;
		if (paintCount > 1){
			paint = !paint;
			paintCount = 0;
			dirty = true;
		}
	}
	else if (glfwGetKey(gWindow, 'F')){
		//alph += 0.01;
		dev += secondsElapsed * 0.1;
		dirty = true;
		//if (alph > 1) alph = 1;
	}
	else if (glfwGetKey(gWindow, 'V')){
		//alph -= 0.01;
		dev -= secondsElapsed *0.1;
		dirty = true;
		//if (alph < 0) alph = 0;
	}
	else if (glfwGetKey(gWindow, 'G')){
		//saturation += 0.01;
		gamma += secondsElapsed * 0.1;
		dirty = true;
	}
	else if (glfwGetKey(gWindow, 'B')){
		//saturation -= 0.01;
		gamma -= secondsElapsed *0.1;
		dirty = true;
		//if (saturation < 0) saturation = 0;
	}
	else if (glfwGetKey(gWindow, 'H')){
		epsilon += secondsElapsed * 0.1;
		dirty = true;
	}
	else if (glfwGetKey(gWindow, 'N')){
		epsilon -= secondsElapsed * 0.1;
		dirty = true;
	}
	else if (glfwGetKey(gWindow, 'J')){
		//patchScale += 0.001;
		sigmax += secondsElapsed * 0.1;
		dirty = true;
	}
	else if (glfwGetKey(gWindow, 'M')){
		//patchScale -= 0.001;
		sigmax -= secondsElapsed *0.1;
		dirty = true;
	}
	else if (glfwGetKey(gWindow, 'K')){
		sigmay += secondsElapsed *0.1;
		dirty = true;
		//style++;
		//if (style > 30){
			//style = 0;
		//	dirty = true;
	//	}
	}
	else if (glfwGetKey(gWindow, 'L')){
		sigmay -= secondsElapsed * 0.1;
		dirty = true;
	}
	else if (glfwGetKey(gWindow, 'I')){
		a += secondsElapsed *  0.5;
		dirty = true;
	}
	else if (glfwGetKey(gWindow, 'O')){
		a -= secondsElapsed * 0.5;
		dirty = true;
	}

	else if (glfwGetKey(gWindow, '1')){
		normalMethod = 1;
		dirty = true;
	}
	else if (glfwGetKey(gWindow, '2')){
		normalMethod = 2;
		dirty = true;
	}
	else if (glfwGetKey(gWindow, '3')){
		normalMethod = 3;
		dirty = true;
	}
	else if (glfwGetKey(gWindow, '4')){
		normalMethod = 4;
		dirty = true;
	}
	else if (glfwGetKey(gWindow, '5')){
		normalMethod = 5;
		dirty = true;
	}
	else if (glfwGetKey(gWindow, '6')){
		normalMethod = 6;
		dirty = true;
	}
	else if (glfwGetKey(gWindow, '7')) {
		normalMethod = 7;
		dirty = true;
	}
	else if (glfwGetKey(gWindow, '8')){
		debugCount += secondsElapsed;
		if (debugCount > 1) {
			debug = !debug;
			debugCount = 0;
			}
		}
	else if (glfwGetKey(gWindow, '9')){
		dualCount += secondsElapsed;
		if (dualCount > 1){
			dualLayer = !dualLayer;
			dualCount = 0;
			dirty = true;
		}
	}



	if (debug)
		cout << " a = " << a << "| dev = " << dev << "| gamma = " << gamma << "| sigmax = " << sigmax << "| sigmay = " << sigmay << endl;

    //rotate camera based on mouse movement
    const float mouseSensitivity = 0.1f;
    double mouseX, mouseY;
	int state = glfwGetMouseButton(gWindow, GLFW_MOUSE_BUTTON_LEFT);
	glfwGetCursorPos(gWindow, &mouseX, &mouseY);
	if (state == GLFW_PRESS){
		dirty = true;
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
	wWidth = width;
	wHeight = height;
	glViewport(0, 0, width, height);
	gCamera.setViewportAspectRatio((float)width / (float)height);
	LoadABuffer();
	LoadShaders(); 
	fbo.resize(width, height);
	fboGridBig.resize(width / gridSizeBig, height / gridSizeBig);
	fboGridSmall.resize(width / gridSizeSmall, height / gridSizeSmall);
	
}

void init(){

	wWidth = SCREEN_SIZE.x;
	wHeight = SCREEN_SIZE.y;

	

	// load vertex and fragment shaders into opengl
	LoadShaders();

	LoadABuffer();

	// load the texture
	//LoadTexture();

	// create buffer and fill it with the points of the triangle
	string str2 = ".ply";
	//std::size_t found = cloudName.find(str2);
	//if (found == std::string::npos){
	//	vector<string> clouds = GetFilesInDirectory(cloudName);
	//	for (int i = 0; i < clouds.size(); i++){
	//		LoadCloud(clouds[i]);
	//	}
	//}
	//else{
		LoadCloud(cloudName);
	//}

	LoadRTTVariables();
	//create frame buffer
	gridSizeBig = 25;
	gridSizeSmall = 18;
	fbo.GenerateFBO(wWidth, wHeight, 4);
	fboGridBig.GenerateFBO(wWidth / gridSizeBig, wHeight / gridSizeBig, 1);
	fboGridSmall.GenerateFBO(wWidth / gridSizeSmall, wHeight / gridSizeSmall, 1);

	// setup gCamera
	gCamera.setPosition(glm::vec3(0, 0, 0));

	gCamera.setViewportAspectRatio(SCREEN_SIZE.x / SCREEN_SIZE.y);
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    gWindow = glfwCreateWindow((int)SCREEN_SIZE.x, (int)SCREEN_SIZE.y, "OpenGL Tutorial", NULL, NULL);
    if(!gWindow)
        throw std::runtime_error("glfwCreateWindow failed. Can your hardware handle OpenGL 3.2?");

    // GLFW settings
    glfwSetInputMode(gWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
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

	init();

    // run while the window is open
    double lastTime = glfwGetTime();
	int nbFrames = 0;
	float lastPrint = lastTime;
    while(!glfwWindowShouldClose(gWindow)){
        // process pending events
        glfwPollEvents();
        
        // update the scene based on the time elapsed since last update
        double thisTime = glfwGetTime();
		Update((float)(thisTime - lastTime));
		lastTime = thisTime;
     
		nbFrames++;

		if (thisTime - lastPrint >= 1.0){ // If last prinf() was more than 1 sec ago
			// printf and reset timer
			printf("%f frames/s \n", double(nbFrames));
			nbFrames = 0;
			lastPrint += 1.0;
		}   
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
		cloudName = argv[1];
		
			AppMain();
    } catch (const std::exception& e){
        std::cerr << "ERROR: " << e.what() << std::endl;
		getchar();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


//this converts to ply
//vector<string> clouds = GetFilesInDirectory("D:\\OpenGL3Repository\\source\\PCLShader\\resources\\partial");
//for (int i = 0; i < clouds.size(); i++){
//	pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGB>);
//	stringstream s;
//	s << clouds[i] << ".ply";
//	readKinectCloudDisk(cloud, clouds[i], s.str());
//}