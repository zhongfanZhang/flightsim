#define GLFW_INCLUDE_NONE

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <time.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Viewer.h"
#include "Physics.h"
#include "shader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define PI 3.14159f

//shapes and materials vectors containing the object properties
//for the trees, plane and light poles
std::vector<tinyobj::shape_t> shapes;
std::vector<tinyobj::material_t> materials;

std::vector<tinyobj::shape_t> shapes1;
std::vector<tinyobj::material_t> materials1;

std::vector<tinyobj::shape_t> shapes2;
std::vector<tinyobj::material_t> materials2;

//ID for the currently used shader program
unsigned int programID, sceneID, hudID, skyID;

//window dimensions
int winX = 1920;
int winY = 1080;

InputState Input;
Viewer* Camera;
Physics Plane;

//initial camera position
glm::vec3 cameraPos(0.0f, 0.0f, 2.0f);

//contains maximum absolute vertex value for use in initial object scaling
float max_vertex;
float max_vertex1;
float max_vertex2;

//contains minimum and maximum vertex values in order:
//min x, min y, min z, max x, max y, max z
//this is used for printing
float min_max[6];

float scaling = 1.0;

//size of terrain mesh
int terrainX = 1000;
int terrainZ = 1000;

//handles for VAOs
unsigned int vaoHandleSquare;
unsigned int vaoHandleCube;
unsigned int vaoHandleTerrain[2];

//variables for keeping track of time
float lastFrame;
float deltaTime;
float timeOfDay = 0;

int loadHeightMap(float* heightMapArray)
{
    std::ifstream heightMap("heightmap/quantised_image_1k.ppm");

    //skipping over unnecessary lines
    std::string skip;
    getline(heightMap, skip);
    getline(heightMap, skip);
    heightMap >> skip >> skip >> skip;

    //reading in data
    float r, g, b;
    for (int i = 0; i < terrainX * terrainZ; i++){
        heightMap >> r >> g >> b;
        heightMapArray[i] = g;
    }
    heightMap.close();
}

int loadRGBTexture(const char *path)
{
    int x, y, n;

    // Load from file. Force RGB image pixel format
    unsigned char *data = stbi_load(path, &x, &y, &n, 3);

    // Copy to GPU as data for the currently active texture.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    return 0;
}

