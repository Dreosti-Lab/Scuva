// Scuva Display Class Header (OpenGL ES version)
#pragma once

// Include OpenGL ES 3.0 library
#include <GLES2/gl2.h>

// Include OpenGL Math library classes (GLM)
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Include STD headers
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>

// Include local class lbraries
#include "config.h"
//#include "vkWindow\vkWindow.h"

// Lazy Constants
const std::string SOURCE_PATH_ = "../../../../../../src/";
//const std::string SOURCE_PATH_ = "";

// OpenGL data sctructures
// -----------------------
struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

// Vertex data structure
struct Vertex
{
	glm::vec3 pos;
	glm::vec4 color;
};

class Display
{
public:
	// Public members

	// Public methods
	void Initialize(Window* window, Vertex* vertices, int num_vertices);		// Initialize Vulkan display device resources
	void Update(glm::mat4 view);												// Will eventually accept outside transforms
	void Draw();																// Render
	void Wait();																// Wait
	void Cleanup();																// Release resources

private:
	// Private members
	Window*						window_;										// Platform agnostic window and user input sctructure
	UniformBufferObject			ubo_;											// Uniform buffer object
	Vertex*						vertices_;										// Data for vertices
	int							num_vertices_;

	// Members
	GLuint		vertex_buffer;
	GLuint		vertex_shader, fragment_shader, program;
	GLint		model_location, view_location, proj_location, vpos_location, vcol_location;
};

// Helper Functions

// Read test file into string
static std::string read_txt_file(const std::string& filename)
{
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cout << "failed to open file at: " + filename;
	}
	std::string filetext;
	while (file.good()) {
		std::string line;
		std::getline(file, line);
		filetext.append(line + "\n");
	}
	file.close();
	return filetext;
}

// FIN

