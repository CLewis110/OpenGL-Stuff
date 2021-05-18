#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <GL/glew.h>					
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"

//assimp includes
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include "glm/gtc/type_ptr.hpp"

//*******************************Assignment 06 stuff**********************************//
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

//Global Variables
float rotAngle = 0;

glm::vec3 eye(0, 0, 1);
glm::vec3 lookAt(0, 0, 0);
double mousePosX, mousePosY;


glm::mat4 viewMat(1.0);
glm::mat4 projMat(1.0);
glm::mat3 normMat(1.0);



// Struct for holding vertex data
struct Vertex {
	glm::vec3 position;
	glm::vec4 color;
	glm::vec3 normal;
	glm::vec2 uv;
};

// Struct for holding mesh data
struct Mesh {
	vector<Vertex> vertices;
	vector<unsigned int> indices;
};

// Struct for holding OpenGL mesh
struct MeshGL {
	GLuint VBO = 0;
	GLuint EBO = 0;
	GLuint VAO = 0;
	int indexCnt = 0;
};

//Add PointLight struct
struct PointLight
{
	glm::vec4 pos;
	glm::vec3 color;
};




// Read from file and dump in string
string readFileToString(string filename) {
	// Open file
	ifstream file(filename);
	// Could we open file?
	if(!file || file.fail()) {
		cerr << "ERROR: Could not open file: " << filename << endl;
		throw exception(("ERROR: Could not open file: " + filename).c_str());
	}

	// Create output stream to receive file data
	ostringstream outS;
	outS << file.rdbuf();
	// Get actual string of file contents
	string allS = outS.str();
	// Close file
	file.close();
	// Return string
	return allS;
}

// Print out shader code
void printShaderCode(string &vertexCode, string &fragCode) {
	cout << "***********************" << endl;
	cout <<"** VERTEX SHADER CODE **" << endl;
	cout << "***********************" << endl;
	cout << vertexCode << endl;
	cout << "*************************" << endl;
	cout <<"** FRAGMENT SHADER CODE **" << endl;
	cout << "*************************" << endl;
	cout << fragCode << endl;
	cout << "*************************" << endl;
}

// GLFW error callback
static void error_callback(int error, const char* description) {
	cerr << "ERROR " << error << ": " << description << endl;
}

// GLFW setup
GLFWwindow* setupGLFW(int major, int minor, int windowWidth, int windowHeight, bool debugging) {
	
	// Set GLFW error callback
	glfwSetErrorCallback(error_callback);

	// (Try to) initialize GLFW
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	// Force specific OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	// Request debugging context?
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, debugging);
	
	// Create our GLFW window
	GLFWwindow* window = glfwCreateWindow(	windowWidth, windowHeight, 
											"BasicGraphics", 
											NULL, NULL);

	// Were we able to make it?
	if (!window) {
		// Kill GLFW and exit program
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// We want to draw to this window, so make the OpenGL context associated with this window our current context.
	glfwMakeContextCurrent(window);

	// Basically, turning VSync on (so we will wait until the screen is updated once before swapping the back and front buffers
	glfwSwapInterval(1);

	
	// Return window
	return window;
}

// Cleanup GLFW
void cleanupGLFW(GLFWwindow* window) {
	glfwDestroyWindow(window);
	glfwTerminate();
}

// GLEW setup
void setupGLEW(GLFWwindow* window) {
	
	// MAC-SPECIFIC: Some issues occur with using OpenGL core and GLEW; so, we'll use the experimental version of GLEW
	glewExperimental = true;

	// (Try to) initalize GLEW
	GLenum err = glewInit();

	if (GLEW_OK != err) {
		// We couldn't start GLEW, so we've got to go.
		// Kill GLFW and get out of here
		cout << "ERROR: GLEW could not start: " << glewGetErrorString(err) << endl;
		cleanupGLFW(window);
		exit(EXIT_FAILURE);
	}

	cout << "GLEW initialized; version ";
	cout << glewGetString(GLEW_VERSION) << endl;
}

