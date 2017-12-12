

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
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>

using namespace std;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "main.h"



#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

enum GameMode {STATE_MAIN_MENU, STATE_MAP1, STATE_MAP2, STATE_MAP3};
GameMode mode;

enum EntityType{ENTITY_PLAYER, ENTITY_PLATFORM};

//Setup for map stuff
int mapWidth = -1;
int mapHeight = -1;
int** levelData;

float TILE_SIZE = 21.0/(692.0/8);
int SPRITE_COUNT_X = 30;
int SPRITE_COUNT_Y = 30;

vector<int> solidTiles{723, 721, 303, 336, 306, 2, 33};

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
	Entity() : should_render(true), faceleft(false), faceright(false) {}

	void Update(float elapsed);
	void Render(ShaderProgram &program);

	bool CollidesWithEntity(const Entity &entity);
	bool CollidesWithTile(float& distance);

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

	bool should_render;
	bool faceleft;
	bool faceright;

};


//Setup function
void Setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("SHOOT DUDES", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 576, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

	#ifdef _WINDOWS
		glewInit();
	#endif
	
	glViewport(0, 0, 1024, 576);
}

//Global Variables
Entity player1;
Entity player2;

//Make array of Entities for players
Entity playerList[] = { player1, player2 };

//Setup for some matrices
Matrix Player1_Matrix;
Matrix Player2_Matrix;
Matrix Map_Matrix;

vector<float> vertexData_Map;
vector<float> texCoordData_Map;
int mapSpriteCount = 0;

//Function to place the entity in the map
void placeEntity(string type, float X, float Y) {

	//if the entity type is that of a player, make the new entity a player
	if (type == "ENTITY_PLAYER1_START") {
		playerList[0].position.x = X;
		playerList[0].position.y = Y;
	}

	else if (type == "ENTITY_PLAYER2_START") {
		playerList[1].position.x = X;
		playerList[1].position.y = Y;
	}


}

//Function to read header of map file
bool readHeader(ifstream &stream) {
	string line;
	mapWidth = -1;
	mapHeight = -1;


	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "width") {
			mapWidth = atoi(value.c_str());
		}
		else if (key == "height") {
			mapHeight = atoi(value.c_str());
		}
	}
	if (mapWidth == -1 || mapHeight == -1) {
		return false;
	}
	else { // allocate our map data
		levelData = new int*[mapHeight];
		for (int i = 0; i < mapHeight; ++i) {
			levelData[i] = new int[mapWidth];
		}
		return true;
	}
}

//function to read layer data of map file
bool readLayerData(std::ifstream &stream) {
	string line;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				istringstream lineStream(line);
				string tile;
				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ',');
					int val = atoi(tile.c_str());
					if (val > 0) {
						// be careful, the tiles in this format are indexed from 1 not 0
						levelData[y][x] = val - 1;
					}
					else {
						levelData[y][x] = 0;
					}
				}
			}
		}
	}
	return true;
}

//function to read data for any entities in the map file
bool readEntityData(std::ifstream &stream) {
	string line;
	string type;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "type") {
			type = value;
		}
		else if (key == "location") {
			istringstream lineStream(value);
			string xPosition, yPosition;
			getline(lineStream, xPosition, ',');
			getline(lineStream, yPosition, ',');
			float placeX = atoi(xPosition.c_str())*TILE_SIZE;
			float placeY = atoi(yPosition.c_str())*-TILE_SIZE;
			placeEntity(type, placeX, placeY);
		}
	}
	return true;
}


//Function to draw the level with the build data?
void drawLevel(float* vertexData, float* texCoordData, ShaderProgram* program, int& spriteCount, GLuint textureID) {

	glBindTexture(GL_TEXTURE_2D, textureID);


	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData);
	glEnableVertexAttribArray(program->positionAttribute);


	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData);
	glEnableVertexAttribArray(program->texCoordAttribute);

	glDrawArrays(GL_TRIANGLES, 0, spriteCount * 6);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}


