//Yes I know this is kind of ridiculously long, but I had only learned more proper optimization 
//in the middle of this assignment, at which point most of the game logic had already been made
//For future projects, should probably keep things like Gamestates, more setup and creation functions and whatnot


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
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

enum GameMode {STATE_MAIN_MENU, STATE_GAME, STATE_WIN};
GameMode mode;


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

//Function to draw text from the fontsheet
void DrawText(ShaderProgram* program, int fontTexture, std::string text, float size, float spacing) {

	float textureSize = 1.0 / 16.0;
	
	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	//For every letter in the phrase
	for (int i = 0; i < text.size(); i++) {

		//Grab sprite index by getting ASCII value of letter
		int spriteIndex = (int)text[i];

		//Get x/y coords through these calculations
		float texture_x = (float)((spriteIndex % 16) / 16.0f);
		float texture_y = (float)((spriteIndex / 16) / 16.0f);
		
		//Put in data for the letter's ultimate position in the matrix in a vector
		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size
		});

		//Put the info for the letter's texture coordinates into a vector
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + textureSize,
			texture_x + textureSize, texture_y,
			texture_x + textureSize, texture_y + textureSize,
			texture_x + textureSize, texture_y,
			texture_x, texture_y + textureSize 

		});

	}

	//Draw the phrase by making as many triangles as needed
	glUseProgram(program->programID);
	glBindTexture(GL_TEXTURE_2D, fontTexture);

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);


	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, text.size()*6);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);

}

//Player Entity class
class Player{
	public:

		Player(float l, float r, float t, float b) :left(l), right(r), top(t), bottom(b) {}

		float left;
		float right;
		float top;
		float bottom;
		float P1_verts[12] = { 0.8f, 0.15f, -0.8f, -0.15f, 0.8f, -0.15f,  -0.8f, -0.15f, 0.8f, 0.15f, -0.8f, 0.15f };
};

//Entity class for enemies
class Enemy {
public:
	Enemy() {}
	Enemy(float l, float r, float t, float b) :left(l), right(r), top(t), bottom(b) {}

	float left = -0.2;
	float right = 0.2;
	float top = 0.3;
	float bottom = -0.2;

	bool alive = true;

	//Have modelview matrix for each enemy
	Matrix enemyMatrix;


	float e_verts[12] = { 0.2f, 0.3f, -0.2f, -0.2f,   0.2f, -0.2f, -0.2f, -0.2f, 0.2f, 0.3f, -0.2f, 0.3f };

	void kill_enemy() {
		alive = false;
		enemyMatrix.Translate(0, 10, 0);
	}
};


class Bullet {
	public:

		Bullet(){}


		float left = -0.1;
		float right = 0.1;
		float top = 0.1;
		float bottom = -0.1;
		float Bullet_verts[12] = { 0.1f, 0.1f, -0.1f, -0.1f, 0.1f, -0.1f, -0.1f, -0.1f, 0.1f, 0.1f, -0.1f, 0.1f };
		float bvcoord[12] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };

		bool fired = false;

		int bulletno;

		//Have a matrix for each bullet entity
		Matrix bulletMatrix;

		void reset_bullet() {
			
			bulletMatrix.Identity();

			left = -0.1;
			right = 0.1;
			top = 0.1;
			bottom = -0.1;
			fired = false;
		}

		void fire_bullet(float pleft, float pright, float ptop) {

			//set bullet in front of player
			bulletMatrix.Identity();
			bulletMatrix.SetPosition((pleft + pright) / 2, ptop + 0.2, 0);
			
			left = (pleft + pright) / 2 - 0.2;
			right = (pleft + pright) / 2 + 0.2;
			bottom = ptop;
			top = ptop + 0.2;
			fired = true;

		}

		//Function in each enemy to check for bullet collision through box-box
		bool check_if_shot(Enemy& e) {
			//if any of these are true, then there's no collision with a bullet
			if (e.bottom > this->top || e.top < this->bottom || e.left > this->right || e.right < this->left) {
				return false;
			}

			return true;
		}

};


class Sheetsprite {
	public:
		Sheetsprite() {}
		Sheetsprite(unsigned int tID, float u, float v, float w, float h, float size) : size(size), textureID(tID), u(u), v(v), width(w), height(h) {}

		//Function to set up/draw the sprite
		void Draw(ShaderProgram* program, float vertices[12]) {
			glBindTexture(GL_TEXTURE_2D, textureID);

			GLfloat texCoords[] = {
				u, v + height,
				u + width, v,
				u, v,
				u + width, v,
				u, v + height,
				u + width, v + height
			};
			
			//Aspect ratio of the sprite
			float aspect = height / width;

			//Draw the sprite

			glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
			glEnableVertexAttribArray(program->positionAttribute);

			
			glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
			glEnableVertexAttribArray(program->texCoordAttribute);

			glDrawArrays(GL_TRIANGLES, 0, 6);

			glDisableVertexAttribArray(program->positionAttribute);
			glDisableVertexAttribArray(program->texCoordAttribute);


		}