int loadSkyboxTexture()
{
    int x, y, n;

    std::string skyboxPics[6] = {"skybox/right.jpg",
                            "skybox/left.jpg",
                            "skybox/top.jpg",
                            "skybox/bottom.jpg",
                            "skybox/back.jpg",
                            "skybox/front.jpg"};

    for (int i = 0; i < 6; i++){

        // Load from file. Force RGB image pixel format
        unsigned char *data = stbi_load(skyboxPics[i].c_str(), &x, &y, &n, 3);

        // Copy to GPU as data for the currently active texture.
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    return 0;
}

int createTerrainTextures(int num_textures, GLuint* TexID)
{
    glGenTextures( 3, TexID );
    
    for (int i = 0; i < num_textures; i++){

	    glActiveTexture(GL_TEXTURE0);
        glBindTexture( GL_TEXTURE_2D, TexID[i] );
        loadRGBTexture( "terrain_textures/grass.jpg" );
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture( GL_TEXTURE_2D, TexID[i+1] );
        loadRGBTexture( "terrain_textures/rock.jpg" );

        glActiveTexture(GL_TEXTURE2);
        glBindTexture( GL_TEXTURE_2D, TexID[i+2] );
        loadRGBTexture( "terrain_textures/snow.jpg" );

    }
    
    return 0;
}

int createObjTextures(int num_textures, GLuint* TexID, std::string path)
{
    glGenTextures( num_textures, TexID );
    
    for (int i = 0; i < num_textures; i++){

	    glActiveTexture(GL_TEXTURE0);
        glBindTexture( GL_TEXTURE_2D, TexID[i] );
        loadRGBTexture( (path + materials[i].diffuse_texname).c_str() );

    }
    
    return 0;
}

GLuint createSkyboxTextures()
{

    GLuint TexID;
    glGenTextures( 1, &TexID );
    glBindTexture( GL_TEXTURE_CUBE_MAP, TexID);

    loadSkyboxTexture();

    return TexID;
}

void generateTerrain(float* terrainArray, unsigned int* indexBuffer, float* heightMapArray, int* treeArray){
    
    srand(time(NULL));
    int tree;    

    for (int i = 0; i < terrainZ; i++){
        for (int j = 0; j < terrainX; j++){

            //x coord
            terrainArray[i*terrainX*3 + j*3] = j;
    
            //y coord retrieved from height map
            terrainArray[i*terrainX*3 + j*3 + 1] = heightMapArray[i*terrainX + j]*2;

            //z coord
            terrainArray[i*terrainX*3 + j*3 + 2] = i;
    
            //tree positions are randomly generated and placed into treeArray
            tree = rand() % 255;
            //trees are not placed on runway at height 48 and 49
            if (tree == 0 && heightMapArray[i*terrainX + j] != 48 && heightMapArray[i*terrainX + j] != 49){
                treeArray[2*(i*terrainX + j)] = rand()%2;
                //random sizes are set between 1 and 5
                treeArray[2*(i*terrainX + j)+1] = rand()%5 + 1;
            }        
        
        }
    }

    //indices are created for terrain mesh using triangle strips with degenerate triangles
    for (int i = 0; i < terrainZ - 1; i++){
        for (int j = 0; j < terrainX * 2; j++){
            if (i == 0){
                indexBuffer[i*(terrainX*2) + j] = j/2 + (j%2)*terrainZ + i*terrainZ;
            }else{
                indexBuffer[i*(terrainX*2+2) + j] = j/2 + (j%2)*terrainZ + i*terrainZ;
            }
            //degenerate triangles
            if (j == terrainX*2-1 && i!= terrainZ-2){
                indexBuffer[i*(terrainX*2+2) + j + 1] = j/2 + (j%2)*terrainZ + i*terrainZ;
                indexBuffer[i*(terrainX*2+2) + j + 2] = 0/2 + (0%2)*terrainZ + (i+1)*terrainZ;   
            }
        }
    }

    //normalising terrain size and location in x and z to -500 to 500
    for (int i = 0; i < terrainZ * terrainX * 3; i+=3){
        terrainArray[i] = ((terrainArray[i] / (terrainX - 1)) - 0.5)*1000;
        terrainArray[i+1] = terrainArray[i+1];
        terrainArray[i+2] = ((terrainArray[i+2] / (terrainZ - 1)) - 0.5)*1000;
    }

}

void computeTerrainNormals(float* terrainArray, float* faceNormals, float* vertexNormals)
{

    glm::vec3 v1, v2, v3, u, v, normal;
    int temp;

    //face normals are calculated using cross products of lines in the faces
    for (int i = 0; i < terrainZ - 1; i++){
        for (int j = 0; j < (terrainX - 1) * 2; j++){
            temp = j/2;
            if (j%2 == 0){

                v1 = glm::vec3(terrainArray[i*terrainX*3 + 3*temp],
                terrainArray[i*terrainX*3 + 3*temp+1],
                terrainArray[i*terrainX*3 + 3*temp+2]);

                v3 = glm::vec3(terrainArray[i*terrainX*3 + 3*(temp+1)],
                terrainArray[i*terrainX*3 + 3*(temp+1)+1],
                terrainArray[i*terrainX*3 + 3*(temp+1)+2]);

                v2 = glm::vec3(terrainArray[i*terrainX*3 + 3*(temp+terrainX)],
                terrainArray[i*terrainX*3 + 3*(temp+terrainX)+1],
                terrainArray[i*terrainX*3 + 3*(temp+terrainX)+2]);
    
                u = v2 - v1;
                v = v3 - v1;

            }else{

                v1 = glm::vec3(terrainArray[i*terrainX*3 + 3*(temp+1)],
                terrainArray[i*terrainX*3 + 3*(temp+1)+1],
                terrainArray[i*terrainX*3 + 3*(temp+1)+2]);

                v2 = glm::vec3(terrainArray[i*terrainX*3 + 3*(temp+terrainX)],
                terrainArray[i*terrainX*3 + 3*(temp+terrainX)+1],
                terrainArray[i*terrainX*3 + 3*(temp+terrainX)+2]);

                v3 = glm::vec3(terrainArray[i*terrainX*3 + 3*(temp+terrainX+1)],
                terrainArray[i*terrainX*3 + 3*(temp+terrainX+1)+1],
                terrainArray[i*terrainX*3 + 3*(temp+terrainX+1)+2]);

                u = v3 - v1;
                v = v3 - v2;

            }

            normal = normalize(glm::cross(u, v));
            faceNormals[i*(terrainX - 1)*2*3 + j*3 + 0] = normal.x;
            faceNormals[i*(terrainX - 1)*2*3 + j*3 + 1] = normal.y;
            faceNormals[i*(terrainX - 1)*2*3 + j*3 + 2] = normal.z;
        }
    }

    float sum[3];
    glm::vec3 temp1;

    //vertex normals are calculated from the surrounding face normals
    for (int i = 0; i < terrainZ; i++){
        for (int j = 0; j < terrainX; j++){

            sum[0] = 0.0;
            sum[1] = 0.0;
            sum[2] = 0.0;
    
            //middle vertices (surrounded by 6 triangles)
            if (i != 0 && i != terrainZ - 1 && j != 0 && j != terrainX - 1){

                //average 6 faces around vertex

                //bottom left
                sum[0] += faceNormals[(i-1)*(terrainX-1)*2*3 + 3*(1 + 2*(j-1)) + 0];
                sum[1] += faceNormals[(i-1)*(terrainX-1)*2*3 + 3*(1 + 2*(j-1)) + 1];
                sum[2] += faceNormals[(i-1)*(terrainX-1)*2*3 + 3*(1 + 2*(j-1)) + 2];

                //bottom middle
                sum[0] += faceNormals[(i-1)*(terrainX-1)*2*3 + 3*(2 + 2*(j-1)) + 0];
                sum[1] += faceNormals[(i-1)*(terrainX-1)*2*3 + 3*(2 + 2*(j-1)) + 1];
                sum[2] += faceNormals[(i-1)*(terrainX-1)*2*3 + 3*(2 + 2*(j-1)) + 2];

                //bottom right
                sum[0] += faceNormals[(i-1)*(terrainX-1)*2*3 + 3*(3 + 2*(j-1)) + 0];
                sum[1] += faceNormals[(i-1)*(terrainX-1)*2*3 + 3*(3 + 2*(j-1)) + 1];
                sum[2] += faceNormals[(i-1)*(terrainX-1)*2*3 + 3*(3 + 2*(j-1)) + 2];
    
                //top left
                sum[0] += faceNormals[i*(terrainX-1)*2*3 + 3*(0 + 2*(j-1)) + 0];
                sum[1] += faceNormals[i*(terrainX-1)*2*3 + 3*(0 + 2*(j-1)) + 1];
                sum[2] += faceNormals[i*(terrainX-1)*2*3 + 3*(0 + 2*(j-1)) + 2];

                //top left
                sum[0] += faceNormals[i*(terrainX-1)*2*3 + 3*(1 + 2*(j-1)) + 0];
                sum[1] += faceNormals[i*(terrainX-1)*2*3 + 3*(1 + 2*(j-1)) + 1];
                sum[2] += faceNormals[i*(terrainX-1)*2*3 + 3*(1 + 2*(j-1)) + 2];
    
                //top left
                sum[0] += faceNormals[i*(terrainX-1)*2*3 + 3*(2 + 2*(j-1)) + 0];
                sum[1] += faceNormals[i*(terrainX-1)*2*3 + 3*(2 + 2*(j-1)) + 1];
                sum[2] += faceNormals[i*(terrainX-1)*2*3 + 3*(2 + 2*(j-1)) + 2];

                temp1 = normalize(glm::vec3(sum[0], sum[1], sum[2]));    
    
                vertexNormals[i*terrainX*3 + j*3] = temp1.x;
                vertexNormals[i*terrainX*3 + j*3 + 1] = temp1.y;
                vertexNormals[i*terrainX*3 + j*3 + 2] = temp1.z;
                
            }else{

                //non middle triangles are not calculated as they are not visible on the edges of the map
                vertexNormals[i*terrainX*3 + j*3] = 0.0;
                vertexNormals[i*terrainX*3 + j*3 + 1] = 1.0;
                vertexNormals[i*terrainX*3 + j*3 + 2] = 0.0;

            }

        }
    }
}

int createTerrainVAO(float* terrainArray, unsigned int* indexBuffer, float* vertexNormals){
    
    glGenVertexArrays(2, vaoHandleTerrain);

    unsigned int buffer[4];
    glGenBuffers(4, buffer);

    //creating texture coordinates for the terrain mesh
    float* texCoords;
    texCoords = new float[terrainX * terrainZ * 2];
    for (int i = 0; i < terrainZ; i++){
        for (int j = 0; j < terrainX; j++){
            texCoords[i*terrainZ*2 + j*2 + 0] = terrainArray[i*terrainZ*3 + j*3 + 0]/50;
            texCoords[i*terrainZ*2 + j*2 + 1] = terrainArray[i*terrainZ*3 + j*3 + 2]/50;
        }
    }

    glBindVertexArray(vaoHandleTerrain[0]);

    //Set vertex attribute
    glBindBuffer(GL_ARRAY_BUFFER, buffer[0]);
    glBufferData(GL_ARRAY_BUFFER, terrainX*terrainZ*3*4, terrainArray, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    //Set normal attribute
    glBindBuffer(GL_ARRAY_BUFFER, buffer[1]);
    glBufferData(GL_ARRAY_BUFFER, terrainX*terrainZ*3*4, vertexNormals, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    //Set texture attribute
    glBindBuffer(GL_ARRAY_BUFFER, buffer[2]);
    glBufferData(GL_ARRAY_BUFFER, terrainX*terrainZ*2*4, texCoords, GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

    // Set indice attribute
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ((terrainZ - 1) * (terrainX * 2 + 2) - 2)*4, indexBuffer, GL_STATIC_DRAW);

    //Unbinding
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    delete[] texCoords;
    return 0;
}

int createSquareVAO(){

    float squareVertices[12] = {
        -0.1, -0.1, 0.0,
        0.1, -0.1, 0.0,
        0.1, 0.1, 0.0,
        -0.1, 0.1, 0.0
    };

    unsigned int squareIndices[6] = {
        0, 1, 2, 2, 3, 0
    };
    
    glGenVertexArrays(1, &vaoHandleSquare);
    glBindVertexArray(vaoHandleSquare);

    unsigned int buffer[2];
    glGenBuffers(2, buffer);

    //Set vertex attribute
    glBindBuffer(GL_ARRAY_BUFFER, buffer[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

     // Set indice attribute
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(squareIndices), squareIndices, GL_STATIC_DRAW);

    //Unbinding
    glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

    return 0;
}

//cube used for the skybox
void createCubeVAO()
{
   
    float cubeVertices[108] = {         
        -10000.0f,  10000.0f, -10000.0f,
        -10000.0f, -10000.0f, -10000.0f,
         10000.0f, -10000.0f, -10000.0f,
         10000.0f, -10000.0f, -10000.0f,
         10000.0f,  10000.0f, -10000.0f,
        -10000.0f,  10000.0f, -10000.0f,

        -10000.0f, -10000.0f,  10000.0f,
        -10000.0f, -10000.0f, -10000.0f,
        -10000.0f,  10000.0f, -10000.0f,
        -10000.0f,  10000.0f, -10000.0f,
        -10000.0f,  10000.0f,  10000.0f,
        -10000.0f, -10000.0f,  10000.0f,

         10000.0f, -10000.0f, -10000.0f,
         10000.0f, -10000.0f,  10000.0f,
         10000.0f,  10000.0f,  10000.0f,
         10000.0f,  10000.0f,  10000.0f,
         10000.0f,  10000.0f, -10000.0f,
         10000.0f, -10000.0f, -10000.0f,

        -10000.0f, -10000.0f,  10000.0f,
        -10000.0f,  10000.0f,  10000.0f,
         10000.0f,  10000.0f,  10000.0f,
         10000.0f,  10000.0f,  10000.0f,
         10000.0f, -10000.0f,  10000.0f,
        -10000.0f, -10000.0f,  10000.0f,

        -10000.0f,  10000.0f, -10000.0f,
         10000.0f,  10000.0f, -10000.0f,
         10000.0f,  10000.0f,  10000.0f,
         10000.0f,  10000.0f,  10000.0f,
        -10000.0f,  10000.0f,  10000.0f,
        -10000.0f,  10000.0f, -10000.0f,

        -10000.0f, -10000.0f, -10000.0f,
        -10000.0f, -10000.0f,  10000.0f,
         10000.0f, -10000.0f, -10000.0f,
         10000.0f, -10000.0f, -10000.0f,
        -10000.0f, -10000.0f,  10000.0f,
         10000.0f, -10000.0f,  10000.0f
    };

	glGenVertexArrays(1, &vaoHandleCube);
	glBindVertexArray(vaoHandleCube);
  
	// Buffers to store position, colour and index data
	unsigned int buffer;
	glGenBuffers(1, &buffer);

	// Set vertex position
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);  
	
    // Un-bind
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
    
}
 
//Used to detect key presses to interact with program
//Key input is processed in InputState.h
void key_callback(GLFWwindow* window,
                  int key, int scancode, int action, int mods)
{
    
    bool currentPressedState = false;
    if (action == GLFW_PRESS){
        currentPressedState = true;
    }
    
    if (action == GLFW_PRESS || action == GLFW_RELEASE){

        switch(key){
            case GLFW_KEY_Q:
                Input.qPressed = currentPressedState;
                break;
            case GLFW_KEY_E:
                Input.ePressed = currentPressedState;
                break;
            case GLFW_KEY_W:
                Input.wPressed = currentPressedState;
                break;
            case GLFW_KEY_S:
                Input.sPressed = currentPressedState;
                break;
            case GLFW_KEY_A:
                Input.aPressed = currentPressedState;
                break;
            case GLFW_KEY_D:
                Input.dPressed = currentPressedState;
                break;
            case GLFW_KEY_SPACE:
                Input.spacePressed = currentPressedState;
                break;
            case GLFW_KEY_LEFT_ALT:
                Input.altPressed = currentPressedState;
                break;
            case GLFW_KEY_F:
                Input.fPressed = currentPressedState;
                break;
            case GLFW_KEY_B:
                Input.bPressed = currentPressedState;
                break;
            case GLFW_KEY_ESCAPE: // escape key pressed
                glfwSetWindowShouldClose(window, GL_TRUE);
                break;
            default:
                break;
        }
    }
            
}

void mouse_pos_callback(GLFWwindow* window, double x, double y)
{
    Input.update((float)x, (float)y);
}    

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        Input.rMousePressed = true;
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
        Input.rMousePressed = false;
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        Input.lMousePressed = true;
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        Input.lMousePressed = false;
    }                
}

//Sets the projection matrix for the viewer
//and passes it to the shaders as a uniform variable
void setProjection()
{
    glm::mat4 projection;   

    programID = sceneID;
    glUseProgram(programID);
    projection = glm::perspective( (float)M_PI/3.0f, (float) winX / winY, 1.0f, 20000.0f );
	int projHandleScene = glGetUniformLocation(programID, "projection");
	glUniformMatrix4fv( projHandleScene, 1, false, glm::value_ptr(projection) );

    programID = skyID;
    glUseProgram(programID);
	int projHandleSky = glGetUniformLocation(programID, "projection");
	glUniformMatrix4fv( projHandleSky, 1, false, glm::value_ptr(projection) );


    programID = hudID;
    glUseProgram(programID);
    float aspect = (float) winX / winY;
    projection = glm::ortho( -aspect * 2.0, aspect * 2.0, -2.0, 2.0 );
    int projHandleHud = glGetUniformLocation(programID, "projection");
    glUniformMatrix4fv( projHandleHud, 1, false, glm::value_ptr(projection) );  
}

void reshape_callback( GLFWwindow *window, int x, int y )
{
    winX = x;
    winY = y;
    setProjection();
    glViewport(0, 0, x, y);
}

//Creates the VAOs for the objects and sends them to the GPU
//Includes vertices, colours, texture coordinates and indices
int createObjVAO(unsigned int* vaoHandle, int numShapes)
{

    glGenVertexArrays(numShapes, vaoHandle);

    unsigned int buffer[numShapes*4];
    glGenBuffers(numShapes*4, buffer);

    // Loop which creates all VAOs
    for (int i = 0; i < numShapes; i++){
	    glBindVertexArray(vaoHandle[i]);

        // Set vertex attribute
	    glBindBuffer(GL_ARRAY_BUFFER, buffer[i*4]);
	    glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.positions.size() * sizeof(float), &shapes[i].mesh.positions[0], GL_STATIC_DRAW);
	    glEnableVertexAttribArray(0);
	    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        // Set normal attribute
        glBindBuffer(GL_ARRAY_BUFFER, buffer[i*4+1]);
        glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.normals.size() * sizeof(float), &shapes[i].mesh.normals[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        // Set texture attribute
        glBindBuffer(GL_ARRAY_BUFFER, buffer[i*4+2]);
	    glBufferData(GL_ARRAY_BUFFER, shapes[i].mesh.texcoords.size() * sizeof(float), &shapes[i].mesh.texcoords[0], GL_STATIC_DRAW);
	    glEnableVertexAttribArray(2);
	    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

        // Set indice attribute
	    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer[i*4+3]);
	    glBufferData(GL_ELEMENT_ARRAY_BUFFER, shapes[i].mesh.indices.size() * sizeof(int), &shapes[i].mesh.indices[0], GL_STATIC_DRAW);

        // Unbinding
        glBindBuffer(GL_ARRAY_BUFFER, 0);
	    glBindVertexArray(0);
    }

    return 0;
}

