//Some Windows Headers (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "maths_funcs.h"
#include <time.h> // for random selections
#ifdef WIN32
#endif
#include <windows.h>
#include <conio.h>

// Assimp includes
#pragma comment(lib, "assimp.lib") 
#include <assimp/cimport.h> // C importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.

//for adding text
#include "text.h"
//for loading in images for texturing
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
//for adding music
#include <irrKlang.h>
#pragma comment(lib, "irrKlang.lib") // link with irrKlang.dll
#include <ik_ISoundEngine.h>
using namespace irrklang;
ISoundEngine *SoundEngine = createIrrKlangDevice();

/*----------------------------------------------------------------------------
				MESH TO LOAD AND TEXTURE TO LOAD
----------------------------------------------------------------------------*/
#define GROUND_MESH "../Models/ground.dae"			//the floor mesh
#define OBSTACLE_MESH "../Models/pillar.dae"		//the pillar mesh
#define DIAMOND_MESH "../Models/green_diamond.dae"	//the diamond mesh
#define BOMB_MESH "../Models/bomb.dae"				//the bomb mesh
#define CHARACTER_MESH "../Models/character.dae"	//the character mesh
#define GROUND_TEXTURE  "../floor.gif"				//the floor texture
#define OBSTACLE_TEXTURE "../scifi.jpg"				//the pillar texture
#define CHARACTER_TEXTURE "../green.jpg"			//the character texture
#define DIAMOND_TEXTURE "../diamond.jpg"			//the diamond texture
/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/
#define NUM_BOMBS 3
#define NUM_OBSTACLES 25

float thresh = 0.5;	//this should be the distance from the centre of an object at which we have collided with it  ####NEEDS TO BE DIFFERENT######
std::vector<float> g_vp, g_vn, g_vt;
//counters for object vertices
int g_point_count = 0;
int ground_vertex_count = 0, obstacle_vertex_count = 0;
int diamond_vertex_count = 0, bomb_vertex_count = 0, character_vertex_count = 0;
//object and texture ids
GLuint GROUND_ID = 0,  DIAMOND_ID = 2, BOMB_ID = 3, OBSTACLE_ID = 4, CHARACTER_ID = 5;
GLuint GROUND_TEXTURE_ID = 0, DIAMOND_TEXTURE_ID = 2, BOMB_TEXTURE_ID = 3, OBSTACLE_TEXTURE_ID = 4, CHARACTER_TEXTURE_ID = 5;


// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
using namespace std;
GLuint shaderProgramID;

unsigned int mesh_vao = 0;
int width = 1000;
int height = 1000;

/*
CAMERA GLOBALS
*/
//transformations
GLuint loc1, loc2, loc3;
GLfloat rotate_camera_x = 1.5f, rotate_camera_y = 0.5f;
GLfloat move_camera_x = 0.0f, move_camera_y = 0.0f, move_camera_z = 0.0f; //start off a distance back
GLfloat speed = 0.01f; //speed of movement
GLfloat camera_dist = 2.0f;
//for mouse 
int prev_x, prev_y; //to store the most recent mouse coords
bool mouse = true;
bool upkey = false, downkey = false, leftkey = false, rightkey = false;// to tell us whether we should be looking for collisions, moving etc
bool i_key = false, k_key = false, j_key = false, l_key = false;

/*
GAMEPLAY VARIABLES
*/
#define HOMESCREEN 0
#define PLAYING 1
#define YOU_DIED 2
#define REPLAY_AND_SCORES 3
bool gameover = false;
int state = PLAYING;//the gamestate
bool dead = false;//start off playing the game
float high_scores[10] = { 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f };//no high-score to start
float highest = 0.0f;//the highest score achieved
int curr_score = 0.0;//keep track of current score
GLfloat character_position_x = 0.0f, character_position_y =0.0f, character_position_z = -4.0f;
float bomb_position_x[NUM_BOMBS] = { 10.0, 0.0, 30.5 };
float bomb_position_z[NUM_BOMBS] = {  10.0, 0.0, 60.0 };
GLfloat obstacle_pos_x[] = { -9.0f, -4.5f, 0.0f, 4.5f, 9.0f, -4.5f, -4.5f, 0.0f, 4.5f, 9.0f, 
							-9.0f, -4.5f, 4.5f, 9.0f, 9.0f, -4.5f, 0.0f,4.5f, 9.0f, -9.0f, 
							-4.5f, 0.0f, 4.5f, 9.0f,  };