// Check OpenGL version
void checkOpenGLVersion() {
	GLint glMajor, glMinor;
	glGetIntegerv(GL_MAJOR_VERSION, &glMajor);
	glGetIntegerv(GL_MINOR_VERSION, &glMinor);
	cout << "OpenGL context version: ";
	cout << glMajor << "." << glMinor << endl;
	cout << "Supported GLSL version is ";
	cout << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

// OpenGL debugging callback
// Taken from: https://learnopengl.com/In-Practice/Debugging
void APIENTRY openGLDebugCallback(	GLenum source, 
									GLenum type, 
									unsigned int id, 
									GLenum severity, 
									GLsizei length, 
									const char *message, 
									const void *userParam) {
    // ignore non-significant error/warning codes
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return; 

    cout << "---------------" << endl;
    cout << "Debug message (" << id << "): " <<  message << endl;

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             cout << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   cout << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: cout << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     cout << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     cout << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               cout << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: cout << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  cout << "Type: Undefined Behaviour"; break; 
        case GL_DEBUG_TYPE_PORTABILITY:         cout << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         cout << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              cout << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          cout << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           cout << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               cout << "Type: Other"; break;
    } std::cout << std::endl;
    
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         cout << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       cout << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          cout << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: cout << "Severity: notification"; break;
    } cout << endl;
    cout << endl;
}

// Check and setup debugging
void checkAndSetupOpenGLDebugging() {
	// If we have a debug context, we can connect a callback function for OpenGL errors...
	int flags; 
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if(flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		// Enable debug output
		glEnable(GL_DEBUG_OUTPUT);
		// Call debug output function when error occurs
    	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); 
		// Attach error callback
    	glDebugMessageCallback(openGLDebugCallback, nullptr);
		// Control output
		// * ALL messages...
    	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		// * Only high severity errors from the OpenGL API...
		// glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE); 
	}
}

// GLSL Compiling/Linking Error Check
// Returns GL_TRUE if compile was successful; GL_FALSE otherwise.
GLint checkGLSLError(GLuint ID, bool isCompile) {

	GLint resultGL = GL_FALSE;
	int infoLogLength;
	char *errorMessage = nullptr;

	if(isCompile) {
		// Get the compilation status and message length
		glGetShaderiv(ID, GL_COMPILE_STATUS, &resultGL);	
		glGetShaderiv(ID, GL_INFO_LOG_LENGTH, &infoLogLength);
	}
	else {
		// Get linking status and message length
		glGetProgramiv(ID, GL_LINK_STATUS, &resultGL);
		glGetProgramiv(ID, GL_INFO_LOG_LENGTH, &infoLogLength);
	}

	// Make sure length is at least one and allocate space for message	
	infoLogLength = (infoLogLength > 1) ? infoLogLength : 1;
	errorMessage = new char[infoLogLength];	

	// Get actual message
	if(isCompile)
		glGetShaderInfoLog(ID, infoLogLength, NULL, errorMessage);		
	else	
		glGetProgramInfoLog(ID, infoLogLength, NULL, errorMessage);

	// Print error message
	if(infoLogLength > 1)
		cout << errorMessage << endl;

	// Cleanup
	if(errorMessage) delete [] errorMessage;

	// Return OpenGL error
	return resultGL;
}

// Creates and compiles GLSL shader from code string; returns shader ID
GLuint createAndCompileShader(const char *shaderCode, GLenum shaderType) {
	// Create the shader ID
	GLuint shaderID = glCreateShader(shaderType);

	// Compile the vertex shader...
	cout << "Compiling shader..." << endl;
	glShaderSource(shaderID, 1, &shaderCode, NULL);
	glCompileShader(shaderID);

	// Checking result of compilation...
	GLint compileOK = checkGLSLError(shaderID, true);
	if (!compileOK || shaderID == 0) {
		glDeleteShader(shaderID);		
		cout << "Error compiling shader." << endl;
		throw runtime_error("Error compiling shader.");
	}

	// Return shader ID
	return shaderID;
}

