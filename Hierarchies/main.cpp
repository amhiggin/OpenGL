//Some Windows Headers (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>

#include "maths_funcs.h" //Anton's math class
#include "teapot.h" // teapot mesh
#include <string> 
#include <fstream>
#include <iostream>
#include <sstream>


typedef double D_WORD;
mat4 transform;

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;
GLuint shaderProgramID;

unsigned int teapot_vao = 0;
int width = 800.0;
int height = 600.0;
int object=0;						//denoting the selected object for moving - we assume not moving initially
GLuint loc1;
GLuint loc2;
GLfloat rotatey[8] = { 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};	//for whatever level we are going to rotate at
GLfloat rotatez[8] = { 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f };	//for whatever level we are going to rotate at
//for manipulating the root object
GLfloat translateRoot = 0.0f;
GLfloat rotateRoot = 0.0f;


// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS

std::string readShaderSource(const std::string& fileName)
{
	std::ifstream file(fileName.c_str());
	if (file.fail()) {
		cout << "error loading shader called " << fileName;
		exit(1);
	}

	std::stringstream stream;
	stream << file.rdbuf();
	file.close();

	return stream.str();
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}
	std::string outShader = readShaderSource(pShaderText);
	const char* pShaderSource = outShader.c_str();

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
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
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
	AddShader(shaderProgramID, "../Graphics3/Shaders/simpleVertexShader.txt", GL_VERTEX_SHADER);
	AddShader(shaderProgramID, "../Graphics3/Shaders/simpleFragmentShader.txt", GL_FRAGMENT_SHADER);

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





void generateObjectBufferTeapot() {
	GLuint vp_vbo = 0;

	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normals");

	glGenBuffers(1, &vp_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * teapot_vertex_count * sizeof(float), teapot_vertex_points, GL_STATIC_DRAW);
	GLuint vn_vbo = 0;
	glGenBuffers(1, &vn_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * teapot_vertex_count * sizeof(float), teapot_normals, GL_STATIC_DRAW);

	glGenVertexArrays(1, &teapot_vao);
	glBindVertexArray(teapot_vao);

	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
}


#pragma endregion VBO_FUNCTIONS