		float size;
		unsigned int textureID;
		float u;
		float v;
		float width;
		float height;
};


//Setup function
void Setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("CORN INVADERS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 576, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

	#ifdef _WINDOWS
		glewInit();
	#endif
	
	glViewport(0, 0, 1024, 576);
}

//Might want to make a function for general sprite drawing if possible? Will come back to this if I have time I guess.


int main(int argc, char *argv[])
{
	//set stuff up	
	Setup();
	
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	//Sets a texture ID for the spritesheet
	GLuint spritesheet = LoadTexture(RESOURCE_FOLDER"sprites.png")	;
	GLuint fontsheet = LoadTexture(RESOURCE_FOLDER"font1.png");

	float last_frame_ticks = 0.0f;

	Matrix proMatrix;
	proMatrix.SetOrthoProjection(-5.33f, 5.33f, -3.0f, 3.0f, -1.0f, 1.0f);

	//Matrix for the main menu
	Matrix Menu_Matrix;
	Menu_Matrix.Translate(-2.0, 0, 0);
	//Matrix for subtitle on main menu
	Matrix Subtitle_Matrix;
	Subtitle_Matrix.Translate(-2.0, -0.7, 0);

	//Matrix for the player
	Matrix Player_Matrix;
	//Shift it down from the center
	Player_Matrix.Translate(0, -2.5f, 0);

	//Stuff for bullets, including an array of 30 bullets
	int bulletLimit = 10;
	Bullet bulletArray[10];


	int currentBullet = 0;
	for (Bullet& x : bulletArray) {
		x.bulletno = currentBullet;
		currentBullet++;
	}
	currentBullet = 0;
	
	
	//test bullet
	//Bullet x;

	//Stuff for enemies
	Enemy enemyArray[10];
	float x_shift = 0.0;
	float y_shift = 3.0;
		//Initial factor by which enemies move left and right
	float enemy_dir_mult = 0.5;

	//Set up a row of enemies (for now) by spacing each enemy in the row apart
	//Rows should be 5 enemies apart, so additional enemies are shifted down a bit
	for (Enemy& e : enemyArray) {

		if ((int)x_shift % 5 == 0) {
			y_shift -= 1.0;
			x_shift = 0;
		}

		e.enemyMatrix.Translate(x_shift, y_shift, 0);
		e.left += x_shift;
		e.right += x_shift;
		e.top += y_shift;
		e.bottom += y_shift;
		x_shift++;
		
	}

	//Var to keep track of player score
	int score = 0;
	//Matrix for the score
	Matrix score_txt_matrix;

	Player player(-0.8, 0.8, -2.3, -2.5 );

	SDL_Event event;
	bool done = false;
	
	Sheetsprite player_ss(spritesheet, 391.0/1024.0f, 201.0/512.0f, 70.0/1024.0f, 105.0/512.0f, 0.7f);
	Sheetsprite enemy_ss(spritesheet, 0.0f, 0.0f, 389.0/1024.0f, 500.0/512.0f, 0.8f);
	Sheetsprite bullet_ss(spritesheet, 391.0/1024.0f, 0.0f, 300.0/1024.0f, 199.0/512.0f, 0.2f);

	mode = STATE_MAIN_MENU;

	//==============================Main Loop of the game============================================
	while (!done) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//Elapsed time tracker
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - last_frame_ticks;
		last_frame_ticks = ticks;

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		const Uint8 *keys = SDL_GetKeyboardState(NULL);


		//A little messy, but didn't have time to implement overarching render functions I think
		//PROBLEM: Things don't render in the main menu
		switch (mode) {

			//In the main menu
			case STATE_MAIN_MENU:
				

				glClearColor(0.4f, 0.6f, 0.3f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);



				while (SDL_PollEvent(&event)) {
					if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
						done = true;
					}
					//If player presses spacebar on main menu, go into game
					else if (event.type == SDL_KEYDOWN) {
						//if spacebar is pressed and while the current bullet hasn't been fired yet, set position of bullet to start at current player position
						if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
							mode = STATE_GAME;
						}
					}
				}

				glUseProgram(program.programID);
				program.SetProjectionMatrix(proMatrix);

				glLoadIdentity();

				program.SetModelviewMatrix(Menu_Matrix);

				DrawText(&program, fontsheet, "CORN INVADERS", 0.4f, 0.0001f);

				program.SetModelviewMatrix(Subtitle_Matrix);
				DrawText(&program, fontsheet, "Press Spacebar to start", 0.3, 0.0f);
				


				break;


			//in the game
			case STATE_GAME:

				glClearColor(0.2f, 0.4f, 0.3f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);


				//Check for various input events during the game
				while (SDL_PollEvent(&event)) {
					if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
						done = true;
					}
					else if (event.type == SDL_KEYDOWN) {
						//if spacebar is pressed and while the current bullet hasn't been fired yet, set position of bullet to start at current player position
						if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {

							int duds = 0;
							//Go to next available bullet
							while (bulletArray[currentBullet].fired) {
								++duds;
								++currentBullet;

								//reset current bullet to the first one if it's reached the end
								if (currentBullet >= bulletLimit) {
									currentBullet = 0;
								}

								//If no bullets available right now, continue on with render until next time spacebar is pressed
								if (duds >= bulletLimit) {
									break;
								}

							}



							//Fire bullet if it hasn't been fired yet (Unlikely given previous loop, but just a check frankly)
							if (!(bulletArray[currentBullet].fired)) {

								bulletArray[currentBullet].fire_bullet(player.left, player.right, player.top);
								++currentBullet;

								if (currentBullet >= bulletLimit) {
									currentBullet = 0;
								}
							}


						}

					}
				}


				//Check/Facilitate player movement left and right
				if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
					if (player.left > -5.33) {


						player.left -= (2.5 * elapsed);
						player.right -= (2.5 * elapsed);
						Player_Matrix.Translate(-(2.5 * elapsed), 0.0, 0.0);


					}

				}
				if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
					if (player.right < 5.33) {


						player.left += (2.5 * elapsed);
						player.right += (2.5 * elapsed);
						Player_Matrix.Translate((2.5 * elapsed), 0.0, 0.0);


					}
				}


				glUseProgram(program.programID);
				program.SetProjectionMatrix(proMatrix);



				//Draw/Update player every frame
				program.SetModelviewMatrix(Player_Matrix);
				player_ss.Draw(&program, player.P1_verts);


				for (Enemy& e : enemyArray) {

					//If one enemy touches the side of the screen, send all of them in the opposite direction
					if (e.right > 5.33  && e.alive) {
						enemy_dir_mult = -0.5;
					}
					else if (e.left < -5.33 && e.alive) {
						enemy_dir_mult = 0.5;

					}

					e.enemyMatrix.Translate(enemy_dir_mult * elapsed, 0, 0);
					e.left += enemy_dir_mult * elapsed;
					e.right += enemy_dir_mult * elapsed;

					//Draw enemy if they are still alive
					program.SetModelviewMatrix(e.enemyMatrix);

					enemy_ss.Draw(&program, e.e_verts);

				}

				

				//Draw every bullet each frame
				//Do so by passing bullets by reference
				for (Bullet& b : bulletArray) {
					//if a bullet is marked as fired, move it upward and reflect that in its movement data
					if (b.fired) {
						//if a bullet has gone offscreen set it back to its original position and set it back to not fired
						if (b.bottom >= 3.0) {
							b.reset_bullet();

						}
						else {
							b.top += (1.4 * elapsed);
							b.bottom += (1.4 * elapsed);
							b.bulletMatrix.Translate(0, 1.4 * elapsed, 0);

							//If the bullet has hit an enemy and if the enemy is actually alive, kill the enemy and reset the bullet
							for (Enemy& e : enemyArray) {
								if (b.check_if_shot(e) && e.alive) {
									//Add one to score
									score += 1;
									//destroy bullet and hopefully stop drawing enemy
									b.reset_bullet();
									e.kill_enemy();

									currentBullet = b.bulletno;

									if (score == 10) {
										mode = STATE_WIN;	
									}
								}
							}


							//set modelview matrix to the current bullet's, bind texture to it, draw all of them
							program.SetModelviewMatrix(b.bulletMatrix);
							bullet_ss.Draw(&program, b.Bullet_verts);

						}

					}

				}
				
				score_txt_matrix.Identity();
				score_txt_matrix.Translate(3.0, 2.8, 0.0);
				program.SetModelviewMatrix(score_txt_matrix);
				DrawText(&program, fontsheet, "SCORE: " + std::to_string(score), 0.2, 0.0);


				break;

			case STATE_WIN:

				glClearColor(0.4f, 0.6f, 0.3f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);



				while (SDL_PollEvent(&event)) {
					if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
						done = true;
					}
					//If player presses spacebar on main menu, go into game
					else if (event.type == SDL_KEYDOWN) {
						//if spacebar is pressed and while the current bullet hasn't been fired yet, set position of bullet to start at current player position
						if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
							done = true;
						}
					}
				}

				//Use same program and promatrix
				glUseProgram(program.programID);
				program.SetProjectionMatrix(proMatrix);

				glLoadIdentity();

				//Display win message and quit prompt
				program.SetModelviewMatrix(Menu_Matrix);

				DrawText(&program, fontsheet, "CORNGRATURBLATON!", 0.4f, 0.0001f);

				program.SetModelviewMatrix(Subtitle_Matrix);
				DrawText(&program, fontsheet, "Press Spacebar to quit", 0.3, 0.0f);
		}
		

		//Go to next frame
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}