// Given a list of compiled shaders, create and link a shader program (ID returned).
GLuint createAndLinkShaderProgram(std::vector<GLuint> allShaderIDs) {

	// Create program ID and attach shaders
	cout << "Linking program..." << endl;
	GLuint programID = glCreateProgram();
	for (GLuint &shaderID : allShaderIDs) {
		glAttachShader(programID, shaderID);
	}

	// Actually link the program
	glLinkProgram(programID);

	// Detach shaders (program already linked, successful or not)
	for (GLuint &shaderID : allShaderIDs) {
		glDetachShader(programID, shaderID);		
	}

	// Check linking
	GLint linkOK = checkGLSLError(programID, false);
	if (!linkOK || programID == 0) {		
		glDeleteProgram(programID);		
		cout << "Error linking shaders." << endl;
		throw runtime_error("Error linking shaders.");
	}

	// Return program ID
	return programID;
}

// Does the following:
// - Creates and compiles vertex and fragment shaders (from provided code strings)
// - Creates and links shader program
// - Deletes vertex and fragment shaders
GLuint initShaderProgramFromSource(string vertexShaderCode, string fragmentShaderCode) {
	GLuint vertID = 0;
	GLuint fragID = 0;
	GLuint programID = 0;

	try {
		// Create and compile shaders
		cout << "Vertex shader: ";
		vertID = createAndCompileShader(vertexShaderCode.c_str(), GL_VERTEX_SHADER);
		cout << "Fragment shader: ";
		fragID = createAndCompileShader(fragmentShaderCode.c_str(), GL_FRAGMENT_SHADER);

		// Create and link program
		programID = createAndLinkShaderProgram({ vertID, fragID });

		// Delete individual shaders
		glDeleteShader(vertID);
		glDeleteShader(fragID);

		// Success!
		cout << "Program successfully compiled and linked!" << endl;
	}
	catch (exception e) {
		// Cleanup shaders and shader program, just in case
		if (vertID) glDeleteShader(vertID);
		if (fragID) glDeleteShader(fragID);		
		// Rethrow exception
		throw e;
	}

	return programID;
}



//Random color number generator
float randomColorNumber()
{
	return (float)rand()/RAND_MAX;
}


//extract mesh information from aiMesh and store in Mesh struct
void extractMeshData(aiMesh* mesh, Mesh& m)
{
	//Clear out vertices and elements
	m.vertices.clear();
	m.indices.clear();

	glm::vec3 avePoint(0, 0, 0);

	//Loop through aiMesh
	for(unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		//Create vertex
		Vertex tempVert;

	
		//Grab positions from aiMesh and store in vertex's pos
		tempVert.position[0] = mesh->mVertices[i].x;
		tempVert.position[1] = mesh->mVertices[i].y;
		tempVert.position[2] = mesh->mVertices[i].z;


		//*******************************Assignment 06 stuff**********************************//
		//Add vert position to avePoint
		avePoint += tempVert.position;


		tempVert.normal.x = mesh->mNormals[i].x;
		tempVert.normal.y = mesh->mNormals[i].y;
		tempVert.normal.z = mesh->mNormals[i].z;




		//Set Color
		tempVert.color = glm::vec4(1, 1, 1, 1);

		//Add to vertex list
		m.vertices.push_back(tempVert);
	}



	//******************************* START Assignment 06 stuff**********************************//
	//Divide avePoint by total vertices
	avePoint = avePoint / glm::vec3((float)mesh->mNumVertices, (float)mesh->mNumVertices, (float)mesh->mNumVertices);

	//Loop through each Vertex in Mesh m
	for (unsigned int i = 0; i < m.vertices.size(); i++)
	{
		//Get position of vertex
		glm::vec3 p = m.vertices.at(i).position;

		//Subtract avePoint from p
		p = avePoint - p;

		//For U
		//Calculate atan2 of p.y and p.x and divide it by pi	
		m.vertices.at(i).uv.x = atan2(p.y, p.x) / glm::pi<float>();


		//For V		
		//Calculate length of vec2(p.y, p.x)
		float xyLen = glm::length(glm::vec2(p.y, p.x));

		//Get atan2 of p.z and length, divide by pi, add 0.5
		m.vertices.at(i).uv.y = ((atan2(p.z, xyLen) / glm::pi<float>()) + 0.5); 
			

	}
	//*******************************End Assignment 06 stuff**********************************//


	//Loop through all faces in ai Mesh (mNumFaces)
	for(unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		//Grab aiFace from mFaces
		aiFace *tempFace = &mesh->mFaces[i];

		//Loop through number of indices for this face (mNumIndicies)
		for (unsigned int j = 0; j < tempFace->mNumIndices; j++)
		{
			//Add index for the face (mIndicies) to vertex list of indices
			m.indices.push_back(tempFace->mIndices[j]);
		}
	}
}