//to display the viewports
void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderProgramID);
	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation(shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation(shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation(shaderProgramID, "proj");

	// Hierarchy of Teapots

	// Root of the Hierarchy
	mat4 view = identity_mat4();
	mat4 persp_proj = perspective(60.0, (float)width / (float)height, 0.1, 100.0);
	mat4 local1 = identity_mat4();
	//SET UP INITIAL ROOT POSITION
	local1 = scale(local1, vec3(0.75f, 0.75f, 0.75f));		
	local1 = rotate_y_deg(local1, rotatey[0]);
	local1 = rotate_z_deg(local1, rotatez[0]);
	local1 = translate(local1, vec3(0.0, -20.0, translateRoot));		
	local1 = rotate_y_deg(local1, rotateRoot);

	// for the root, we orient it in global space
	mat4 global1 = local1;
	// update uniforms & draw
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global1.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);


	/*
	*	LEVEL ABOVE THE ROOT
	*/

	// child of hierarchy
	mat4 local2a = identity_mat4();
	local2a = translate(local2a, vec3(20.0, 15.0, 0.0));
	local2a = rotate_y_deg(local2a, rotatey[1]);
	local2a = rotate_z_deg(local2a, rotatez[1]);
	mat4 global2a = global1*local2a;
	// update uniform & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global2a.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	// child of hierarchy
	mat4 local2b = identity_mat4();
	local2b = rotate_y_deg(local2b, rotatey[1]);
	local2b = rotate_z_deg(local2b, rotatez[1]);
	// translation is 15 units in the y direction from the parents coordinate system
	local2b = translate(local2b, vec3(0.0, 15.0, 0.0));
	// global of the child is got by pre-multiplying the local of the child by the global of the parent
	mat4 global2b = global1*local2b;
	// update uniform & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global2b.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	// child of hierarchy
	mat4 local2c = identity_mat4();
	local2c = rotate_y_deg(local2c, 180);
	// translation is 15 units in the y direction from the parents coordinate system
	local2c = translate(local2c, vec3(-20.0, 15.0, 0.0));
	local2c = rotate_y_deg(local2c, rotatey[1]);
	local2c = rotate_z_deg(local2c, rotatez[1]);
	// global of the child is got by pre-multiplying the local of the child by the global of the parent
	mat4 global2c = global1*local2c;
	// update uniform & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global2c.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);



	/*
	*	KEEP STACKING UP ON TOP 
	*/

	// child of hierarchy
	mat4 local3a = identity_mat4();
	local3a = rotate_y_deg(local3a, rotatey[2]);
	local3a = rotate_z_deg(local3a, rotatez[2]);
	// translation is 15 units in the y direction from the parents coordinate system
	local3a = translate(local3a, vec3(0.0, 15.0, 0.0));
	// global of the child is got by pre-multiplying the local of the child by the global of the parent
	mat4 global3a = global2b*local3a;
	// update uniform & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global3a.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	// child of hierarchy
	mat4 local4a = identity_mat4();
	local4a = rotate_y_deg(local4a, rotatey[3]);
	local4a = rotate_z_deg(local4a, rotatez[3]);
	// translation is 15 units in the y direction from the parents coordinate system
	local4a = translate(local4a, vec3(10.0, 15.0, 0.0));
	// global of the child is got by pre-multiplying the local of the child by the global of the parent
	mat4 global4a = global3a*local4a;
	// update uniform & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global4a.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	// child of hierarchy
	mat4 local4b = identity_mat4();
	// translation is 15 units in the y direction from the parents coordinate system
	local4b = translate(local4b, vec3(-10.0, 15.0, 0.0));
	// global of the child is got by pre-multiplying the local of the child by the global of the parent
	local4b = rotate_y_deg(local4b, rotatey[3]);
	local4b = rotate_z_deg(local4b, rotatez[3]);
	mat4 global4b = global3a*local4b;
	// update uniform & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global4b.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);


	// child of hierarchy
	mat4 local5a = identity_mat4();
	local5a = rotate_z_deg(local5a, -135.0);
	// translation is 15 units in the y direction from the parents coordinate system
	local5a = translate(local5a, vec3(5.0, 15.0, 0.0));
	local5a = rotate_y_deg(local5a, rotatey[4]);
	local5a = rotate_z_deg(local5a, rotatez[4]);
	// global of the child is got by pre-multiplying the local of the child by the global of the parent
	mat4 global5a = global4a*local5a;
	// update uniform & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global5a.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	// child of hierarchy
	mat4 local5b = identity_mat4();
	local5b = rotate_z_deg(local5b, 135.0);
	// translation is 15 units in the y direction from the parents coordinate system
	local5b = translate(local5b, vec3(-5.0, 15.0, 0.0));
	local5b = rotate_y_deg(local5b, rotatey[5]);
	local5b = rotate_z_deg(local5b, rotatez[5]);
	// global of the child is got by pre-multiplying the local of the child by the global of the parent
	mat4 global5b = global4b*local5b;
	// update uniform & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global5b.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	// child of hierarchy
	mat4 local6a = identity_mat4();
	// translation is 15 units in the y direction from the parents coordinate system
	local6a = translate(local6a, vec3(-25.0, 0.0, 0.0));
	local6a = rotate_y_deg(local6a, rotatey[6]);
	local6a = rotate_z_deg(local6a, rotatez[6]);
	// global of the child is got by pre-multiplying the local of the child by the global of the parent
	mat4 global6a = global5a*local6a;
	// update uniform & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global6a.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);

	// child of hierarchy
	mat4 local6b = identity_mat4();
	local6b = translate(local6b, vec3(25.0, 0.0, 0.0));
	local6b = rotate_y_deg(local6b, rotatey[7]);
	local6b = rotate_z_deg(local6b, rotatez[7]);
	mat4 global6b = global5b*local6b;
	// update uniform & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global6b.m);
	glDrawArrays(GL_TRIANGLES, 0, teapot_vertex_count);



	glutSwapBuffers();
}


void updateScene() {
	// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static D_WORD  last_time = 0;
	D_WORD  curr_time = timeGetTime();
	float  delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.03f)
		delta = 0.03f;
	last_time = curr_time;

	// Draw the next frame
	glutPostRedisplay();
}

