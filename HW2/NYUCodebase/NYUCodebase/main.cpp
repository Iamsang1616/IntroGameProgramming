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

//Paddle Entity class
class Paddle {
	public:

		Paddle(float l, float r, float t, float b) : left(l), right(r), top(t), bottom(b) {}

		void defaultLeft() {
			left = -5;
			right = -4.7;
			top = 0.8;
			bottom = -0.8;
		}

		void defaultRight() {
			left = 4.7;
			right = 5;
			top = 0.8;
			bottom = -0.8;
		}

		float left;
		float right;
		float top;
		float bottom;
};

class Ball {

	public:
		
		Ball(float l, float r, float t, float b) : left(l), right(r), top(t), bottom(b){}
		
		void defaultBall() {
			left = -0.2;
			right = 0.2;
			top = 0.2;
			bottom = -0.2;
		}

	 	float left;
		float right;
		float top;
		float bottom;

		float y_vel = 0.0;
		float x_vel = 0.0;
};

//Setup function
void Setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("WRONG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 576, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

	#ifdef _WINDOWS
		glewInit();
	#endif
	
	glViewport(0, 0, 1024, 576);
}

int main(int argc, char *argv[])
{
	//set stuff up	
	Setup();
	
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	GLuint mytexture1 = LoadTexture(RESOURCE_FOLDER"boxCoin.png");
	GLuint mytexture2 = LoadTexture(RESOURCE_FOLDER"CORN.jpg");
	GLuint mytexture3 = LoadTexture(RESOURCE_FOLDER"meat.png");

	float last_frame_ticks = 0.0f;

	Matrix proMatrix;
	proMatrix.SetOrthoProjection(-5.33f, 5.33f, -3.0f, 3.0f, -1.0f, 1.0f);

	//Matrices for the entities of the game
	Matrix Paddle1_Matrix;
	Matrix Paddle2_Matrix;
	Matrix BallMatrix;



	Paddle paddle1(-5.0, -4.7, 0.8, -0.8 );
	Paddle paddle2(4.7, 5.0, 0.8, -0.8);
	Ball theBall(-0.2, 0.2, 0.2, -0.2);

	SDL_Event event;
	bool done = false;
	
	//==============================Main Loop of the game============================================
	while (!done) {

		
		glClearColor(0.5f, 0.2f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - last_frame_ticks;
		last_frame_ticks = ticks;



		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		//left paddle movement
		//left paddle up
		if (keys[SDL_SCANCODE_W]) {
			if (paddle1.top <= 3.0) {
				

				paddle1.top += (2.5 * elapsed);
				paddle1.bottom += (2.5 * elapsed);
				Paddle1_Matrix.Translate(0.0, (2.5 * elapsed), 0.0);


			}

		}
		//left paddle down, unless at bottom of screen
		if (keys[SDL_SCANCODE_S]) {
			if (paddle1.bottom >= -3.0) {
				

				paddle1.top -= (2.5 * elapsed);
				paddle1.bottom -= (2.5 * elapsed);
				Paddle1_Matrix.Translate(0.0, (-2.5 * elapsed), 0.0);


			}
				
		}

		//right paddle movement
		//right paddle up
		if (keys[SDL_SCANCODE_UP]) {
			if (paddle2.top <= 3.0) {
				

				paddle2.top += (2.5 * elapsed);
				paddle2.bottom += (2.5 * elapsed);
				Paddle2_Matrix.Translate(0.0, (2.5 * elapsed), 0.0);


			}

		}
		//right paddle down
		if (keys[SDL_SCANCODE_DOWN]) {
			if (paddle2.bottom >= -3.0) {
				

				paddle2.top -= (2.5 * elapsed);
				paddle2.bottom -= (2.5 * elapsed);
				Paddle2_Matrix.Translate(0.0, (-2.5 * elapsed), 0.0);


			}
				
		}

		//Check if window has closed
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		

		glUseProgram(program.programID);
		program.SetProjectionMatrix(proMatrix);

		//Draw first paddle
		program.SetModelviewMatrix(Paddle1_Matrix);

		glBindTexture(GL_TEXTURE_2D, mytexture1);
		//Set to left side of screen
		float P1_verts[] = { -5.0f, -0.8f, -4.7f, -0.8f, -4.7f, 0.8f, -5.0, -0.8, -4.7, 0.8, -5.0, 0.8 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, P1_verts);
		glEnableVertexAttribArray(program.positionAttribute);
		
		float vcoord[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, vcoord);
		glEnableVertexAttribArray(program.texCoordAttribute);
		
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);


		program.SetModelviewMatrix(Paddle2_Matrix);

		glBindTexture(GL_TEXTURE_2D, mytexture1);
		float P2_verts[] = { 4.7f, -0.8f, 5.0f, -0.8f, 5.0f, 0.8f, 4.7, -0.8, 5.0, 0.8, 4.7, 0.8 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, P2_verts);
		glEnableVertexAttribArray(program.positionAttribute);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, vcoord);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);



		//Draw ball

		//Kickstart ball at start of game upward, make it an easy shot
		if (theBall.y_vel == 0.0) {
			theBall.y_vel = 0.1 * elapsed;
		}
		//Kickstart ball at start of game rightward
		if (theBall.x_vel == 0.0) {
			theBall.x_vel = 0.1 * elapsed;
		}



		//if ball's top is to go off the top, turn it around
		if (theBall.top >= 3.0) {
			theBall.y_vel = -1.5 * elapsed;


		}
		//if the ball is about to go off the bottom, turn it around
		else if (theBall.bottom <= -3.0) {
			theBall.y_vel = 1.5 * elapsed;


		}


		//Ball resets when it passes a boundary
		if (theBall.left <= -5.33) {

			BallMatrix.Identity();
			Paddle1_Matrix.Identity();
			Paddle2_Matrix.Identity();

			theBall.defaultBall();
			paddle1.defaultLeft();
			paddle2.defaultRight();
			


			//Pause game briefly, declare a winner through background color changes (hopefully)
			float timer = 0.0;

			

			while (timer < 1) {
				ticks = (float)SDL_GetTicks() / 1000.0f;
				elapsed = ticks - last_frame_ticks;
				timer += elapsed;
				last_frame_ticks = ticks;

				glClearColor(0.5f, 0.0f, 0.0f, 0.5f);
				glClear(GL_COLOR_BUFFER_BIT);
				
				SDL_GL_SwapWindow(displayWindow);

			}
			
		}
		else if (theBall.right > 5.33) {
			BallMatrix.Identity();
			Paddle1_Matrix.Identity();
			Paddle2_Matrix.Identity();

			theBall.defaultBall();
			paddle1.defaultLeft();
			paddle2.defaultRight();

			//Pause game briefly, declare a winner through background color changes (hopefully)
			float timer = 0.0;
			while (timer < 1) {
				ticks = (float)SDL_GetTicks() / 1000.0f;
				elapsed = ticks - last_frame_ticks;
				timer += elapsed;
				last_frame_ticks = ticks;


				glClearColor(0.0f, 0.0f, 0.5f, 0.5f);
				glClear(GL_COLOR_BUFFER_BIT);

				SDL_GL_SwapWindow(displayWindow);

			}
		}


		

		//If ball collides with left paddle, send it right
		if (!(paddle1.bottom > theBall.top || paddle1.top < theBall.bottom || paddle1.left > theBall.right || paddle1.right < theBall.left)) {
			theBall.x_vel = 1.5 * elapsed;
		}
		//If ball collides with right paddle, send it left
		if (!(paddle2.bottom > theBall.top || paddle2.top < theBall.bottom || paddle2.left > theBall.right || paddle2.right < theBall.left)) {
			theBall.x_vel = -1.5 * elapsed;
		}

		//Move the ball's mvMatrix
		BallMatrix.Translate((theBall.x_vel), (theBall.y_vel), 0.0);
		theBall.top += theBall.y_vel;
		theBall.bottom += theBall.y_vel;
		theBall.left += theBall.x_vel;
		theBall.right += theBall.x_vel;

		
		program.SetModelviewMatrix(BallMatrix);
		glBindTexture(GL_TEXTURE_2D, mytexture3);
		

		float ballvert[] = { -0.2f, -0.2f, 0.2f, -0.2f, 0.2f, 0.2f, -0.2, -0.2, 0.2, 0.2, -0.2, 0.2 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, ballvert);
		glEnableVertexAttribArray(program.positionAttribute);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, vcoord);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		//Go to next frame
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}