// Create very simple mesh: a quad (4 vertices, 6 indices, 2 triangles)
void createSimpleQuad(Mesh &m) {

	// Clear out vertices and elements
	m.vertices.clear();
	m.indices.clear();

	// Create four corners
	Vertex upperLeft, upperRight;
	Vertex lowerLeft, lowerRight;
	Vertex middleLeft;

	// Set positions of vertices
	// Note: glm::vec3(x, y, z)
	upperLeft.position = glm::vec3(-0.5, 0.5, 0.0);
	upperRight.position = glm::vec3(0.5, 0.5, 0.0);	
	lowerLeft.position = glm::vec3(-0.5, -0.5, 0.0);
	lowerRight.position = glm::vec3(0.5, -0.5, 0.0);
	middleLeft.position = glm::vec3(-0.9, 0.0, 0.0);

	// Set vertex colors (red, green, blue, white)
	// Note: glm::vec4(red, green, blue, alpha)
	upperLeft.color = glm::vec4(1.0, 0.0, 0.0, 1.0);
	upperRight.color = glm::vec4(0.0, 1.0, 0.0, 1.0);
	lowerLeft.color = glm::vec4(0.0, 0.0, 1.0, 1.0);
	lowerRight.color = glm::vec4(1.0, 1.0, 1.0, 1.0);
	middleLeft.color = glm::vec4(1.0, 1.0, 0.0, 1.0);

	// Add to mesh's list of vertices
	m.vertices.push_back(upperLeft);
	m.vertices.push_back(upperRight);	
	m.vertices.push_back(lowerLeft);
	m.vertices.push_back(lowerRight);
	m.vertices.push_back(middleLeft);
	
	// Add indices for two triangles
	m.indices.push_back(0);
	m.indices.push_back(3);
	m.indices.push_back(1);

	m.indices.push_back(0);
	m.indices.push_back(2);
	m.indices.push_back(3);

	m.indices.push_back(0);
	m.indices.push_back(4);
	m.indices.push_back(2);
}

// Create OpenGL mesh (VAO) from mesh data
void createMeshGL(Mesh &m, MeshGL &mgl) {
	// Create Vertex Buffer Object (VBO)
	glGenBuffers(1, &(mgl.VBO));
	glBindBuffer(GL_ARRAY_BUFFER, mgl.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*m.vertices.size(), m.vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	// Create Vertex Array Object (VAO)
	glGenVertexArrays(1, &(mgl.VAO));

	// Enable VAO
	glBindVertexArray(mgl.VAO);

	// Enable the first two vertex attribute arrays
	glEnableVertexAttribArray(0);	// position
	glEnableVertexAttribArray(1);	// color
	glEnableVertexAttribArray(2);	// normal



	//*******************************Assignment 06 stuff**********************************//
	glEnableVertexAttribArray(3);	// texture coords



	// Bind the VBO and set up data mappings so that VAO knows how to read it
	// 0 = pos (3 elements)
	// 1 = color (4 elements)
	glBindBuffer(GL_ARRAY_BUFFER, mgl.VBO);	

	// Attribute, # of components, type, normalized?, stride, array buffer offset
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));



	//*******************************Assignment 06 stuff**********************************//
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));




	
	// Create Element Buffer Object (EBO)
	glGenBuffers(1, &(mgl.EBO));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mgl.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		m.indices.size() * sizeof(GLuint),
		m.indices.data(),
		GL_STATIC_DRAW);

	// Set index count
	mgl.indexCnt = (int)m.indices.size();

	// Unbind vertex array for now
	glBindVertexArray(0);
}