//The main render function which draws the object
//Several arrays are passed to the function as these need to be changed every loop
void render(unsigned int* vaoHandleObj, unsigned int* vaoHandleObj1, unsigned int* vaoHandleObj2, int numShapes, int numShapes1, int numShapes2, float* terrainArray, GLuint* terrainTexID, GLuint* objTexID, GLuint skyboxTexID, int* treeArray)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //handles are created for the scene and skybox shaders
    int viewHandleScene = glGetUniformLocation(sceneID, "view");
    int modelHandleScene = glGetUniformLocation(sceneID, "model");
    int viewHandleSky = glGetUniformLocation(skyID, "view");
    int modelHandleSky = glGetUniformLocation(skyID, "model");

    ////////////////////////////////////////////////////////
    // 3d scene is drawn (scene shader)

    programID = sceneID;
    glUseProgram(programID);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 
    glEnable(GL_DEPTH_TEST);
    
    //camera is updated using current plane parameters from physics class
    Camera->update( Input, Plane.getPosition(), Plane.getYaw(), Plane.getPitch(), Plane.getRoll());
    glm::mat4 viewMatrix;
    viewMatrix = Camera->getViewMtx();

	glUniformMatrix4fv( viewHandleScene, 1, false, glm::value_ptr(viewMatrix) );

    glm::mat4 modelMatrix, translateMatrix, rotateMatrix, scaleMatrix;  
    scaleMatrix = glm::scale(scaleMatrix, glm::vec3(1.0/max_vertex1, 1.0/max_vertex1, 1.0/max_vertex1));
    
    //current plane position in terrain grid is found for collision detection
    int xcoord, zcoord, arrayPos;
    float minX = 1000000.0;
    float minZ = 1000000.0;
    float tempx, tempz;
    for (int i = 0; i < terrainX * terrainZ * 3; i+=3){
        tempx = fabsf(Plane.getPosition().x - terrainArray[i]);
        if (tempx < minX){
            minX = tempx;
            xcoord = i;
        }
        tempz = fabsf(Plane.getPosition().z - terrainArray[i+2]);
        if (tempz < minZ){
            minZ = tempz;
            zcoord = i;
        }
    }
    xcoord = xcoord % (terrainZ*3);
    arrayPos = zcoord + xcoord;

    //collision detection is set for the terrain and the runway
    if (Plane.getPosition().y < terrainArray[arrayPos + 1]){
        if(terrainArray[arrayPos + 1] != 96){
            Plane.setCollision();
        }else{
            Plane.setLanding();
        }
    }

    //collision with the ceiling is detected
    if (Plane.getPosition().y > 400){
        Plane.setCeiling();
    }

    //collision with the boundaries of the map is detected
    if (Plane.getPosition().x < -480 || Plane.getPosition().x > 480 || Plane.getPosition().z < -480 || Plane.getPosition().z > 480){
        Plane.setCollision();   
    }

    //plane is moved through the map using current plane parameters from Physics class
    translateMatrix = glm::translate(translateMatrix, Plane.getPosition());
    rotateMatrix = glm::rotate(rotateMatrix, Plane.getYaw(), glm::vec3(0.0, -1.0, 0.0));
    rotateMatrix = glm::rotate(rotateMatrix, Plane.getPitch(), glm::vec3(-1.0, 0.0, 0.0));
    rotateMatrix = glm::rotate(rotateMatrix, Plane.getRoll(), glm::vec3(0.0, 0.0, 1.0));
    modelMatrix = translateMatrix * rotateMatrix * scaleMatrix;
    glUniformMatrix4fv(modelHandleScene, 1, false, glm::value_ptr(modelMatrix));   

    //texture handles
    int tex0 = glGetUniformLocation(programID, "tex0");
    int tex1 = glGetUniformLocation(programID, "tex1");
    int tex2 = glGetUniformLocation(programID, "tex2");
    int current = glGetUniformLocation(programID, "currentObject");
    glUniform1i(tex0, 0);
    glUniform1i(tex1, 1);
    glUniform1i(tex2, 2);
    glUniform1i(current, 0);

    //Drawing plane from loaded obj data
    for (int i = 0; i < numShapes1; i++){
        glBindTexture(GL_TEXTURE_2D, objTexID[i]);
        glBindVertexArray(vaoHandleObj1[i]);
        glDrawElements(GL_TRIANGLES, shapes1[i].mesh.indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    //reversing plane movements so that world stays in place
    translateMatrix = glm::translate(translateMatrix, -Plane.getPosition());
    scaleMatrix = glm::scale(scaleMatrix, glm::vec3(max_vertex1, max_vertex1, max_vertex1));
    rotateMatrix = glm::rotate(rotateMatrix, Plane.getRoll(), glm::vec3(0.0, 0.0, -1.0));
    rotateMatrix = glm::rotate(rotateMatrix, Plane.getPitch(), glm::vec3(1.0, 0.0, 0.0));
    rotateMatrix = glm::rotate(rotateMatrix, Plane.getYaw(), glm::vec3(0.0, 1.0, 0.0));
    modelMatrix = translateMatrix * rotateMatrix * scaleMatrix;
    glUniformMatrix4fv(modelHandleScene, 1, false, glm::value_ptr(modelMatrix));

    //Drawing terrain mesh
    glUniform1i(current, 1);
    glBindTexture(GL_TEXTURE_2D, terrainTexID[0]);
    glBindVertexArray(vaoHandleTerrain[0]);
    glDrawElements(GL_TRIANGLE_STRIP, (terrainZ - 1) * (terrainX * 2 + 2) - 2, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    //Drawing trees
    //Each position in the terrain mesh is checked to see if a tree was procedurally generated there
    //If so it is placed using translations and scaled according to its procedurally generated scale factor
    float scaleFactor;
    for (int i = 0; i < terrainZ; i++){
        for (int j = 0; j < terrainX; j++){
            if (treeArray[2*(i*terrainX + j)] == 1){

                scaleFactor = float(treeArray[2*(i*terrainX+j)+1]);
                translateMatrix = glm::translate(translateMatrix, glm::vec3(j-500, terrainArray[3*(i*terrainX+j)+1], i-500));
                scaleMatrix = glm::scale(scaleMatrix, glm::vec3(scaleFactor, scaleFactor, scaleFactor));
                modelMatrix = translateMatrix * rotateMatrix * scaleMatrix;
                glUniformMatrix4fv(modelHandleScene, 1, false, glm::value_ptr(modelMatrix));
                translateMatrix = glm::translate(translateMatrix, glm::vec3(-j+500, -terrainArray[3*(i*terrainX+j)+1], -i+500));
                scaleMatrix = glm::scale(scaleMatrix, glm::vec3(1.0/scaleFactor, 1.0/scaleFactor, 1.0/scaleFactor));              

                //trees are drawn
                for (int k = 0; k < numShapes; k++){
                    glBindTexture(GL_TEXTURE_2D, objTexID[k]);
                    glBindVertexArray(vaoHandleObj[k]);
                    glDrawElements(GL_TRIANGLES, shapes[k].mesh.indices.size(), GL_UNSIGNED_INT, 0);
                    glBindVertexArray(0);

                }
            }
        }
    }

    //Drawing light poles
    //Eight light poles are drawn along the runway using translations to place each
    //pole in the correct spot
    glUniform1i(current, 2);

    translateMatrix = glm::translate(translateMatrix, glm::vec3(-7.0, 98.0, 230.0));
    modelMatrix = translateMatrix * rotateMatrix * scaleMatrix;
    glUniformMatrix4fv(modelHandleScene, 1, false, glm::value_ptr(modelMatrix));
    for (int i = 0; i < numShapes2; i++){
        glBindVertexArray(vaoHandleObj2[i]);
        glDrawElements(GL_TRIANGLES, shapes2[i].mesh.indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    translateMatrix = glm::translate(translateMatrix, glm::vec3(7.0, 0.0, -40.0));
    modelMatrix = translateMatrix * rotateMatrix * scaleMatrix;
    glUniformMatrix4fv(modelHandleScene, 1, false, glm::value_ptr(modelMatrix));
    for (int i = 0; i < numShapes2; i++){
        glBindVertexArray(vaoHandleObj2[i]);
        glDrawElements(GL_TRIANGLES, shapes2[i].mesh.indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    translateMatrix = glm::translate(translateMatrix, glm::vec3(0.0, 0.0, -40.0));
    modelMatrix = translateMatrix * rotateMatrix * scaleMatrix;
    glUniformMatrix4fv(modelHandleScene, 1, false, glm::value_ptr(modelMatrix));
    for (int i = 0; i < numShapes2; i++){
        glBindVertexArray(vaoHandleObj2[i]);
        glDrawElements(GL_TRIANGLES, shapes2[i].mesh.indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    translateMatrix = glm::translate(translateMatrix, glm::vec3(-3.0, 0.0, -40.0));
    modelMatrix = translateMatrix * rotateMatrix * scaleMatrix;
    glUniformMatrix4fv(modelHandleScene, 1, false, glm::value_ptr(modelMatrix));
    for (int i = 0; i < numShapes2; i++){
        glBindVertexArray(vaoHandleObj2[i]);
        glDrawElements(GL_TRIANGLES, shapes2[i].mesh.indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    translateMatrix = glm::translate(translateMatrix, glm::vec3(27.0, 0.0, 0.0));
    modelMatrix = translateMatrix * rotateMatrix * scaleMatrix;
    glUniformMatrix4fv(modelHandleScene, 1, false, glm::value_ptr(modelMatrix));
    for (int i = 0; i < numShapes2; i++){
        glBindVertexArray(vaoHandleObj2[i]);
        glDrawElements(GL_TRIANGLES, shapes2[i].mesh.indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    translateMatrix = glm::translate(translateMatrix, glm::vec3(-2.0, 0.0, 40.0));
    modelMatrix = translateMatrix * rotateMatrix * scaleMatrix;
    glUniformMatrix4fv(modelHandleScene, 1, false, glm::value_ptr(modelMatrix));
    for (int i = 0; i < numShapes2; i++){
        glBindVertexArray(vaoHandleObj2[i]);
        glDrawElements(GL_TRIANGLES, shapes2[i].mesh.indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    translateMatrix = glm::translate(translateMatrix, glm::vec3(3.0, 0.0, 40.0));
    modelMatrix = translateMatrix * rotateMatrix * scaleMatrix;
    glUniformMatrix4fv(modelHandleScene, 1, false, glm::value_ptr(modelMatrix));
    for (int i = 0; i < numShapes2; i++){
        glBindVertexArray(vaoHandleObj2[i]);
        glDrawElements(GL_TRIANGLES, shapes2[i].mesh.indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    translateMatrix = glm::translate(translateMatrix, glm::vec3(0.0, 0.0, 40.0));
    modelMatrix = translateMatrix * rotateMatrix * scaleMatrix;
    glUniformMatrix4fv(modelHandleScene, 1, false, glm::value_ptr(modelMatrix));
    for (int i = 0; i < numShapes2; i++){
        glBindVertexArray(vaoHandleObj2[i]);
        glDrawElements(GL_TRIANGLES, shapes2[i].mesh.indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    int timeHandleScene = glGetUniformLocation(programID, "time");
    glUniform1f(timeHandleScene, timeOfDay);

    //////////////////////////////////////////////////////////////////
    // Skybox is drawn (sky shader)
    programID = skyID;
    glUseProgram(programID);
    glUniformMatrix4fv(viewHandleSky, 1, false, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(modelHandleSky, 1, false, glm::value_ptr(modelMatrix));
    
    //cube for skybox is drawn
    glDepthMask(GL_FALSE);
    glBindVertexArray(vaoHandleCube);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);
    glBindVertexArray(0);
    
    int timeHandleSky = glGetUniformLocation(programID, "time");
    glUniform1f(timeHandleSky, timeOfDay);

    //////////////////////////////////////////////////////////////////
    // HUD is drawn (hud shader)
    programID = hudID;
    glUseProgram(programID);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_DEPTH_TEST);

    int HUDHandle = glGetUniformLocation(programID, "display");
    glUniform1i(HUDHandle, 0);

    //speed bar is drawn on the bottom right of the screen
    int HUDTransformHandle = glGetUniformLocation(programID, "HUDTranslate");
    glm::mat4 powerTranslate;
    float aspect = (float) winX / winY;
    powerTranslate = glm::translate(powerTranslate, glm::vec3(aspect * 1.9, -1.8, 0.0));
    glUniformMatrix4fv( HUDTransformHandle, 1, false, glm::value_ptr(powerTranslate));

    //squares are stacked according to the plane's current speed
    for (int i = 0; i < 100; i++){
        if (Plane.getPower() >= (float)i / 100){
            powerTranslate = glm::translate(powerTranslate, glm::vec3(0.0, 0.02, 0.0));
            glUniformMatrix4fv( HUDTransformHandle, 1, false, glm::value_ptr(powerTranslate));
            glBindVertexArray(vaoHandleSquare);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }

    glUniform1i(HUDHandle, 1);

    //speed bar is drawn on the bottom left of the screen
    glm::mat4 altitudeTranslate;
    altitudeTranslate = glm::translate(altitudeTranslate, glm::vec3(-aspect * 1.9, -1.8, 0.0));
    glUniformMatrix4fv( HUDTransformHandle, 1, false, glm::value_ptr(altitudeTranslate));

    //squares are stacked according the plane's current altitude
    for (int i = 0; i < 100; i++){
        if (Plane.getPosition().y >= (float)i * 4.0){
            altitudeTranslate = glm::translate(altitudeTranslate, glm::vec3(0.0, 0.02, 0.0));
            glUniformMatrix4fv( HUDTransformHandle, 1, false, glm::value_ptr(altitudeTranslate));
            glBindVertexArray(vaoHandleSquare);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }

    glFlush();
}

//Function for calculating the minimum and maximum
//vertex values in each dimension, as well as the largest
//absolute value overall
float min_max_vertices()
{
    //minimum values are initialised high
    min_max[0] = 1000000.0;
    min_max[1] = 1000000.0;
    min_max[2] = 1000000.0;
    //maximum values are initialised low
    min_max[3] = -1000000.0;
    min_max[4] = -1000000.0;
    min_max[5] = -1000000.0;
    
    //loops over every shape to find all vertices and compare them
    //to the current minimums and maximums in each dimension
    for (int i = 0; i < shapes.size(); i++){
        for (int j = 0; j < shapes[i].mesh.positions.size() / 3; j++){
            //min values
            if (shapes[i].mesh.positions[3*j] < min_max[0]){
                min_max[0] = shapes[i].mesh.positions[3*j];
            }
            if (shapes[i].mesh.positions[3*j+1] < min_max[1]){
                min_max[1] = shapes[i].mesh.positions[3*j+1];
            }
            if (shapes[i].mesh.positions[3*j+2] < min_max[2]){
                min_max[2] = shapes[i].mesh.positions[3*j+2];
            }
            //max values
            if (shapes[i].mesh.positions[3*j] > min_max[3]){
                min_max[3] = shapes[i].mesh.positions[3*j];
            }
            if (shapes[i].mesh.positions[3*j+1] > min_max[4]){
                min_max[4] = shapes[i].mesh.positions[3*j+1];
            }
            if (shapes[i].mesh.positions[3*j+2] > min_max[5]){
                min_max[5] = shapes[i].mesh.positions[3*j+2];
            }
        }
    }

    //result is converted to absolute values
    float min_max_abs[6];
    for (int i = 0; i < 6; i++){
        if (min_max[i] < 0.0){
            min_max_abs[i] = -min_max[i];
        }else{
            min_max_abs[i] = min_max[i];
        }
    } 

    //maximum overall vertex is found
    float max_vertexx = -1.0;
    for (int i = 0; i < 6; i++){
        if (min_max_abs[i] > max_vertexx){
            max_vertexx = min_max_abs[i];
        }
    } 
    return max_vertexx;      
} 

int main(int argc, char *argv[])
{
    std::string err;
    bool ret;

    ret = tinyobj::LoadObj(shapes, materials, err, "tree/PineTree03.obj", "tree/");

    if (!err.empty()) { // `err` may contain warning message.
      std::cerr << err << std::endl;
    }

    if (!ret) {
      exit(1);
    } 

    GLFWwindow* window;

    if (!glfwInit()) {
        exit(1);
    }

    // Specify that we want OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create the window and OpenGL context
    window = glfwCreateWindow(winX, winY, "Flight simulator", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(1);
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
	
	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		exit(1);
	}
   
    // load all three shaders and assign IDs to them
    sceneID = LoadShaders("scene.vert", "scene.frag");
    hudID = LoadShaders("hud.vert", "hud.frag");
    skyID = LoadShaders("sky.vert", "sky.frag");

    // setting background colour to grey
    glClearColor(0.5f, 0.5f, 0.5f, 0.5f);
    
    int numShapes = shapes.size();

    setProjection();
    Camera =  new Viewer(cameraPos);
    
    unsigned int vaoHandleObj[numShapes];

    //calculating and printing the min/max vertex data
    max_vertex = min_max_vertices();
   
    //creating VAOs
	createObjVAO(vaoHandleObj, numShapes);
    createSquareVAO();
    createCubeVAO();

    //loading height map data into array
    float* heightMapArray;
    heightMapArray = new float[terrainX * terrainZ];
    loadHeightMap(heightMapArray);

    //creating terrain data arrays and filling them using functions
    float* terrainArray;
    float* faceNormals;
    float* vertexNormals;
    unsigned int* indexBuffer;
    int* treeArray;
    treeArray = new int[terrainX * terrainZ * 2];
    terrainArray = new float[terrainX * terrainZ * 3];
    faceNormals = new float[(terrainX - 1) * (terrainZ - 1) * 2 * 3];
    vertexNormals = new float[(terrainX * terrainZ * 3)];
    indexBuffer = new unsigned int[(terrainZ - 1) * (terrainX * 2 + 2) - 2];
    generateTerrain(terrainArray, indexBuffer, heightMapArray, treeArray);
    computeTerrainNormals(terrainArray, faceNormals, vertexNormals);
    createTerrainVAO(terrainArray, indexBuffer, vertexNormals);

    //deleting dynamic arrays
    delete[] faceNormals;
    delete[] vertexNormals;
    delete[] indexBuffer;
    delete[] heightMapArray;

    //creating textures
    GLuint terrainTexID[3];
    GLuint objTexID[numShapes];
    createTerrainTextures(1, terrainTexID);
    createObjTextures(numShapes, objTexID, "tree/");
    GLuint skyboxTexID = createSkyboxTextures();

    //loading obj files for plane and light poles
    shapes.resize(0);
    materials.resize(0);
    ret = tinyobj::LoadObj(shapes, materials, err, "plane/asdf3.obj", "plane/");
    int numShapes1 = shapes.size();
    max_vertex1 = min_max_vertices();
    unsigned int vaoHandleObj1[numShapes1];
    createObjVAO(vaoHandleObj1, numShapes1);
    shapes.resize(0);
    materials.resize(0);
    ret = tinyobj::LoadObj(shapes, materials, err, "pole/3d-model.obj", "pole/");
    int numShapes2 = shapes.size();
    max_vertex2 = min_max_vertices();
    unsigned int vaoHandleObj2[numShapes2];
    createObjVAO(vaoHandleObj2, numShapes2);
    shapes.resize(0);
    materials.resize(0);
    ret = tinyobj::LoadObj(shapes, materials, err, "tree/PineTree03.obj", "tree/");
    ret = tinyobj::LoadObj(shapes1, materials1, err, "plane/asdf3.obj", "plane/");
    ret = tinyobj::LoadObj(shapes2, materials2, err, "pole/3d-model.obj", "pole/");
    
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_pos_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetFramebufferSizeCallback(window, reshape_callback);

    float currentFrame;
    lastFrame = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        //time between frames is calculated to make plane move at constant rate
        //with different framerates
        currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        //timeOfDay is incremented for use in the day/night cycle
        timeOfDay += deltaTime/4;
		if(timeOfDay >= 30 ){
            timeOfDay = 0;
        }

        //Plane parameters are updated
        Plane.updateYaw(deltaTime, Input);
        Plane.updatePitch(deltaTime, Input);
        Plane.updateRoll(deltaTime, Input);
        Plane.updatePower(deltaTime, Input);
        Plane.updateSpeed(deltaTime);
        Plane.updatePosition(deltaTime);    
    
        //everything is rendered
        render(vaoHandleObj, vaoHandleObj1, vaoHandleObj2, numShapes, numShapes1, numShapes2, terrainArray, terrainTexID, objTexID, skyboxTexID, treeArray);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(0);

	return 0;    

}
