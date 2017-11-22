

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
#include <SDL_mixer.h>

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

enum EntityType{ENTITY_PLAYER, ENTITY_COIN, ENTITY_PLATFORM};

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

class Vector3 {
	public:
		Vector3() : x(0.0f), y(0.0f), z(0.0f) { 
		}

		float x;
		float y;
		float z;

};

class Sheetsprite {
	public:
		Sheetsprite():worldwidth(0.0), worldheight(0.0){}
		Sheetsprite(unsigned int tID, float u, float v, float w, float h, float size) : size(size), textureID(tID), u(u), v(v), width(w), height(h), worldwidth(size*h/w), worldheight(size) {}


		//Function to set up/draw the sprite
		void Draw(ShaderProgram* program) {
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


			float vertices[] = {
				-0.5f * size * aspect, -0.5 * size,
				0.5 * size * aspect, 0.5f * size,
				-0.5f * size * aspect, 0.5f * size,
				0.5f * size * aspect, 0.5f * size,
				-0.5f * size * aspect, -0.5f * size,
				0.5f * size * aspect, -0.5f * size
			};

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

		float worldwidth;
		float worldheight;
};

class Entity {
public:
	Entity() : render(true) {}

	void Update(float elapsed);
	void Render(ShaderProgram &program);

	bool CollidesWith(const Entity &entity);

	Sheetsprite sprite;

	Vector3 position;
	Vector3 size;
	Vector3 velocity;
	Vector3 acceleration;

	bool isStatic;
	EntityType entitytype;

	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;

	bool render;


};


//Setup function
void Setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("PLATFORME", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 576, SDL_WINDOW_OPENGL);
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
	
	//Initialize audio mixer and sounds
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

	Mix_Chunk *JumpSound;
	JumpSound = Mix_LoadWAV("jump.wav");
	Mix_Chunk *CoinSound;
	CoinSound = Mix_LoadWAV("Pickup_Coin.wav");


	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	//Sets a texture ID for the spritesheet
	GLuint spritesheet = LoadTexture(RESOURCE_FOLDER"sprites.png");
	GLuint fontsheet = LoadTexture(RESOURCE_FOLDER"font1.png");

	float last_frame_ticks = 0.0f;

	Matrix proMatrix;
	proMatrix.SetOrthoProjection(-5.33f, 5.33f, -3.0f, 3.0f, -1.0f, 1.0f);


	Entity player;
	Entity platform;
	Entity platform2;
	Entity platform3;
	Entity coin1;

	//Viewmatrix to translate everything by to simulate scrolling?
	Matrix viewMatrix;

	SDL_Event event;
	bool done = false;
	
	//Things for fixed timestep
	#define FIXED_TIMESTEP 0.0166666f
	float accumulator = 0.0f;


	//Setup for the player
	Matrix Player_Matrix;
	Sheetsprite player_ss(spritesheet, 391.0 / 1024.0f, 0.0f, 300.0 / 1024.0f, 199.0 / 512.0f, 0.2f);
	player.sprite = player_ss;
	//Player size in x and y
	player.size.x = player_ss.width;
	player.size.y = player_ss.height;

	player.entitytype = ENTITY_PLAYER;

	//Setup for a test platform
	Matrix Platform_Matrix_1;
	Sheetsprite platform_ss(spritesheet, 391.0 / 1024.0f, 201.0 / 512.0f, 70.0 / 1024.0f, 105.0 / 512.0f, 0.5f);
	platform.sprite = platform_ss;
	platform.isStatic = true;
	platform.size.x = platform_ss.worldwidth;
	platform.size.y = platform_ss.height;
	
	//Set platform down a little
	Platform_Matrix_1.Translate(0.0f, -2.0f, 0.0f);
	platform.position.y = -2.0f;
	platform.position.x = 0.0f;

	platform.entitytype = ENTITY_PLATFORM;


	//Setup for a test platform
	Matrix Platform_Matrix_2;
	platform2.sprite = platform_ss;
	platform2.isStatic = true;
	platform2.size.x = platform_ss.worldwidth;
	platform2.size.y = platform_ss.height;

	//Set platform position
	Platform_Matrix_2.Translate(-2.0f, 0.0f, 0.0f);
	platform2.position.y = 0.0f;
	platform2.position.x = -2.0f;


	platform2.entitytype = ENTITY_PLATFORM;


	//Setup for a test platform
	Matrix Platform_Matrix_3;
	platform3.sprite = platform_ss;
	platform3.isStatic = true;
	platform3.size.x = platform_ss.worldwidth;
	platform3.size.y = platform_ss.height;