// Draw OpenGL mesh
void drawMesh(MeshGL &mgl) {
	glBindVertexArray(mgl.VAO);
	glDrawElements(GL_TRIANGLES, mgl.indexCnt, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);		
}

// Cleanup OpenGL mesh
void cleanupMesh(MeshGL &mgl) {

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &(mgl.VBO));
	mgl.VBO = 0;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &(mgl.EBO));
	mgl.EBO = 0;

	glBindVertexArray(0);
	glDeleteVertexArrays(1, &(mgl.VAO));
	mgl.VAO = 0;

	mgl.indexCnt = 0;
}


//Utitlity functions
void aiMatToGLM4(aiMatrix4x4& a, glm::mat4& m) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			m[j][i] = a[i][j];
		}
	}
}

void printTab(int cnt) {
	for (int i = 0; i < cnt; i++) {
		cout << "\t";
	}
}

void printNodeInfo(aiNode* node, glm::mat4& nodeT, glm::mat4& parentMat, glm::mat4& currentMat, int level)
{
	printTab(level);
	cout << "NAME: " << node->mName.C_Str() << endl;
	printTab(level);
	cout << "NUM MESHES: " << node->mNumMeshes << endl;
	printTab(level);
	cout << "NUM CHILDREN: " << node->mNumChildren << endl;
	printTab(level);
	cout << "Parent Model Matrix:" << glm::to_string(parentMat) << endl;
	printTab(level);
	cout << "Node Transforms:" << glm::to_string(nodeT) << endl;
	printTab(level);
	cout << "Current Model Matrix:" << glm::to_string(currentMat) << endl;
	cout << endl;
}


//Function for generating a transformation to rotate around the Local Z axis
glm::mat4 makeRotateZ(glm::vec3 offset)
{
	//Generate transform matrices for composite transform

	//translate negative offset
	glm::mat4 T = glm::translate(-offset);

	//rotate rotAngle around Z axis & Remember to convert to Radians
	glm::mat4 R = glm::rotate(glm::radians(rotAngle), glm::vec3(0, 0, 1));

	//Translate by offset
	glm::mat4 E = glm::translate(offset);

	//Return the composite transformation
	return E * R * T;
}


//Function to render scene recursively
void renderScene(vector<MeshGL>& allMeshes, aiNode* node, glm::mat4 parentMat, GLint modelMatLoc, int level, GLint normMatLoc, glm::mat4 viewMat)
{
	//Get transformation for current node
	aiMatrix4x4 trans = node->mTransformation;

	glm::mat4 nodeT;

	//Convert the transform to a glm::mat4 nodeT
	aiMatToGLM4(trans, nodeT);

	//Compute current model matrix
	glm::mat4 modelMat = parentMat * nodeT;

	//Call makeRotateZ() on the last column of modelMat
	glm::mat4 R = makeRotateZ(glm::vec3(modelMat[3]));

	//Generate temp model matrix as...
	glm::mat4 tmpModel = R * modelMat;


	normMat = glm::transpose(glm::inverse(glm::mat3(viewMat * tmpModel))); 
	glUniformMatrix3fv(normMatLoc, 1, false, glm::value_ptr(normMat));



	//Use glUniformMatrix4fv() to pass in tmpModel as the model matrix
	glUniformMatrix4fv(modelMatLoc, 1, false, glm::value_ptr(tmpModel));

	//Debugging
	//printNodeInfo(node, nodeT,  parentMat, tmpModel, level);

	//For each mesh in the node
	for (int i = 0; i < node->mNumMeshes; i++)
	{
		//Get index of mesh
		int index = node->mMeshes[i];

		//Call drawMesh()
		drawMesh(allMeshes.at(index));
	}		
			
	//Call renderScene() on each child of the NODE
	for (int i = 0; i < node->mNumChildren; i++)
	{
		renderScene(allMeshes, node->mChildren[i], modelMat, modelMatLoc, level + 1, normMatLoc, viewMat);
	}

}

