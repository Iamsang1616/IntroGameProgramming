#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "ShaderProgram.h"
#include "Matrix.h"
#include <iostream>
#include <assert.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

//Function to load texture into memory
GLuint LoadTexture(const char* filePath) {
	int w, h, comp;

	//Load the image from the filepath
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load the image. Make sure you have the right path" << std::endl;
		assert(false);
	}

	GLuint retTexture;
	//Make a new texture ID
	glGenTextures(1, &retTexture);
	//Bind it
	glBindTexture(GL_TEXTURE_2D, retTexture);
	//Assign properties to it, including the image
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	//Set scaling properties
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Free up the allocated memory for the image
	stbi_image_free(image);
	//Return the number associated with the new texture
	return retTexture;

}



int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

	#ifdef _WINDOWS
		glewInit();
	#endif

	glViewport(0, 0, 640, 360);
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	GLuint mytexture1 = LoadTexture(RESOURCE_FOLDER"boxCoin.png");
	GLuint mytexture2 = LoadTexture(RESOURCE_FOLDER"CORN.jpg");
	GLuint mytexture3 = LoadTexture(RESOURCE_FOLDER"meat.png");

	float last_frame_ticks = 0.0f;
	float angle = 0;

	Matrix proMatrix;
	proMatrix.SetOrthoProjection(-5.33f, 5.33f, -3.0f, 3.0f, -1.0f, 1.0f);
	Matrix mvMatrix;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	SDL_Event event;
	bool done = false;

	//==============================Main Loop of the game============================================
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - last_frame_ticks;
		last_frame_ticks = ticks;

		angle += 3.14 / 5000;

		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.4f, 0.2f, 0.3f, 1.0f);

		glUseProgram(program.programID);
		mvMatrix.Translate(4, -2, 0);
		program.SetModelviewMatrix(mvMatrix);
		program.SetProjectionMatrix(proMatrix);

		glBindTexture(GL_TEXTURE_2D, mytexture1);

		float vertices[] = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);
		

		float vcoord[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, vcoord);
		glEnableVertexAttribArray(program.texCoordAttribute);
		
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);


		//Make Corn
		mvMatrix.Identity();
		mvMatrix.Translate(0, -1, 0);
		mvMatrix.Rotate(angle);
		program.SetModelviewMatrix(mvMatrix);

		float cornVertices[] = { -0.8f, -0.8f, 0.8f, -0.8f, 0.8f, 0.8f, -0.8, -0.8, 0.8, 0.8, -0.8, 0.8 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, cornVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float cornVcoord[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, cornVcoord);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, mytexture2);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		//Make Meat
		mvMatrix.Identity();
		mvMatrix.Translate(-2, 1, 0);

		program.SetModelviewMatrix(mvMatrix);


		float meatVertices[] = { -0.3f, -0.4f, 0.3f, -0.4f, 0.3f, 0.4f, -0.3, -0.4, 0.3, 0.4, -0.3, 0.4 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, meatVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float meatVcoord[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, meatVcoord);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, mytexture3);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}