GLfloat obstacle_pos_z[] = { -9.0f, -9.0f, -9.0f, -9.0f, -9.0f, -4.5f, -4.5f, -4.5f, -4.5f, 
							-4.5f, 0.0f, 0.0f, 0.0f, 0.0f, 4.5f, 4.5f, 4.5f, 4.5f, 4.5f, 
							9.0f, 9.0f, 9.0f, 9.0f, 9.0f };
GLfloat diamond_pos[] = { 0.0f, 0.0f };
GLfloat character_translation = 0.0f;
GLfloat diamond_rotation = 0.0f;//the rotation of the diamonds
GLfloat bomb_rotation = 0.0f; //the rotation of the bombs
GLfloat possibilities[] = { -7.5f, 6.0f, 8.5f,-2.5f, 2.5f, 7.5f, -3.65f, -1.39f, 8.2f, -4.0f };//possible positions for the diamonds
GLfloat diamond_count=0.0f;
GLfloat bomb_count = 0.0f;
GLfloat game_counter = 0.0f;//#### WILL NEED TO UPDATE THE BELOW ####

GLfloat jump_size = 0.0f;
bool jump = false;

//for text
int game_over_text = 0;
int score_text = 1;


#pragma region MESH LOADING
/*----------------------------------------------------------------------------
MESH LOADING FUNCTION
----------------------------------------------------------------------------*/
bool load_mesh(const char* file_name) {
	const aiScene* scene = aiImportFile(file_name, aiProcess_Triangulate); // TRIANGLES!
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return false;
	}
	printf("  %i animations\n", scene->mNumAnimations);
	printf("  %i cameras\n", scene->mNumCameras);
	printf("  %i lights\n", scene->mNumLights);
	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);

	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);

		g_vp.clear();
		g_vn.clear();
		g_vt.clear();

		//get the num of vertices for each mesh
		if (file_name == GROUND_MESH)
			ground_vertex_count = mesh->mNumVertices;
		else if (file_name == OBSTACLE_MESH)
			obstacle_vertex_count = mesh->mNumVertices;
		else if (file_name == DIAMOND_MESH)
			diamond_vertex_count = mesh->mNumVertices;
		else if (file_name == BOMB_MESH)
			bomb_vertex_count = mesh->mNumVertices;
		else if (file_name == CHARACTER_MESH)
			character_vertex_count = mesh->mNumVertices;
		else
			g_point_count = mesh->mNumVertices;


		for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
			if (mesh->HasPositions()) {
				const aiVector3D* vp = &(mesh->mVertices[v_i]);
				//printf ("      vp %i (%f,%f,%f)\n", v_i, vp->x, vp->y, vp->z);
				g_vp.push_back(vp->x);
				g_vp.push_back(vp->y);
				g_vp.push_back(vp->z);
			}
			if (mesh->HasNormals()) {
				const aiVector3D* vn = &(mesh->mNormals[v_i]);
				//printf ("      vn %i (%f,%f,%f)\n", v_i, vn->x, vn->y, vn->z);
				g_vn.push_back(vn->x);
				g_vn.push_back(vn->y);
				g_vn.push_back(vn->z);
			}
			if (mesh->HasTextureCoords(0)) {
				const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
				//printf ("      vt %i (%f,%f)\n", v_i, vt->x, vt->y);
				g_vt.push_back(vt->x);
				g_vt.push_back(vt->y);
			}
			if (mesh->HasTangentsAndBitangents()) {
				// NB: could store/print tangents here
			}
		}
	}

	aiReleaseImport(scene);
	return true;
}


