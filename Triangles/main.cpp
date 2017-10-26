#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "maths_funcs.h"

/********************************************************************************************************************************************************/
/*		HIGH LEVEL DESCRIPTION:																															*/
/*			-2 triangles																																*/
/*			-rotations are performed simulatneously on both, and can rotate in both +ve (lowercase) and -ve (uppercase) directions about each axis		*/
/*			-scalings are performed as follows:																											*/
/*				> x-scaling (triangle 1	)																												*/
/*				> y-scaling (triangle 2)																												*/
/*				> z-scaling (both triangles)																											*/
/*				> balanced scaling performed on both triangles simultaneously																			*/	
/*			 The scalings are also performed in terms of increase (lowercase) and decrease (uppercase)													*/
/*			-translations are performed as follows, and can be done in both +ve (lowercase) and -ve (uppercase) directions:								*/
/*				> translation in x (triangle 1)																											*/	
/*				> translation in y (triangle 2)																											*/
/*				> translation in z (both triangles)																										*/
/*			-multiple transformations can be performed on the triangles at the same time by pressing 'b' (forward) and 'B' (backwards).					*/
/********************************************************************************************************************************************************/



// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
using namespace std;

//create 2 identity matrices for manipulation of the objects in x, y, z planes
mat4 firsttransform = identity_mat4();		//matrix for triangle 1 manipulation
mat4 secondtransform = identity_mat4();		//matrix for triangle 2 manipulation


// Vertex Shader (for convenience, it is defined in the main here, but we will be using text files for shaders in future)
// Note: Input to this shader is the vertex positions that we specified for the triangle. 
// Note: gl_Position is a special built-in variable that is supposed to contain the vertex position (in X, Y, Z, W)
// Since our triangle vertices were specified as vec3, we just set W to 1.0.
static const char* pVS = "														\n\
#version 330																	\n\
																				\n\
in vec3 vPosition;																\n\
in vec4 vColor;																	\n\
out vec4 color;																	\n\
uniform mat4 Mat_position;	                                                    \n\
                                                                                \n\
void main()                                                                     \n\
{                                                                               \n\
    gl_Position = Mat_position*vec4(vPosition, 1.0);							\n\
	color = vColor;																\n\
}";

GLuint OBJECT_ID = 0;		//for linking shader to generated objects

// Fragment Shader
// Note: no input in this shader, it just outputs the colour of all fragments, in this case set to red (format: R, G, B, A).
static const char* pFS = "														\n\
#version 330																	\n\
																				\n\
out vec4 FragColor;																\n\
in vec4 color;																	\n\
void main()																		\n\
{																				\n\
FragColor = color;																\n\
}";


// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}
	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderText, NULL);
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
	GLuint shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, pVS, GL_VERTEX_SHADER);
	AddShader(shaderProgramID, pFS, GL_FRAGMENT_SHADER);

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
GLuint generateObjectBuffer(GLfloat vertices[], GLfloat colors[]) {
	GLuint numVertices = 6;		//for 2 triangles (3x2 vertices)

	// Genderate 1 generic buffer object, called VBO
	GLuint VBO;
	glGenBuffers(1, &VBO);
	// In OpenGL, we bind (make active) the handle to a target name and then execute commands on that target
	// Buffer will contain an array of vertices 
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// After binding, we now fill our object with data, everything in "Vertices" goes to the GPU
	glBufferData(GL_ARRAY_BUFFER, numVertices * 7 * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	// if you have more data besides vertices (e.g., vertex colours or normals), use glBufferSubData to tell the buffer when the vertices array ends and when the colors start
	glBufferSubData(GL_ARRAY_BUFFER, 0, numVertices * 3 * sizeof(GLfloat), vertices);
	glBufferSubData(GL_ARRAY_BUFFER, numVertices * 3 * sizeof(GLfloat), numVertices * 4 * sizeof(GLfloat), colors);
	return VBO;
}

void linkCurrentBuffertoShader(GLuint shaderProgramID) {
	GLuint numVertices = 6;		//for 2 triangles (3x2 vertices)
	// find the location of the variables that we will be using in the shader program
	GLuint positionID = glGetAttribLocation(shaderProgramID, "vPosition");
	GLuint colorID = glGetAttribLocation(shaderProgramID, "vColor");
	// Have to enable this
	glEnableVertexAttribArray(positionID);
	// Tell it where to find the position data in the currently active buffer (at index positionID)
	glVertexAttribPointer(positionID, 3, GL_FLOAT, GL_FALSE, 0, 0);
	// Similarly, for the color data.
	glEnableVertexAttribArray(colorID);
	glVertexAttribPointer(colorID, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(numVertices * 3 * sizeof(GLfloat)));
	OBJECT_ID = glGetUniformLocation(shaderProgramID, "Mat_position");		//link to the shader
}
#pragma endregion VBO_FUNCTIONS


void display() {

	glClear(GL_COLOR_BUFFER_BIT);
	
	//draw triangle 1
	glUniformMatrix4fv(OBJECT_ID, 1, GL_FALSE, &firsttransform.m[0]);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	//draw triangle 2
	glUniformMatrix4fv(OBJECT_ID, 1, GL_FALSE, &secondtransform.m[0]);
	glDrawArrays(GL_TRIANGLES, 3, 6);

	glutSwapBuffers();
}

//function to deal with keypresses and perform transformations
void keyPressed(unsigned char key, int i, int y)
{
	/********************/
	/*	  ROTATIONS		*/
	/********************/
	if (key == 'z'){		//Rotate about x (+ve)
			firsttransform = rotate_x_deg(firsttransform, 5.0);
			secondtransform = rotate_x_deg(secondtransform, 5.0);
	}
	if (key == 'Z') {		//Rotate about x (-ve)
		firsttransform = rotate_x_deg(firsttransform, -5.0);
		secondtransform = rotate_x_deg(secondtransform, -5.0);
	}
	if (key == 'x'){		//Rotate about y (+ve)
			firsttransform = rotate_y_deg(firsttransform, 5.0);
			secondtransform = rotate_y_deg(secondtransform, 5.0);
	}
	if (key == 'X') {		//Rotate about y (-ve)
		firsttransform = rotate_y_deg(firsttransform, -5.0);
		secondtransform = rotate_y_deg(secondtransform, -5.0);
	}
	if (key == 'c'){		//Rotate about z (+ve)
			firsttransform = rotate_z_deg(firsttransform, 5.0);
			secondtransform = rotate_z_deg(secondtransform, 5.0);
	}
	if (key == 'C') {		//Rotate about z (-ve)
		firsttransform = rotate_z_deg(firsttransform, -5.0);
		secondtransform = rotate_z_deg(secondtransform, -5.0);
	}

	
	/********************/
	/*	   SCALINGS		*/
	/********************/

	//Unbalanced scalings
	if (key == 'y'){					//decrease scale of t1 in x
		firsttransform = scale(firsttransform, { 0.95f, 1.0f, 1.0f });
	}
	if (key == 'Y') {					//increase scale of t1 in x
		firsttransform = scale(firsttransform, { 1.2f, 1.0f, 1.0f });
	}
	if (key == 'u') {					//decrease scale of t2 in y
		secondtransform = scale(secondtransform, { 1.0f, 0.95f, 1.0f });	
	}
	if (key == 'U') {					//increase scale of t2 in y
		secondtransform = scale(secondtransform, { 1.0f, 1.2f, 1.0f });
	}
	if (key == 'i') {					//decrease scale of both triangles in z
		firsttransform = scale(firsttransform, { 1.0f, 1.0f, 0.95f });
		secondtransform = scale(secondtransform, { 1.0f, 1.0f, 0.95f });
	}
	if (key == 'I') {					//increase scale of both triangles in z
		firsttransform = scale(firsttransform, { 1.0f, 1.0f, 1.2f });
		secondtransform = scale(secondtransform, { 1.0f, 1.0f, 1.2f });
	}
	
	//Now balanced scalings
	if (key == 'q') {					//decrease scale of both triangles in all directions uniformly
		firsttransform = scale(firsttransform, { 0.8f, 0.8f, 0.8f });
		secondtransform = scale(secondtransform, { 0.8f, 0.8f, 0.8f });
	}
	if (key == 'Q') {					//increase scale of both triangles in all directions uniformly
		firsttransform = scale(firsttransform, { 1.6f, 1.6f, 1.6f });
		secondtransform = scale(secondtransform, { 1.6f, 1.6f, 1.6f });
	}


	/********************/
	/*   TRANSLATIONS	*/
	/********************/

	//X translations - triangle 1
	if (key == 'f'){					//translate t1 in x (+ve)
			firsttransform = translate(firsttransform, { 0.5f, 0.0f, 0.0f });
	}
	if (key == 'F') {					//translate t1 in x (-ve)
		firsttransform = translate(firsttransform, {-0.5f, 0.0f, 0.0f });
	}

	//Y translations - triangle 2
	if (key == 'g'){					//translate t2 in y (+ve)
			secondtransform = translate(secondtransform, { 0.0f, 0.5f, 0.0f });
	}
	if (key == 'G') {					//translate t2 in y (-ve)
		secondtransform = translate(secondtransform, { 0.0f, -0.5f, 0.0f });
	}

	//Z translations - both triangles
	if (key == 'h'){					//translate both triangles in z (+ve)
			firsttransform = translate(firsttransform, { 0.0f, 0.0f, 0.5f });
			secondtransform = translate(secondtransform, { 0.0f, 0.0f, 0.5f });
	}
	if (key == 'H') {					//translate both triangles in z (-ve)
		firsttransform = translate(firsttransform, { 0.0f, 0.0f, -0.5f });
		secondtransform = translate(secondtransform, { 0.0f, 0.0f, -0.5f });
	}

	/************************/
	/*   COMBINED FUNCTIONS	*/
	/************************/

	if (key == 'b'){

		firsttransform = rotate_x_deg(firsttransform, 20.0);
		firsttransform = rotate_y_deg(firsttransform, -20.0);
		firsttransform = scale(firsttransform, { 0.8f, 0.8f, 0.8f });
		//do stuff to triangle 2

		secondtransform = rotate_x_deg(secondtransform, -20.0);
		secondtransform = rotate_y_deg(secondtransform, 20.0);
		secondtransform = scale(secondtransform, { 1.5f, 1.5f, 1.5f });
	}
	if (key == 'B') {
		//undo stuff to triangle 1
		firsttransform = rotate_x_deg(firsttransform, -20.0);
		firsttransform = rotate_y_deg(firsttransform, 20.0);
		firsttransform = scale(firsttransform, { 1.5f, 1.5f, 1.5f });
		//undo stuff to triangle 2
		secondtransform = rotate_x_deg(secondtransform, 20.0);
		secondtransform = rotate_y_deg(secondtransform, -20.0);
		secondtransform = scale(secondtransform, { 0.8f, 0.8f, 0.8f });
	}
	//call display function to display the selected transformation
	display();
}



void init()
{
	// Create 3 vertices that make up a triangle that fits on the viewport 
	GLfloat vertices[] = { -0.5f, 0.0f, 0.5f, //BL
		0.0f, 0.0f, 0.5f,//BR
		0.5f, 0.5f, 0.5f,
		0.5f, -0.5f, 0.5f, //BL
		0.5f, 0.0f, 0.5f,//BR
		0.0f, -0.5f, 0.5f//T
	}; //T
	   // Create a color array that identfies the colors of each vertex (format R, G, B, A)
	GLfloat colors[] = { 0.0f, 1.0f, 0.5f, 1.0f,
		0.0f, 0.5f, 1.0f, 1.0f,
		0.0f, 0.5f, 0.0f, 1.0,
		0.0f, 0.0f, 1.0f, 1.0f,
		1.0f, 0.0f, 0.50f, 1.0f,
		1.0f, 0.0f, 1.0f, 1.0 };
	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	// Put the vertices and colors into a vertex buffer object
	generateObjectBuffer(vertices, colors);
	// Link the current buffer to the shader
	linkCurrentBuffertoShader(shaderProgramID);
}


int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(600, 600);
	glutCreateWindow("Transformations!!");
	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(display);

	//call the keypress function to deal with our keypresses
	glutKeyboardFunc(keyPressed);

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