//function to deal with keypresses and perform transformations
void keyPressed(unsigned char key, int i, int y){
	//move level 1 objects in z and y
	if (key == 'a') 
		rotatey[0] += 1.5f;
	if (key == 'A') 
		rotatey[0] -= 1.5f;
	if (key == 'b')
		rotatez[0] += 1.5f;
	if (key == 'B')
		rotatez[0] -= 1.5f;
	//move level 2 objects in z and y
	if (key == 'c') 
		rotatey[1] += 1.5f;
	if (key == 'C') 
		rotatey[1] -= 1.5f;
	if (key == 'd') 
		rotatez[1] += 1.5f;
	if (key == 'D') 
		rotatez[1] -= 1.5f;
	//move level 3 objects in z and y
	if (key =='e') 
		rotatey[2] += 1.5f;
	if (key == 'E') 
		rotatey[2] -= 1.5f;
	if (key == 'f') {
		rotatez[2] += 1.5f;
	if (key == 'F')
		rotatez[2] -= 1.5f;
	//move level 4 objects in z and y
	if (key == 'g')
		rotatey[3] += 1.5f;
	if (key == 'G')
		rotatey[3] -= 1.5f;
	if (key == 'h') 
		rotatez[3] += 1.5f;
	if (key == 'H') 
		rotatez[3] -= 1.5f;
	//move level 5 object 1 in z and y
	if (key == 'i') 
		rotatey[4] += 1.5f;
	if (key == 'I') 
		rotatey[4] -= 1.5f;
	if (key == 'j') 
		rotatez[4] += 1.5f;
	if (key == 'J') 
		rotatez[4] -= 1.5f;
	//move level 5 object 2 in z and y
	if (key == 'k') 
		rotatey[5] += 1.5f;
	if (key == 'k') 
		rotatey[5] -= 1.5f;
	if (key == 'l') 
		rotatez[5] += 1.5f;
	if (key == 'L') 
		rotatez[5] -= 1.5f;
	//move level 6 object 1 in z and y
	if (key == 'm') 
		rotatey[6] += 1.5f;
	if (key == 'M') 
		rotatey[6] -= 1.5f;
	if (key == 'n') 
		rotatez[6] += 1.5f;
	if (key == 'N') 
		rotatez[6] -= 1.5f;
	//move level 6 object 2 in z and y
	if (key == 'o') 
		rotatey[7] += 1.5f;
	if (key == 'O') 
		rotatey[7] -= 1.5f;
	if (key == 'p') 
		rotatez[7] += 1.5f;
	if (key == 'P') 
		rotatez[7] -= 1.5f;


	if (key == 'x') {
		//reset everything
		for (int i = 0; i < 8; i++) {
			rotatey[i] = 0.0f;
			rotatez[i] = 0.0f;
		}
		translateRoot = -100.0f;
		rotateRoot = 0.0f;
	}

	glutPostRedisplay();
}



void processSpecialKeys(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_UP:
			translateRoot += 2.0f;
			break;
		case GLUT_KEY_DOWN:
			translateRoot -= 2.0f;
			break;
		case GLUT_KEY_LEFT:
			rotateRoot += 2.0f;
			break;
		case GLUT_KEY_RIGHT:
			rotateRoot -= 2.0f;
			break;
	}
	glutPostRedisplay();
}



void init()
{
	// Create 3 vertices that make up a triangle that fits on the viewport 
	GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, 1.0,
		1.0f, -1.0f, 0.0f, 1.0,
		0.0f, 1.0f, 0.0f, 1.0 };
	// Create a color array that identfies the colors of each vertex (format R, G, B, A)
	GLfloat colors[] = { 0.0f, 1.0f, 0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f, 1.0f };
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();

	// load teapot mesh into a vertex buffer array
	generateObjectBufferTeapot();

}

int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("HELLO MR TEAPOT!!");
	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keyPressed);					//to deal with the hierarchical movement keypresses (numerical keys)
	glutSpecialFunc(processSpecialKeys);			//to deal with the base translations (Arrowkeys)

	// A call to glewInit() must be done after glut is initialized!
	glewExperimental = GL_TRUE; //for non-lab machines, this line gives better modern GL support
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