#pragma endregion MESH LOADING

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
#pragma warning(disable : 4996) 
// Create a NULL-terminated string by reading the provided file
char* readShaderSource(const char* shaderFile) {
	FILE* fp = fopen(shaderFile, "rb"); //!->Why does binary flag "RB" work and not "R"... wierd msvc thing?

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);
	buf[size] = '\0';

	fclose(fp);

	return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}
	const char* pShaderSource = readShaderSource(pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		//exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

//adds a texture using the stb_image library ( (c) Dr Anton Gerdelan)
void addTexture(GLuint& texture_id, char* file_name) {
	int width, height, n;
	unsigned char* image = stbi_load(file_name, &width, &height, &n, STBI_rgb);

	glGenTextures(1, &texture_id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

	//wrap around texture repeated
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, 0);
}


GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, "../Shaders/simpleVertexShader.txt", GL_VERTEX_SHADER);
	AddShader(shaderProgramID, "../Shaders/simpleFragmentShader.txt", GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { 0 };
	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS


//give the vao, filename and the num of  vertices, generate the mesh for the object
void generateObjectBufferMesh(GLuint &vao, const char* f_name, int &count) {
	/*----------------------------------------------------------------------------
	LOAD MESH HERE AND COPY INTO BUFFERS
	----------------------------------------------------------------------------*/

	//Note: you may get an error "vector subscript out of range" if you are using this code for a mesh that doesnt have positions and normals
	//Might be an idea to do a check for that before generating and binding the buffer.

	load_mesh(f_name);
	unsigned int vp_vbo = 0;
	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
	loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

	glGenBuffers(1, &vp_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glBufferData(GL_ARRAY_BUFFER, count * 3 * sizeof(float), &g_vp[0], GL_STATIC_DRAW);
	unsigned int vn_vbo = 0;
	glGenBuffers(1, &vn_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glBufferData(GL_ARRAY_BUFFER, count * 3 * sizeof(float), &g_vn[0], GL_STATIC_DRAW);

	//	This is for texture coordinates which you don't currently need, so I have commented it out
	unsigned int vt_vbo = 0;
	glGenBuffers(1, &vt_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
	glBufferData(GL_ARRAY_BUFFER, count * 2 * sizeof(float), &g_vt[0], GL_STATIC_DRAW);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	//	This is for texture coordinates which you don't currently need, so I have commented it out
	glEnableVertexAttribArray(loc3);
	glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
	glVertexAttribPointer(loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindVertexArray(0);
}


#pragma endregion VBO_FUNCTIONS


void display() {

	//check if we are currently playing or not
	if (dead==true){//(state == YOU_DIED ) {
		//no gameplay
		glClearColor(0.0f, 0.1f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//### FOR LATER: PRINT GAME OVER
	}
	/*else if (state == REPLAY_AND_SCORES) {
		
	}*/
	else{// if(state == PLAYING){ //not dead - game on!

		// tell GL to only draw onto a pixel if the shape is closer to the viewer
		glEnable(GL_DEPTH_TEST);							// enable depth-testing
		glDepthFunc(GL_LESS);								// depth-testing interprets a smaller value as "closer"
		glClearColor(0.5f, 0.1f, 0.5f, 1.0f);				// pink backdrop
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shaderProgramID);
		//Declare your uniform variables that will be used in your shader
		int matrix_location = glGetUniformLocation(shaderProgramID, "model");
		int view_mat_location = glGetUniformLocation(shaderProgramID, "view");
		int proj_mat_location = glGetUniformLocation(shaderProgramID, "proj");
		int texture_location = glGetUniformLocation(shaderProgramID, "tex");


		//Draw the ground - the root object
		mat4 persp_proj = perspective(90.0, (float)width / (float)height, 0.1, 1000.0);
		mat4 floor_local = identity_mat4();
		floor_local = scale(floor_local, vec3(4.75, 4.75,4.75));
		mat4 floor_global = floor_local;
		//camera - look at character
		mat4 cam_view = look_at(vec3(move_camera_x, 5.0f * character_position_y + 3.0f + move_camera_y, move_camera_z), 
							vec3(5.0f * character_position_x, 5.0f * character_position_y + 3.0f, 5.0f * character_position_z), 
							vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
		glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, cam_view.m);
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, floor_global.m);
		//add texture to ground
		glBindTexture(GL_TEXTURE_2D, GROUND_TEXTURE_ID);
		glUniform1i(texture_location, 0);
		glBindVertexArray(GROUND_ID);
		glDrawArrays(GL_TRIANGLES, 0, ground_vertex_count);


		//pillars
		glBindTexture(GL_TEXTURE_2D, OBSTACLE_TEXTURE_ID);
		glUniform1i(texture_location, 0);
		glBindVertexArray(OBSTACLE_ID);
		mat4 obstacle_local[NUM_OBSTACLES];
		mat4 obstacle_global[NUM_OBSTACLES];
		for (int i = 0; i < NUM_OBSTACLES; i++)
		{
			obstacle_local[i] = identity_mat4();
			obstacle_local[i] = scale(obstacle_local[i], vec3(0.08, 0.25, 0.2));
			obstacle_local[i] = rotate_x_deg(obstacle_local[i], -90.0);
			obstacle_local[i] = translate(obstacle_local[i], vec3(obstacle_pos_x[i], 0.0f, obstacle_pos_z[i]-2.0));
			obstacle_global[i] = floor_global * obstacle_local[i];
			glUniformMatrix4fv(matrix_location, 1, GL_FALSE, obstacle_global[i].m);
			glDrawArrays(GL_TRIANGLES, 0, obstacle_vertex_count);
		}

		//bombs
		glBindVertexArray(BOMB_ID);
		mat4 bomb_local[NUM_BOMBS], bomb_global[NUM_BOMBS];
		for (int j = 0; j < NUM_BOMBS; j++) {
			bomb_local[j] = identity_mat4();
			bomb_local[j] = rotate_x_deg(bomb_local[j], -90.0);
			bomb_local[j] = rotate_y_deg(bomb_local[j], bomb_rotation);
			bomb_local[j] = translate(bomb_local[j], vec3(bomb_position_x[j], 1.0, bomb_position_z[j]));
			bomb_local[j] = scale(bomb_local[j], vec3(0.01, 0.01, 0.01f));
			bomb_global[j] = floor_global * bomb_local[j];
			glUniformMatrix4fv(matrix_location, 1, GL_FALSE, bomb_global[j].m);
			glDrawArrays(GL_TRIANGLES, 0, bomb_vertex_count);
		}

		//character
		mat4 character_local_bottom = identity_mat4();
		character_local_bottom = scale(character_local_bottom, vec3(0.01f, 0.01f, 0.01f));
		character_local_bottom = rotate_y_deg(character_local_bottom, rotate_camera_y);
		character_local_bottom = translate(character_local_bottom, vec3(character_position_x, 
					character_position_y + 0.35f + character_translation, character_position_z));
		mat4 character_global = floor_global * character_local_bottom;
		// update uniforms & draw
		glBindTexture(GL_TEXTURE_2D, CHARACTER_TEXTURE_ID);
		glUniform1i(texture_location, 0);
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, character_global.m);
		glBindVertexArray(CHARACTER_ID);
		glDrawArrays(GL_TRIANGLES, 0, character_vertex_count);
		//top
		mat4 character_local_top = identity_mat4();
		character_local_top = rotate_y_deg(character_local_top, rotate_camera_y);
		character_local_top = rotate_y_deg(rotate_z_deg(character_local_top, diamond_rotation), diamond_rotation);
		character_local_top = translate(character_local_top, vec3(0.0f,20.0f, 0.0f));
		mat4 character_global1 = character_global * character_local_top;
		// update uniforms & draw
		glBindTexture(GL_TEXTURE_2D, CHARACTER_TEXTURE_ID);
		glUniform1i(texture_location, 0);
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, character_global1.m);
		glBindVertexArray(CHARACTER_ID);
		glDrawArrays(GL_TRIANGLES, 0, character_vertex_count);

		//diamonds
		glBindTexture(GL_TEXTURE_2D, DIAMOND_TEXTURE_ID);
		glUniform1i(texture_location, 0);
		glBindVertexArray(DIAMOND_ID);
		mat4 diamond_local = identity_mat4();
		diamond_local = scale(diamond_local, vec3(0.015f, 0.015f, 0.015f));
		diamond_local = rotate_x_deg(diamond_local, -90.0);
		diamond_local = rotate_y_deg(diamond_local, diamond_rotation);
		diamond_local = translate(diamond_local, vec3(diamond_pos[0], 1.0f, diamond_pos[1]));
		mat4 diamond_global = floor_global * diamond_local;
		// update uniforms & draw
		glUniformMatrix4fv(matrix_location, 1, GL_FALSE, diamond_global.m);
		glDrawArrays(GL_TRIANGLES, 0, diamond_vertex_count);

	}//end of dead=false loop
	/*else {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		char* text = "KABOOM";
		add_text(text, 30.0, 20.0, 20.0, 1.0, 0.0, 0.0, 1.0);
	}*/
	glutSwapBuffers();
}

//generates random diamonds 
void diamond_logic() {
	diamond_pos[0] = possibilities[rand() % 10];
	diamond_pos[1] = possibilities[rand() % 10];
}

//translates the bombs around the scene
void bomb_logic() {
	for (int i = 0; i < NUM_BOMBS; i++) {
			if (bomb_position_x[i] < 80.0)
				bomb_position_x[i] += 0.01;
			else if (bomb_position_x[i] >= 80.0)
				bomb_position_x[i] -= 0.01;
	}
}


//reset the game state to start over
void reset_game() {

	dead = false; //we are playing again
	curr_score = 0.0;//reset current score to 0
	//reset camera position
	rotate_camera_x = 0.0f, rotate_camera_y = 0.0f;
	move_camera_x = 0.0f, move_camera_z = -4.0f;
	//reset the character starting point
	character_position_x = 0.0f;
	character_position_y = 0.0f;
	character_position_z = 0.0f;
	//reset other variables
	diamond_vertex_count = 0;
	game_counter = 0.0f;

}

//update the scoreboard
void updateScoreBoard() {
	if (curr_score>high_scores[9]) {
		//we have a new score in the top-10
		for (int i = 0; i<10; i++) {
			if (curr_score <= high_scores[i]) {
				high_scores[i++] = high_scores[i];
				high_scores[i] = curr_score;
				break;
			}
			//keep searching
			high_scores[i++] = high_scores[i];
		}
	}

	if (curr_score>highest) {
		//new high score
		highest = curr_score;
	}
	string score_string = "";
	score_string.append(to_string((int)curr_score));
	update_text(game_over_text, score_string.c_str());
}


//checks to see if you've hit a bomb - if you have, you die
void check_bomb_collisions() {
	for (int i = 0; i<NUM_BOMBS; i++) {
		float x_pos = character_position_x - bomb_position_x[i];
		float z_pos = character_position_z - bomb_position_z[i];
		float dist = sqrt((x_pos*x_pos) + (z_pos*z_pos));
		if (abs(dist) <0.9) {
			//game over - we hit a bomb!!!
			dead = true;
			gameover = true;
			SoundEngine->play2D("../audio/explosion.mp3", false);
			cout << "you died" << endl;
			updateScoreBoard();
			//state = YOU_DIED;
		}
	}
	//else we didn't collide with a bomb
}

//checks to see if we have hit an pillar: called in the character_move() function
bool check_obstacle_collision(float _x, float _z) {
	float x, z; //only care about x and z directions i.e. horizontal planes - will need to review this to allow jumping over bombs
	for (int i = 0; i<NUM_OBSTACLES; i++) {
		x = _x - obstacle_pos_x[i];
		z = _z - obstacle_pos_z[i];
		if (sqrt((x*x) + (z*z))<thresh) {	//check if this #### 1.2f #### works
			return true;
		}
	}
	if (sqrt((_x*_x) + (_z*_z)) < 0.2f || ((sqrt((_x*_x) + (_z*_z)) >0.4f) && (sqrt((_x*_x) + (_z*_z)) < 0.8f) && (_x > 0.36f || _x < -0.36f)))
		return true;
	return false;
}


void calculate_diamond_position() {
	//generates random diamonds 
	diamond_pos[0] = possibilities[rand() % 10];
	diamond_pos[1] = possibilities[rand() % 10];
}


//checks to see if we have hit a diamond
bool check_diamond_collision(float _x, float _z) {
	//this is where we increment our score
	float x = _x - diamond_pos[0];
	float z = _z - diamond_pos[1];
	if (sqrt((x*x) + (z*z))<=1.0f) {
		curr_score++;	 //we scored!!
		SoundEngine->play2D("../audio/beep.mp3", false);
		calculate_diamond_position(); //should move the diamond somewhere else
		return true;
	}

	if (sqrt((_x*_x) + (_z*_z)) < thresh || ((sqrt((_x*_x) + (_z*_z)) > thresh*2) && (sqrt((_x*_x) + (_z*_z)) < thresh*3) && (_x > 0.36f || _x < -0.36f)))
		return true;
	return false;
}

//determines how the character should move depending on objects in the scene
void character_move() {
	if (upkey==true){
		//move forward as long as we aren't colliding with a diamond or a pillar
		if (!check_diamond_collision(character_position_x, character_position_z - speed * cos(rotate_camera_x)) && 
							!check_obstacle_collision(character_position_x, character_position_z - speed * cos(rotate_camera_x)))
			character_position_z -= speed * cos(rotate_camera_x);
		if (!check_diamond_collision(character_position_x - speed * sin(rotate_camera_x), character_position_z) && 
							!check_obstacle_collision(character_position_x - speed * sin(rotate_camera_x), character_position_z))
			character_position_x -= speed * sin(rotate_camera_x);
	}
	if (downkey==true){
		//move backwards as long as we aren't colliding with a diamond or a pillar
		if (!check_diamond_collision(character_position_x, character_position_z + speed * cos(rotate_camera_x)) && 
							!check_diamond_collision(character_position_x, character_position_z + speed * cos(rotate_camera_x)))
			character_position_z += speed * cos(rotate_camera_x);
		if (!check_diamond_collision(character_position_x + speed * sin(rotate_camera_x), character_position_z) && 
							!check_diamond_collision(character_position_x + speed * sin(rotate_camera_x), character_position_z))
			character_position_x += speed * sin(rotate_camera_x);
	}
	if (leftkey==true){
		//move left as long as we aren't colliding with a diamond or a pillar
		if (!check_diamond_collision(character_position_x - speed * sin(rotate_camera_x + 1.57f), character_position_z) && 
							!check_diamond_collision(character_position_x - speed * sin(rotate_camera_x + 1.57f), character_position_z))
			character_position_x -= speed * sin(rotate_camera_x + 1.57f);
		if (!check_diamond_collision(character_position_x, character_position_z - speed * cos(rotate_camera_x + 1.57f)) && 
							!check_diamond_collision(character_position_x, character_position_z - speed * cos(rotate_camera_x + 1.57f)))
			character_position_z -= speed * cos(rotate_camera_x + 1.57f);
	}
	if (rightkey==true){
		//move right as long as we aren't colliding with a diamond or a pillar
		if (!check_diamond_collision(character_position_x + speed * sin(rotate_camera_x + 1.57f), character_position_z) && 
							!check_diamond_collision(character_position_x + speed * sin(rotate_camera_x + 1.57f), character_position_z))
			character_position_x += speed * sin(rotate_camera_x + 1.57f);
		if (!check_diamond_collision(character_position_x, character_position_z + speed * cos(rotate_camera_x + 1.57f)) && 
							!check_diamond_collision(character_position_x, character_position_z + speed * cos(rotate_camera_x + 1.57f)))
			character_position_z += speed * cos(rotate_camera_x + 1.57f);
	}

	
	//jumping
	if (jump == true){
		jump_size += 0.005f;
		character_position_y = 3.0f * sin(jump_size);
		if (sin(jump_size) <= 0.0f){
			character_position_y = 0.0f;
			jump_size = 0.0f;
			jump= false;
		}
	}
}

//make sure that camera is following the character
void updateCamera(){
	if (i_key){
		rotate_camera_y+= 0.01f;
		if (rotate_camera_y > 1.56f)
			rotate_camera_y = 1.56f;
	}
	if (k_key){
		rotate_camera_y -= 0.01f;
		if (rotate_camera_y < -0.05f)
			rotate_camera_y = -0.05f;
	}
	if (j_key){
		rotate_camera_x -= 0.01f;
	}
	if (l_key){
		rotate_camera_x += 0.01f;
	}
	move_camera_x = (5.0f * character_position_x) + (camera_dist * cos(rotate_camera_y) * sin(rotate_camera_x));
	move_camera_y = camera_dist * sin(rotate_camera_y);
	move_camera_z = (5.0f * character_position_z) + (camera_dist * cos(rotate_camera_y) * cos(rotate_camera_x));
}

//zoom in and out on the character
void mouseWheel(int button, int direction, int x, int y){
		if (direction > 0)
			camera_dist -= 0.5f;
		else
			camera_dist += 0.5f;
}

//function to give the user the option to play again if they want
void show_options() {
	//give the user options
	//play again?
	//or exit

}

void updateScene() {

	// Placeholder code, if you want to work with framerate
	// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static DWORD  last_time = 0;
	DWORD  curr_time = timeGetTime();
	float  delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.03f)
		delta = 0.03f;
	last_time = curr_time;

	string score_string = "";
	score_string.append(to_string((int)curr_score));
	update_text(score_text, score_string.c_str());
	if (gameover == false) {
		//keep rotating going
		bomb_rotation += 1.0f;
		diamond_rotation += 1.0f;
		//move the character
		character_move();
		//check that we are still alive
		check_bomb_collisions();
		//keep moving the bombs
		bomb_logic();
		//character_translation = 0.04f * sin((double)curr_time / 150);
		//make sure the camera state is updated correctly
		updateCamera();
	}
	// Draw the next frame
	glutPostRedisplay();
}


void init(){
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	diamond_logic();
	state =HOMESCREEN;//homescreen

	// load meshes for the objects into a vertex buffer array
	generateObjectBufferMesh(GROUND_ID, GROUND_MESH, ground_vertex_count);
	generateObjectBufferMesh(OBSTACLE_ID, OBSTACLE_MESH, obstacle_vertex_count);
	generateObjectBufferMesh(DIAMOND_ID, DIAMOND_MESH, diamond_vertex_count);
	generateObjectBufferMesh(BOMB_ID, BOMB_MESH, bomb_vertex_count);
	generateObjectBufferMesh(CHARACTER_ID, CHARACTER_MESH, character_vertex_count);
	//generate the textures for these objects
	addTexture(GROUND_TEXTURE_ID, GROUND_TEXTURE);
	addTexture(OBSTACLE_TEXTURE_ID, OBSTACLE_TEXTURE);
	addTexture(CHARACTER_TEXTURE_ID, CHARACTER_TEXTURE);
	addTexture(DIAMOND_TEXTURE_ID, DIAMOND_TEXTURE);

	//start playing music on repeat
	SoundEngine->play2D("../audio/breakout.mp3", true);

}
// Placeholder code for the keypress
void pressNormalKeys(unsigned char key, int x, int y){
	if (gameover == false) {
		switch (key){
		case 'i': //Up
			i_key = true;
			break;
		case 'j': //Left
			j_key = true;
			break;
		case 'k': //Down
			k_key = true;
			break;
		case 'l': //Right
			l_key = true;
			break;
		case 32:
			if (jump == false)
				jump = true;
			break;
		/*case 'x':
			if (state == HOMESCREEN || state == REPLAY_AND_SCORES)
				exit(0);
			break;
		case 'y':
			if (state == REPLAY_AND_SCORES)
				state = PLAYING;*/
		}
		glutPostRedisplay();
	}
}

//ensures we know what happens when we stop pressing a key
void releaseNormalKeys(unsigned char key, int x, int y){
	if (gameover == false) {
		switch (key) {
		case 'i': //Up
			i_key = false;
			break;
		case 'j': //Left
			j_key = false;
			break;
		case 'k': //Down
			k_key = false;
			break;
		case 'l': //Right
			l_key = false;
			break;
		}
		glutPostRedisplay();
	}
}



//prints the game instructions once the console window is displayed
void displayInstructions() {
	cout << "Welcome to KABOOM!" << endl;
	cout << "The rules are as follows: " << endl;
	cout << "\t - To win, collect as many diamonds as you can. " << endl;
	cout << "\t - You will have to navigate around obstacles." << endl;
	cout << "\t - Use the space-bar to jump over the bombs." << endl;
	cout << "\t - If you hit a bomb, the game is over. " << endl;
	cout << "\t - Your top-10 highscores will be saved. " << endl;
	cout << "GOOD LUCK" << endl << endl;
}

// Control the translation of the camera using the arrow keys
void processSpecialKeys(int key, int x, int y) {
	if (gameover == false) {
		switch (key) {
		case GLUT_KEY_UP:
			//character moves forward
			upkey = true;
			break;
		case GLUT_KEY_DOWN:
			//moves backwards
			downkey = true;
			break;
		case GLUT_KEY_LEFT:
			//moves left
			leftkey = true;
			break;
		case GLUT_KEY_RIGHT:
			//moves right
			rightkey = true;
			break;
		}
		glutPostRedisplay();
	}
}


//update things when the key is up
void releaseKeys(int key, int x, int y) {
	if (gameover == false) {
		switch (key) {
		case GLUT_KEY_UP:
			upkey = false;
			break;
		case GLUT_KEY_DOWN:
			downkey = false;
			break;
		case GLUT_KEY_LEFT:
			leftkey = false;
			break;
		case GLUT_KEY_RIGHT:
			rightkey = false;
			break;
		}
		glutPostRedisplay();
	}
}



//move the camera using the mouse
void processMouse(int x, int y) {
	//maybe will use this for rotations
	if (mouse) {
		prev_x = x;
		prev_y = y;
		mouse = false;
	}

	//look at horizontal plane
	if (x>prev_x) {
		rotate_camera_x -= 0.02f;
	}
	else if (x<prev_x) {
		rotate_camera_x += 0.02f;
	}

	//look at vertical plane
	if (y>prev_y) {
		rotate_camera_x -= 0.02f;
	}
	else if (y<prev_y) {
		rotate_camera_x += 0.02f;
	}

	//update the prev variables for next time function is called
	prev_x = x;
	prev_y = y;

	glutPostRedisplay();
}




int main(int argc, char** argv) {
	srand(time(NULL));
	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB); 
	glutInitWindowSize(width, height);
	diamond_logic();

	glutCreateWindow("KABOOM!");
	displayInstructions();

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(pressNormalKeys);
	glutKeyboardUpFunc(releaseNormalKeys);
	glutSpecialFunc(processSpecialKeys);
	glutSpecialUpFunc(releaseKeys);
	glutPassiveMotionFunc(processMouse);
	glutMouseWheelFunc(mouseWheel);


	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
	return 0;
}