//Key callback function
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {

		//Close when ESC
		if (key == GLFW_KEY_ESCAPE) {
			glfwSetWindowShouldClose(window, true);
		}
		//Add to rotate when J
		else if (key == GLFW_KEY_J)
		{
			//Add 1.0 to rotAngle
			rotAngle += 1;
		}
		//Subtract from rotate when K
		else if (key == GLFW_KEY_K)
		{
			//Sub 1.0 from rotAngle
			rotAngle -= 1;
		}
		
	//ADD KEYS
		//Move forward
		else if (key == GLFW_KEY_W)
		{

			glm::vec3 camDir = lookAt - eye;
			camDir *= 0.1;
			eye += camDir;
			lookAt += camDir;
		}


		//Move backward
		else if (key == GLFW_KEY_S)
		{

			glm::vec3 camDir = lookAt - eye;
			camDir *= 0.1;
			eye -= camDir;
			lookAt -= camDir;
		}

		//Move right in LOCAL X (positive)
		else if (key == GLFW_KEY_D)
		{

			glm::vec3 negCamDir = -(lookAt - eye);
			glm::vec3 cProd = cross(glm::vec3(0, 1, 0), negCamDir);
			cProd *= 0.1;
			eye += cProd;
			lookAt += cProd;
		}

		//Move left in LOCAL X (negative)
		else if (key == GLFW_KEY_A)
		{

			glm::vec3 negCamDir = -(lookAt - eye);
			glm::vec3 cProd = cross(glm::vec3(0, 1, 0), negCamDir);
			cProd *= 0.1;
			eye -= cProd;
			lookAt -= cProd;
		}

	}
}

//Optional: create helper function that generates a local rotation matrix(like makeRotateZ)
glm::mat4 makeLocalRotation(glm::vec3 offset, float degrees, glm::vec3 axis)
{
	//Generate transform matrices for composite transform

	//translate negative offset
	glm::mat4 T = glm::translate(-offset);

	//rotate rotAngle around X or Y axis & Remember to convert to Radians
	glm::mat4 R = glm::rotate(glm::radians(degrees), axis);

	//Translate by offset
	glm::mat4 E = glm::translate(offset);

	//Return the composite transformation
	return E * R * T;

}

//Add mouse movement callback
static void mouse_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	int height, width;
	float fWidth, fHeight, rAngle = 30.0;

	glm::mat4 rotX;
	glm::mat4 rotY;


	//Get Frame buffer size
	glfwGetFramebufferSize(window, &width, &height);

	//Get relative mouse motion
	//Subtract mouse position from previous mouse position
	mousePosX = mousePosX - xpos;
	mousePosY = mousePosY - ypos;

	//Divide by current framebuffer width and height to get relative mouse pos

	//Relative X motion
	fWidth = mousePosX / (float)width;

	//Relative Y motion
	fHeight = mousePosY / (float)height;


	//Use relative mouse motion to rotate cam

	//RELATIVE X MOTION (rotation around Global Y axis)
	rotX = makeLocalRotation(eye, rAngle * fWidth, glm::vec3(0, 1, 0));

	//Convert lookAt to and from glm::vec4
	lookAt = glm::vec3(rotX * glm::vec4(lookAt, 1.0));



	//Camera direction
	glm::vec3 NegCamDir = -(lookAt - eye);

	//Cross Global Y and Negative cam direction
	glm::vec3 cProd = cross(glm::vec3(0, 1, 0), NegCamDir);

	//RELATIVE Y MOTION (rotation around Global X axis)
	rotY = makeLocalRotation(eye, rAngle * fHeight, cProd);

	//Convert lookAt to and from glm::vec4
	lookAt = glm::vec3(rotY * glm::vec4(lookAt, 1.0));



	//Store new current mouse pos in global var
	glfwGetCursorPos(window, &mousePosX, &mousePosY);
}


