// Scuva Display Class Source (OpenGL ES version)

#include "window.h"
#include "display.h"

// vkDisplay Class Public Method Definitions
// -----------------------------------------

// Initialize OpenGL resources
void Display::Initialize(Window* window, Vertex* vertices, int num_vertices)
{
	// Set display device window handle
	window_ = window;

	// Tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"

	// Specify front face winding order
	glFrontFace(GL_CW);

	// Get voxel data loaded from VKS file
	vertices_ = vertices;
	num_vertices_ = num_vertices;

	// Generate and bind vertex buffer
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * num_vertices, vertices, GL_STATIC_DRAW);

	// Define vertex shader
	static const char* vertex_shader_text =
		"#version 310 es\n"
		"uniform mat4 model;\n"
		"uniform mat4 view;\n"
		"uniform mat4 proj;\n"
		"layout(location = 0) in vec3 pos;\n"
		"layout(location = 1) in vec4 color;\n"
		"layout(location = 0) out vec4 fragColor;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = proj * view * model * vec4(pos, 1.0);\n"
		"	fragColor = color;\n"
		"}\n";

	// Define fragment shader
	static const char* fragment_shader_text =
		"#version 310 es\n"
		"precision mediump float;\n"
		"layout(location = 0) in vec4 fragColor;\n"
		"layout(location = 0) out vec4 outColor;\n"
		"void main()\n"
		"{\n"
		"	outColor = fragColor;\n"
		"}\n";

	//	OpenGL ES 2.0 Versions of GLSL shaders 
	//	static const GLchar* vertex_shader_text =
	//		"attribute vec4 pos; \n"
	//		"attribute vec4 color; \n"
	//		"void main() \n"
	//		"{ \n"
	//		" gl_Position = pos; \n"
	//		"} \n";
	//
	// Define fragment shader
	//	static const GLchar* fragment_shader_text =
	//		"precision mediump float; \n"
	//		"void main() \n"
	//		"{ \n"
	//		" gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); \n"
	//		"} \n";

	// Load shader text into C-string
//	std::string vertex_shader_text = read_txt_file(SOURCE_PATH_ + "shaders/voxel_gles.vert");
//	const GLchar *vertex_shader_text_c = vertex_shader_text.c_str();
//	std::string fragment_shader_text = read_txt_file(SOURCE_PATH_ + "shaders/voxel_gles.frag");
//	const GLchar *fragment_shader_text_c = fragment_shader_text.c_str();

	GLint compiled = 0;
	// Load and compile vertex shader
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	LOGD("\nVertex Shader Status: %i", vertex_shader);
	glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
	glCompileShader(vertex_shader);

	// Report shader status and log (errors?)
	LOGD("\nVertex Shader Text:\n%s", vertex_shader_text);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		GLint infoLen = 0;
		glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen) 
		{
			char* buf = (char*)malloc(infoLen);
			if (buf) {
				glGetShaderInfoLog(vertex_shader, infoLen, NULL, buf);
				LOGD("Could not compile Vertex shader: %s\n", buf);
				free(buf);
			}
			glDeleteShader(vertex_shader);
			vertex_shader = 0;
		}
	}

	// Load and compile fragment shader
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	LOGD("\nFragment Shader Status: %i", fragment_shader);
	glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
	glCompileShader(fragment_shader);

	// Report shader status and log (errors?)
	LOGD("\nFragment Shader:\n%s", fragment_shader_text);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		GLint infoLen = 0;
		glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen) {
			char* buf = (char*)malloc(infoLen);
			if (buf) {
				glGetShaderInfoLog(fragment_shader, infoLen, NULL, buf);
				LOGD("Could not compile Fragment shader: %s\n", buf);
				free(buf);
			}
			glDeleteShader(fragment_shader);
			fragment_shader = 0;
		}
	}

	// Attach shaders to a "program"
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	// Gather addresses of uniforms (matrices, vertex positions and colors)
	model_location = glGetUniformLocation(program, "model");
	view_location = glGetUniformLocation(program, "view");
	proj_location = glGetUniformLocation(program, "proj");
	vpos_location = glGetAttribLocation(program, "pos");
	vcol_location = glGetAttribLocation(program, "color");
	
	// Enable vertex attribute arrays
	glEnableVertexAttribArray(vpos_location);
	glVertexAttribPointer(vpos_location, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (void*)0);
	glEnableVertexAttribArray(vcol_location);
	glVertexAttribPointer(vcol_location, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (void*)(sizeof(float) * 3));
}

// Update dynamic variables
void Display::Update(glm::mat4 view)
{
	// Local variables
	glm::mat4x4 model, proj;

	// Create a MODEL matrix (identity)
	model = glm::mat4(1.0);

	// Create a PROJECTION matrix (perspective)
	proj = glm::perspective(glm::radians(90.0f), (float)window_->width_ / window_->height_, 0.01f, 100.f);
	proj[1][1] *= -1; // Flip y-axis scaling to convert to Vulkan coorindates

	// Store new MVP values
	ubo_.model = model;
	ubo_.view = view;
	ubo_.proj = proj;
}

// Render to screen
void Display::Draw()
{
	// Set viewport
	glViewport(0, 0, window_->width_, window_->height_);

	// Set clear color and clear buffer
	glClearColor(0.0f, 0.05f, 0.25f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Run shaders (draw points)
	glUseProgram(program);
	glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(ubo_.model));
	glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(ubo_.view));
	glUniformMatrix4fv(proj_location, 1, GL_FALSE, glm::value_ptr(ubo_.proj));

	// Draw vertices
	glDrawArrays(GL_TRIANGLES, 0, num_vertices_);
}

// Wait for everything to finish
void Display::Wait()
{


}


// Close window (and terminate GLFW)
void Display::Cleanup()
{

}