	//Set platform position
	Platform_Matrix_3.Translate(+3.0f, -1.0f, 0.0f);
	platform3.position.y = -1.0f;
	platform3.position.x = +3.0f;


	platform3.entitytype = ENTITY_PLATFORM;



	Matrix Coin_Matrix_1;
	Sheetsprite coin_ss(spritesheet, 0.0f, 0.0f, 389.0 / 1024.0f, 500.0 / 512.0f, 0.2f);

	coin1.sprite = coin_ss;
	coin1.isStatic = false;
	coin1.size.x = coin_ss.worldwidth;
	coin1.size.y = coin_ss.worldheight;
	
	//set position of a coin
	Coin_Matrix_1.Translate(0.0f, 0.3f, 0.0f);
	coin1.position.y = 0.3f;
	coin1.entitytype = ENTITY_COIN;




	//Set Player gravity

	player.acceleration.y = -1.8;

	//==============================Main Loop of the game============================================
	while (!done) {


		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//Use program and projection matrix
		glUseProgram(program.programID);
		program.SetProjectionMatrix(proMatrix);


		//Get elapsed time
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - last_frame_ticks;
		last_frame_ticks = ticks;

		//Adjust elapsed with accumulated time
		elapsed += accumulator;

		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
		}

		//Update based on units of time passed
		while (elapsed >= FIXED_TIMESTEP) {
			player.Update(FIXED_TIMESTEP);
			elapsed -= FIXED_TIMESTEP;
		}
		accumulator = elapsed;
		//Render once


		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		glClearColor(0.6f, 0.4f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);


		//Check for various input events during the game
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			//If spacebar is pressed (Later add check for collision on bottom), "Jump" by directly setting y velocity
			else if (event.type == SDL_KEYDOWN) {

			}
		}
		
		//Reset player's acceleration value every frame
		player.acceleration.x = 0;

		if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
			
			player.acceleration.x -= (100 * FIXED_TIMESTEP);
				

		}

		if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {

			player.acceleration.x += (100 * FIXED_TIMESTEP);


		}
		if (keys[SDL_SCANCODE_SPACE] && player.collidedBottom) {
			player.velocity.y = 5.0;
			Mix_PlayChannel(-1, JumpSound, 0);
		}


		//Translate View Matrix by opposite of player movement	
		viewMatrix.Translate(-player.velocity.x * FIXED_TIMESTEP, 0.0f, 0.0f);
		if (!player.collidedBottom) {
			viewMatrix.Translate(0.0f, -player.velocity.y * FIXED_TIMESTEP, 0.0f);
		}

		//Render all entities (Create array/vector for these if there's time)
		program.SetModelviewMatrix(Platform_Matrix_1 * viewMatrix);
		platform.Render(program);
		program.SetModelviewMatrix(Platform_Matrix_2 * viewMatrix);
		platform2.Render(program);
		program.SetModelviewMatrix(Platform_Matrix_3 * viewMatrix);
		platform3.Render(program);



		//If player is collided with a platform in a particular direction, shift it by some amount
		if (player.CollidesWith(platform) ) {
			if (player.collidedBottom){
				player.position.y = platform.position.y + platform.size.y / 2 + player.size.y / 2;
			}
			else if (player.collidedLeft) {
				player.position.x = platform.position.x + platform.size.x / 2 + player.size.x / 2;
			}
			else if (player.collidedRight) {
				player.position.x = platform.position.x - platform.size.x / 2 - player.size.x/2;
			}
			else if (player.collidedTop) {
				player.position.y = platform.position.y - platform.size.y / 2 - player.size.y/2;
			}
		}
		else if (player.CollidesWith(platform2)) {
			if (player.collidedBottom) {
				player.position.y = platform2.position.y + platform2.size.y / 2 + player.size.y / 2;
			}
			else if (player.collidedLeft) {
				player.position.x = platform2.position.x + platform2.size.x / 2 + player.size.x / 2;
			}
			else if (player.collidedRight) {
				player.position.x = platform2.position.x - platform2.size.x / 2 - player.size.x / 2;
			}
			else if (player.collidedTop) {
				player.position.y = platform2.position.y - platform2.size.y / 2 - player.size.y / 2;
			}
		}
		
		else if (player.CollidesWith(platform3)) {
			if (player.collidedBottom) {
				player.position.y = platform3.position.y + platform3.size.y / 2 + player.size.y / 2;
			}
			else if (player.collidedLeft) {
				player.position.x = platform3.position.x + platform3.size.x / 2 + player.size.x / 2;
			}
			else if (player.collidedRight) {
				player.position.x = platform3.position.x - platform3.size.x / 2 - player.size.x / 2;
			}
			else if (player.collidedTop) {
				player.position.y = platform3.position.y - platform3.size.y / 2 - player.size.y / 2;
			}
		}

		//if player has collided with a coin entity, stop rendering it
		else if (player.CollidesWith(coin1) && coin1.render) {
			Mix_PlayChannel(-1, CoinSound, 0);
			coin1.render = false;
		}


		//Only render coins if they haven't been collided into yet
		if (coin1.render) {
			program.SetModelviewMatrix(Coin_Matrix_1 * viewMatrix);
			coin1.Render(program);
		}


		//Adjust player position based on calculations
		Player_Matrix.SetPosition(player.position.x, player.position.y, player.position.z);


		program.SetModelviewMatrix(Player_Matrix * viewMatrix);
		player.Render(program);


		

		//Go to next frame
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}