//MAIN
int main(int argc, char **argv) {
	

	int fwidth, fheight;
	float aspect = 1.0;


	// Are we in debugging mode?
	bool DEBUG_MODE = true;

	// GLFW setup
	GLFWwindow* window = setupGLFW(4, 3, 800, 800, DEBUG_MODE);

	// GLEW setup
	setupGLEW(window);

	// Check OpenGL version
	checkOpenGLVersion();

	//Set key callback
	glfwSetKeyCallback(window, key_callback);


	//Get initial position of the mouse
	double mx, my;
	glfwGetCursorPos(window, &mx, &my);


	//Call glfwCursorPosCallback() to set mouse cursor function
	glfwSetCursorPosCallback(window, mouse_position_callback);

	//Hide cursor
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	// Set up debugging (if requested)
	if(DEBUG_MODE) checkAndSetupOpenGLDebugging();

	// Set the background color to a shade of blue
	glClearColor(0.0f, 0.7f, 0.0f, 1.0f);	

	// Create and load shader
	GLuint programID = 0;
	try {		
		// Load vertex shader code and fragment shader code
		string vertexCode = readFileToString("../Basic.vs");
		string fragCode = readFileToString("../Basic.fs");

		// Print out shader code, just to check
		if(DEBUG_MODE) printShaderCode(vertexCode, fragCode);

		// Create shader program from code
		programID = initShaderProgramFromSource(vertexCode, fragCode);
	}
	catch (exception e) {		
		// Close program
		cleanupGLFW(window);
		exit(EXIT_FAILURE);
	}

	// Create simple quad
	Mesh m;
	createSimpleQuad(m);
	
	//Get model matrix location
	GLint modelMatLoc = glGetUniformLocation(programID, "modelMat");


	//Get view matrix location
	GLint viewMatLoc = glGetUniformLocation(programID, "viewMat");

	//Get projection matrix location
	GLint projMatLoc = glGetUniformLocation(programID, "projMat");

	//Check for argc
	if (argc <= 1)
	{
		cerr << "Error: No argc" << endl;
		exit(1);
	}

	Assimp::Importer importer;

	//Load model

	const aiScene* scene = importer.ReadFile(argv[1],
		aiProcess_Triangulate | aiProcess_FlipUVs |
		aiProcess_GenNormals | aiProcess_JoinIdenticalVertices);

	//Make sure model loaded correctly
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		cerr << "Error: " << importer.GetErrorString() << endl;
		exit(1);
	}
	
	//Create a vector of meshGLs
	vector<MeshGL> mgls;

	//Create MeshGL
	MeshGL mgl;

	//FOR EACH MESH IN SCENE
	for(unsigned int i = 0; i < scene->mNumMeshes; i++)
	{	
		//Extract mesh data from mesh
		extractMeshData(scene->mMeshes[i],m);

		//Create MeshGL out of mesh data
		createMeshGL(m, mgl);	

		//Add MeshGL to vector of MeshGLs
		mgls.push_back(mgl);
	}


	// Create OpenGL mesh (VAO) from data
	//MeshGL mgl;
	//createMeshGL(m, mgl);
	
	// Enable depth testing
	glEnable(GL_DEPTH_TEST);



	PointLight light;
	light.pos = glm::vec4(0.5, 0.5, 0.5, 1);
	light.color = glm::vec4(1, 1, 1, 1);

	GLint lightPosID = glGetUniformLocation(programID, "light.pos");
	GLint lightColorID = glGetUniformLocation(programID, "light.color");
	GLint normMatLoc = glGetUniformLocation(programID, "normMat");





	//*******************************START Assignment 06 stuff**********************************//

	int width, height, nrComponents;
	unsigned int textureID = 0;

	//Turn verticle flip on
	stbi_set_flip_vertically_on_load(true);

	//load hardcoded image data
	unsigned char* data = stbi_load("../test.jpg", &width, &height, &nrComponents, 0);

	//If data NOT null
	if (data != NULL)
	{
		//generate texture ID
		glGenTextures(1, &textureID);

		//check nrComp for format
		GLenum format;

		//if nrComp == 3
		if (nrComponents == 3)
		{
			//format is RGB
			format = GL_RGB;

			//call pixelStore
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		}

		//if nrComp == 4
		else if (nrComponents == 4)
		{
			//format is RGBA
			format = GL_RGBA;

		}

		//bind texture ID
		glBindTexture(GL_TEXTURE_2D, textureID);

		//allocate texture memory and copy in new data
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

		//glTexParameteri
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//Delete image data
		stbi_image_free(data); 
	}

	else
	{
		//print error
		cerr << "ERROR: Cannot Load Texture"  << endl;

		//terminate
		glfwTerminate();

		//exit with error code
		exit(1);
	}

	//Active first tex channel
	glActiveTexture(GL_TEXTURE0);

	//Bind texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	//Get uniform location 
	GLint uniformTextureID = glGetUniformLocation(programID, "diffuseTexture");

	//*******************************END Assignment 06 stuff**********************************//




	while (!glfwWindowShouldClose(window)) {

		// Set viewport size
		glfwGetFramebufferSize(window, &fwidth, &fheight);
		glViewport(0, 0, fwidth, fheight);

		// Clear the framebuffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use shader program
		glUseProgram(programID);

		//Create glm::mat4 for view matrix
		viewMat = glm::lookAt(eye, lookAt, glm::vec3(0, 1, 0));

		//Pass the view matrix to the shader 
		glUniformMatrix4fv(viewMatLoc, 1, false, glm::value_ptr(viewMat));

		//Calculate the aspect ratio as framebuffer width/height
		glfwGetFramebufferSize(window, &fwidth, &fheight);

		//If height or width = 0  aspect ratio = 1.0
		if (fwidth > 0 && fheight > 0)
		{
			aspect = ((float)fwidth / (float)fheight);
		}

		float near = 0.01, far = 50.0, fov = 90;;
		
		//Create a glm::mat4 for the projMat
		projMat = glm::perspective(glm::radians(fov), aspect, near, far);


		//Pass projMat to shader
		glUniformMatrix4fv(projMatLoc, 1, false, glm::value_ptr(projMat));

		glm::vec4 viewLight = viewMat * light.pos;
		glUniform4fv(lightPosID, 1, glm::value_ptr(viewLight));
		glUniform4fv(lightColorID, 1, glm::value_ptr(light.color));


		//*******************************Assignment 06 stuff**********************************//

		glUniform1i(uniformTextureID, 0);


		//Draw objects in MeshGL vector
		//for (unsigned int i = 0; i < mgls.size(); i++)
		//{
			//drawMesh(mgls[i]);
				
		//}

		//Render scene
		renderScene(mgls, scene->mRootNode, glm::mat4(1.0), modelMatLoc, 0, normMatLoc, viewMat);



		// Swap buffers and poll for window events		
		glfwSwapBuffers(window);
		glfwPollEvents();

		// Sleep for 15 ms
		this_thread::sleep_for(chrono::milliseconds(15));
	}

	//Cleaning loop
	for (unsigned int i = 0; i < mgls.size(); i++)
	{
		cleanupMesh(mgls[i]);
	}


	// Clean up shader programs
	glUseProgram(0);
	glDeleteProgram(programID);
		
	// Destroy window and stop GLFW
	cleanupGLFW(window);

	return 0;
}