//Function to build level out of spritesheet
void buildLevel(vector<float>& vertexData, vector<float>& texCoordData, int& SpriteCount) {

		
	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {

			//Only add stuff if they're not empty tiles
			if (levelData[y][x] != 0) {

				//increment spritecount
				SpriteCount++;

				float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
				float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
				float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
				float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;
				vertexData.insert(vertexData.end(), {
					TILE_SIZE * x, -TILE_SIZE * y,
					TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					TILE_SIZE * x, -TILE_SIZE * y,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
				});
				texCoordData.insert(texCoordData.end(), {
					u, v,
					u, v + (spriteHeight),
					u + spriteWidth, v + (spriteHeight),
					u, v,
					u + spriteWidth, v + (spriteHeight),
					u + spriteWidth, v
				});
			}
		}
		}
	}


void loadMap(string mapname) {

	//Read the file for map 1 and set up its data?
	ifstream infile(mapname);
	string line;
	while (getline(infile, line)) {
		if (line == "[header]") {
			if (!readHeader(infile)) {
				assert(false);
			}
		}
		else if (line == "[layer]") {
			readLayerData(infile);
		}
		else if (line == "[Object Layer 1]") {
			readEntityData(infile);
		}
	}
}


int main(int argc, char *argv[])
{
	//set stuff up	
	Setup();
	
	 ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	//Sets a texture ID for the spritesheet
	GLuint spritesheet = LoadTexture(RESOURCE_FOLDER"sprites.png");
	GLuint fontsheet = LoadTexture(RESOURCE_FOLDER"font1.png");
	GLuint map_spritesheet = LoadTexture(RESOURCE_FOLDER"spritesheet.png");

	float last_frame_ticks = 0.0f;

	Matrix proMatrix;
	proMatrix.SetOrthoProjection(-5.33f, 5.33f, -3.0f, 3.0f, -1.0f, 1.0f);






	//Entities for bullets from players (one each)
	Entity p1Bullet;
	bool b1fired = false;

	Entity p2Bullet;
	bool b2fired = false;
	

	//Viewmatrix, or basically a camera at the moment.
	Matrix viewMatrix;


	SDL_Event event;
	bool done = false;
	
	//Things for fixed timestep
	#define FIXED_TIMESTEP 0.0166666f
	float accumulator = 0.0f;




	Sheetsprite player_ss(spritesheet, 391.0 / 1024.0f, 0.0f, 300.0 / 1024.0f, 199.0 / 512.0f, 0.2f);

	for (int i = 0; i < 2; i++) {
		
		playerList[i].sprite = player_ss;
		//Player size in x and y
		playerList[i].size.x = player_ss.width;
		playerList[i].size.y = player_ss.height;

		playerList[i].entitytype = ENTITY_PLAYER;

		//Set Player gravity

		playerList[i].acceleration.y = -1.8;
	}



	int gamestate = STATE_MAIN_MENU;


	//==============================Main Loop of the game============================================
	while (!done) {

		//each loop, reset Map's vertex/texture data
		vector<float> vertexData_Map;
		vector<float> texCoordData_Map;
		int mapSpriteCount = 0;


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

		
		

		//Render once
		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		switch (gamestate) {
			case STATE_MAIN_MENU:
				while (SDL_PollEvent(&event)) {
					if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
						done = true;
					}

				}
				if (keys[SDL_SCANCODE_1]) {
					gamestate = STATE_MAP1;
					loadMap("map1.txt");
					viewMatrix.Translate(-12 * TILE_SIZE, 12 * TILE_SIZE, 0.0);
				}
				
				if (keys[SDL_SCANCODE_2]) {
					gamestate = STATE_MAP2;
					loadMap("map2.txt");
					viewMatrix.Translate(-16 * TILE_SIZE, 19 * TILE_SIZE, 0.0);
				}
				glClearColor(0.6f, 0.2f, 0.3f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);
				break;
				
			default:

				glClearColor(0.6f, 0.4f, 0.3f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);

				//Update based on units of time passed
				while (elapsed >= FIXED_TIMESTEP) {
					for (int i = 0; i < 2; i++) {
						playerList[i].Update(FIXED_TIMESTEP);
					}
					elapsed -= FIXED_TIMESTEP;
				}
				accumulator = elapsed;

				//Check for various input events during the game
				while (SDL_PollEvent(&event)) {
					if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
						done = true;
					}

				}

				//Reset player's acceleration value every frame
				for (int i = 0; i < 2; i++) {
					playerList[i].acceleration.x = 0;
				}

				//Player1 controls
				if (keys[SDL_SCANCODE_A]) {

					playerList[0].acceleration.x -= (100 * FIXED_TIMESTEP);
					playerList[0].faceleft = true;
					playerList[0].faceright = false;

				}
				if (keys[SDL_SCANCODE_D]) {

					playerList[0].acceleration.x += (100 * FIXED_TIMESTEP);
					playerList[0].faceleft = false;
					playerList[0].faceright = true;

				}
				if (keys[SDL_SCANCODE_W] /*&& playerList[0].collidedBottom*/) {
					playerList[0].velocity.y = 5.0;
				}



				//Player2 controls
				if (keys[SDL_SCANCODE_LEFT]) {

					playerList[1].acceleration.x -= (100 * FIXED_TIMESTEP);
					playerList[1].faceleft = true;
					playerList[1].faceright = false;

				}
				if (keys[SDL_SCANCODE_RIGHT]) {

					playerList[1].acceleration.x += (100 * FIXED_TIMESTEP);
					playerList[1].faceleft = false;
					playerList[1].faceright = true;

				}
				if (keys[SDL_SCANCODE_UP] /*&& playerList[1].collidedBottom*/) {
					playerList[1].velocity.y = 5.0;
				}

				float distance;

				if (playerList[0].CollidesWithTile(distance)) {
					playerList[0].position.y += distance + 0.001;
				}


				//Adjust player position based on calculations
				Player1_Matrix.SetPosition(playerList[0].position.x, playerList[0].position.y, playerList[0].position.z);
				Player2_Matrix.SetPosition(playerList[1].position.x, playerList[1].position.y, playerList[1].position.z);

				program.SetModelviewMatrix(Player1_Matrix * viewMatrix);
				//program.SetModelviewMatrix(Player_Matrix);
				playerList[0].Render(program);

				program.SetModelviewMatrix(Player2_Matrix * viewMatrix);
				//program.SetModelviewMatrix(Player2_Matrix);
				playerList[1].Render(program);

				//Build and draw the level
				buildLevel(vertexData_Map, texCoordData_Map, mapSpriteCount);

				program.SetModelviewMatrix(Map_Matrix * viewMatrix);
				drawLevel(&vertexData_Map[0], &texCoordData_Map[0], &program, mapSpriteCount, map_spritesheet);
				break;

		}

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
bool Entity::CollidesWithEntity(const Entity& entity) {

	//Reset collisions at the beginning of each check
	bool collision = false;

	collidedLeft = false;
	collidedRight = false;
	collidedBottom = false;
	collidedTop = false;

	//calculate penetration on x
	if (!entity.should_render) return false;

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

					//If it's within the bounds of an entity, but not above the top of the entity, return that player is colliding and on the top
					if ((entity.position.x - entity.size.x / 2) >= this->position.x - this->size.x / 2 && this->position.x + this->size.x / 2 <= (entity.position.x + entity.size.x / 2)
						&& this->position.y + this->size.y / 2 < entity.position.y - entity.size.y / 2) {

						collision = true;
						this->collidedTop = true;
					}

				}
	
	return collision;
}

//Check collision with tilemap tiles
bool Entity::CollidesWithTile(float& distance) {


	int ent_convert_x = (this->position.x / TILE_SIZE);
	int ent_convert_y = -(this->position.y) / TILE_SIZE;

	if (ent_convert_x > 0 && ent_convert_y > 0) {
		if (ent_convert_x < mapWidth && ent_convert_y < mapHeight) {
			int tileID = levelData[ent_convert_y][ent_convert_x];

			bool collided = (find(solidTiles.begin(), solidTiles.end(), tileID) != solidTiles.end());

			if (collided) {
				distance = (-TILE_SIZE * (ent_convert_y)) - this->position.y;
			}
			return collided;
		}
	}


	return false;
}