float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t*v1;
}

void Entity::Update(float elapsed) {
	
	//Update moving objects' positions using the elapsed time
	float friction_x = 2.0;
	float friction_y = 1.0;

	//Apply Friction
	Entity::velocity.x = lerp(Entity::velocity.x, 0.0f, elapsed * friction_x);
	Entity::velocity.y = lerp(Entity::velocity.y, 0.0f, elapsed * friction_y);


	//Update the entity's velocity with the vector accelerations
	Entity::velocity.x += Entity::acceleration.x * elapsed;
	Entity::velocity.y += Entity::acceleration.y * elapsed;


	//Update the entity's position with the velocities
	Entity::position.x += Entity::velocity.x * elapsed;
	Entity::position.y += Entity::velocity.y * elapsed;

	

}

void Entity::Render(ShaderProgram& program) {
	this->sprite.Draw(&program);
}

//Check collision with entities
bool Entity::CollidesWith(const Entity& entity) {

	//Reset collisions at the beginning of each check
	bool collision = false;

	collidedLeft = false;
	collidedRight = false;
	collidedBottom = false;
	collidedTop = false;

	//calculate penetration on x
	if (!entity.render) return false;

	float x_penetration = fabs((this->position.x - entity.position.x) - (this->position.x) / 2 - (entity.position.x) / 2);
	
	//Calculate penetration on y and check collision through that?
	float y_penetration = fabs((this->position.y - entity.position.y) - (this->position.y) / 2 - (entity.position.y) / 2);


				//If it would go through the top of an entity
				if ((this->position.y - this->size.y / 2) <= (entity.position.y + entity.size.y / 2)) {

					//If it's within the bounds of a platform, but not below the bottom of the platform, return that player is colliding and on the bottom
					if ((entity.position.x - entity.size.x / 2) <= this->position.x - this->size.x/2 && this->position.x + this->size.x/2 <= (entity.position.x + entity.size.x / 2)
						&& this->position.y + this->size.y / 2 > entity.position.y - entity.size.y / 2) {

						collision = true;
						this->collidedBottom = true;
					}
				}
				
				//if right side collides with left of platform, and y position is also between the platform's top and bottom, it's colliding on the right
				else if ((this->position.x) + this->size.x/2 > entity.position.x + entity.size.x/2 && this->position.y < entity.position.y + 
					entity.size.y/2 && this->position.y > entity.position.y - entity.position.y/2) {

					collision = true;
					this->collidedRight = true;
				}
				
				//if left side collides with right of platform, and y position is also between the platform's top and bottom, it's colliding on the right
				else if ((this->position.x) - this->size.x / 2 < entity.position.x + entity.size.x / 2 && this->position.y < entity.position.y +
					entity.size.y / 2 && this->position.y > entity.position.y - entity.position.y / 2) {

					collision = true;
					this->collidedLeft = true;
				}
				
				//If it would go through the bottom of a platform
				else if ((this->position.y + this->size.y / 2) >= (entity.position.y - entity.size.y / 2)) {

					//If it's within the bounds of a platform, but not above the top of the platform, return that player is colliding and on the top
					if ((entity.position.x - entity.size.x / 2) >= this->position.x - this->size.x / 2 && this->position.x + this->size.x / 2 <= (entity.position.x + entity.size.x / 2)
						&& this->position.y + this->size.y / 2 < entity.position.y - entity.size.y / 2) {

						collision = true;
						this->collidedTop = true;
					}

				}
	
	return collision;
}


