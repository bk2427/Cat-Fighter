#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 26
#define ENEMY_COUNT 3
#define FIREBALL_COUNT 10

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"

/**
 STRUCTS AND ENUMS
 */
struct GameState
{
    Entity *player;
    Entity *platforms;
    Entity *enemies;
    Entity *bg;
    Entity *bullets;
    Entity* enemy_bullets;

    
    Mix_Music *bgm;
    Mix_Chunk *jump_sfx;
};

/**
 CONSTANTS
 */
const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

const float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const char SPRITESHEET_FILEPATH[] = "assets/cat_fighter_sprite1.png";
const char PLATFORM_FILEPATH[]    = "assets/stone.png";
const char BACKGROUND[] = "assets/Aibg.jpg";

const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL  = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER   = 0;   // this value MUST be zero

const float PLATFORM_OFFSET = 5.0f;

/**
 VARIABLES
 */
GameState state;

SDL_Window* display_window;
bool game_is_running = true;

ShaderProgram program;
glm::mat4 view_matrix, projection_matrix;

float previous_ticks = 0.0f;
float accumulator = 0.0f;
int shots_fired = 0;

/**
 GENERAL FUNCTIONS
 */
GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    // STEP 3: Setting our texture filter modes
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // STEP 4: Setting our texture wrapping modes
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // the last argument can change depending on what you are looking for
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // STEP 5: Releasing our file from memory and returning our texture id
    stbi_image_free(image);
    
    return textureID;
}

void DrawText(ShaderProgram *program, GLuint fontTextureId, std::string text, float size, float spacing, glm::vec3 position) {
    float width = 1.0f / 16.0f;
    float height = 1.0f / 16.0f;
    
    std::vector<float> vertices;
    std::vector<float> texCoords;
    
    for (int i = 0; i < text.size(); i++) {
        int index = (int)text[i];
        float offset = (size + spacing) * i;
        
        float u = (float)(index % 16) / 16.0f;
        float v = (float)(index / 16) / 16.0f;
        
        vertices.insert(vertices.end(), {
            offset + (-0.5f * size), 0.5f * size,
            offset + (-0.5f * size), -0.5f * size,
            offset + (0.5f * size), 0.5f * size,
            offset + (0.5f * size), -0.5f * size,
            offset + (0.5f * size), 0.5f * size,
            offset + (-0.5f * size), -0.5f * size,
        });
        
        texCoords.insert(texCoords.end(), {
            u, v,
            u, v + height,
            u + width, v,
            u + width, v + height,
            u + width, v,
            u, v + height,
        });
    }
    
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    program->SetModelMatrix(modelMatrix);
    
    glUseProgram(program->programID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices.data());
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords.data());
    glEnableVertexAttribArray(program->texCoordAttribute);

    glBindTexture(GL_TEXTURE_2D, fontTextureId);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));
    
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

bool areEnemiesActive(Entity *enemies) {
    for (int i = 0; i < ENEMY_COUNT; i++) {
        if (enemies[i].isActive == true) { return true;}
    }
    return false;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    display_window = SDL_CreateWindow("Hello, AI!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(display_window);
    SDL_GL_MakeCurrent(display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    program.Load(V_SHADER_PATH, F_SHADER_PATH);
    
    view_matrix = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.
    
    program.SetProjectionMatrix(projection_matrix);
    program.SetViewMatrix(view_matrix);
    
    glUseProgram(program.programID);
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    
//    background
    state.bg = new Entity();
    state.bg->position = glm::vec3(0.0f, 4.5f,1.0f);
    state.bg->movement= glm::vec3(0.0f);
    state.bg->textureID = load_texture(BACKGROUND);
    
    /**
     Platform stuff
     */
    GLuint platform_texture_id = load_texture(PLATFORM_FILEPATH);
    
    state.platforms = new Entity[PLATFORM_COUNT];
    
    for (int i = 0; i < 11; i++)
    {
        state.platforms[i].entityType = PLATFORM;
        state.platforms[i].textureID = platform_texture_id;
        state.platforms[i].position = glm::vec3(i - PLATFORM_OFFSET, -4.0f, 0.0f);
        state.platforms[i].Update(0, NULL, NULL, NULL, 0, 0, NULL);
    }
    for (int i = 11; i < 18; i++)
    {
        state.platforms[i].entityType = PLATFORM;
        state.platforms[i].textureID = platform_texture_id;
        state.platforms[i].position = glm::vec3((i-14) - PLATFORM_OFFSET, 1.5f, 0.0f);
        state.platforms[i].Update(0, NULL, NULL, NULL, 0, 0, NULL);

    }
    
    state.platforms[18].entityType = PLATFORM;
    state.platforms[18].textureID = platform_texture_id;
    state.platforms[18].position = glm::vec3((18-15) - PLATFORM_OFFSET, -2.6f, 0.0f);
    state.platforms[18].Update(0, NULL, NULL, NULL, 0, 0, NULL);

    state.platforms[19].entityType = PLATFORM;
    state.platforms[19].textureID = platform_texture_id;
    state.platforms[19].position = glm::vec3((18-14.5) - PLATFORM_OFFSET, 0.80f, 0.0f);
    state.platforms[19].Update(0, NULL, NULL, NULL, 0, 0, NULL);

    
    for (int i = 20; i < PLATFORM_COUNT; i++)
    {
        state.platforms[i].entityType = PLATFORM;
        state.platforms[i].textureID = platform_texture_id;
        state.platforms[i].position = glm::vec3((i-14.0) - PLATFORM_OFFSET, -0.8f, 0.0f);
        state.platforms[i].Update(0, NULL, NULL, NULL, 0, 0, NULL);
    }
    
    /**
     George's stuff
     */
    // Existing
    state.player = new Entity();
    state.player->entityType = PLAYER;
    state.player->position = glm::vec3(-4.5f, 6.0f, 0.0f);
    state.player->movement = glm::vec3(0);
    state.player->acceleration = glm::vec3(0, -9.81, 0);
    state.player->speed = 2.0f;
    state.player->textureID = load_texture(SPRITESHEET_FILEPATH);

    // Walking
    state.player->walking[state.player->LEFT]  = new int[4] { 1, 5, 9,  13 };
    state.player->walking[state.player->RIGHT] = new int[4] { 0, 3, 4, 3};
    state.player->walking[state.player->UP]    = new int[4] { 23, 24, 27, 28 };
    state.player->walking[state.player->DOWN]  = new int[8] { 12, 13, 14, 15};
    state.player->walking[state.player->JUMP]  = new int[4] { 0, 4, 8,  12 };

    state.player->animation_indices = state.player->walking[state.player->RIGHT];  // start George looking left
    state.player->animation_frames = 3;
    state.player->animation_index  = 0;
    state.player->animation_time   = 0.0f;
    state.player->animation_cols   = 10;
    state.player->animation_rows   = 10;
    state.player->height= 0.65f;
    state.player->width = 0.5f;
    
    // Jumping
    state.player->jumping_power = 5.5f;
    
    /**
     Enemies' stuff
     */
    GLuint yarn_id = load_texture("assets/yarn-removebg-preview.png");
    GLuint vacuum_id = load_texture("assets/vacuum-removebg-preview.png");
    
    
    state.enemies = new Entity[ENEMY_COUNT];
//    WALKING
    state.enemies[0].entityType = ENEMY;
    state.enemies[0].ai_type = GUARD;
    state.enemies[0].ai_state = IDLE;
    state.enemies[0].textureID = vacuum_id;
    state.enemies[0].position = glm::vec3(4.0f, -0.25f, 0.0f);
    state.enemies[0].movement = glm::vec3(0.0f);
    state.enemies[0].speed = 0.75f;
    state.enemies[0].acceleration = glm::vec3(0.0f, -9.81f, 0.0f);
    state.enemies[0].height= 0.5f;
    state.enemies[0].width = 0.5f;
    
    
    state.enemies[1].entityType = ENEMY;
    state.enemies[1].ai_type = JUMP;
    state.enemies[1].textureID = yarn_id;
    state.enemies[1].jump = true;
    state.enemies[1].jumping_power = 4.0f;
    state.enemies[1].position = glm::vec3(-2.5f, 3.0f, 0.0f);
    state.enemies[1].movement = glm::vec3(0.0f);
    state.enemies[1].speed = 0.0f;
    state.enemies[1].acceleration = glm::vec3(0.0f, -9.81f, 0.0f);
    
    
    state.enemies[2].entityType = ENEMY;
    state.enemies[2].ai_type = WALKER;
    state.enemies[2].textureID = vacuum_id;
    state.enemies[2].position = glm::vec3(-4.0f, -1.8f, 0.0f);
    state.enemies[2].movement = glm::vec3(0.5f);
    state.enemies[2].speed = 0.4f;
    state.enemies[2].acceleration = glm::vec3(0.0f, -9.81f, 0.0f);
    state.enemies[2].height= 0.5f;
    state.enemies[2].width = 0.5f;
    
    state.bullets = new Entity[FIREBALL_COUNT];
    for (int i = 0; i < FIREBALL_COUNT; i++)
    {
        state.bullets[i].textureID = yarn_id;
        state.bullets[i].entityType = FIREBALL;
//        state.bullets[i].isActive = false;
    }

//fireball stuff
//    state.player = new Entity();
//    state.player->entityType = PLAYER;
//    state.player->position = glm::vec3((player.position.x)f, 6.0f, 0.0f);
//    state.player->movement = glm::vec3(0);
//    state.player->acceleration = glm::vec3(0, 0, 0);
//    state.player->speed = 3.0f;
//    state.player->textureID = load_texture(SPRITESHEET_FILEPATH);
////    state.player->shooting  = new int[4] { 0, 4, 8,  12 };
//
//    state.player->animation_indices = state.player->walking[state.player->RIGHT];  // start George looking left
//    state.player->animation_frames = 3;
//    state.player->animation_index  = 0;
//    state.player->animation_time   = 0.0f;
//    state.player->animation_cols   = 10;
//    state.player->animation_rows   = 10;
//    state.player->height= 0.65f;
//    state.player->width = 0.5f;
    
    /**
     BGM and SFX
     */
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    
    state.bgm = Mix_LoadMUS("assets/mixkit-alter-ego-481.mp3");
    Mix_PlayMusic(state.bgm, -1);
    Mix_VolumeMusic(MIX_MAX_VOLUME / 6.0f);
    
    state.jump_sfx = Mix_LoadWAV("assets/mixkit-video-game-spin-jump-2648.wav");
    
    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    state.player->movement = glm::vec3(0);
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                game_is_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        game_is_running = false;
                        break;
                        
                    case SDLK_SPACE:
                        // Jump
                        if (state.player->collidedBottom)
                        {
                            state.player->jump = true;
                            Mix_PlayChannel(-1, state.jump_sfx, 0);
                        }
                        break;
                        
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    
    if (key_state[SDL_SCANCODE_SPACE])
    {
        state.player->movement.y = 1.0f;
        state.player->animation_indices = state.player->walking[state.player->UP];
    }
    else if (key_state[SDL_SCANCODE_RETURN])
    {
        state.player->movement.x = -0.0001f;
        state.player->animation_indices = state.player->walking[state.player->DOWN];
    }
    if (key_state[SDL_SCANCODE_LEFT])
    {
        state.player->movement.x = -1.0f;
        state.player->animation_indices = state.player->walking[state.player->LEFT];
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {
        state.player->movement.x = 1.0f;
        state.player->animation_indices = state.player->walking[state.player->RIGHT];
    }
    
    // This makes sure that the player can't move faster diagonally
    if (glm::length(state.player->movement) > 1.0f)
    {
        state.player->movement = glm::normalize(state.player->movement);
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - previous_ticks;
    previous_ticks = ticks;
    
    delta_time += accumulator;
    
    if (delta_time < FIXED_TIMESTEP)
    {
        accumulator = delta_time;
        return;
    }
    
    while (delta_time >= FIXED_TIMESTEP) {
        // Update. Notice it's FIXED_TIMESTEP. Not deltaTime
        state.player->Update(FIXED_TIMESTEP, state.player, state.platforms, state.enemies, PLATFORM_COUNT, ENEMY_COUNT, NULL);
        
        for (int i = 0; i < ENEMY_COUNT; i++) state.enemies[i].Update(FIXED_TIMESTEP, state.player, state.platforms, state.enemies, PLATFORM_COUNT, ENEMY_COUNT, state.enemy_bullets);
        for (int i = 0; i < FIREBALL_COUNT; i++) state.bullets[i].Update(FIXED_TIMESTEP, state.player, state.platforms, state.enemies, PLATFORM_COUNT, ENEMY_COUNT, NULL);


        delta_time -= FIXED_TIMESTEP;
    }
    
    accumulator = delta_time;
    
    
    if (state.enemies[1].collidedBottom)
    {
        state.enemies[1].jump = true;
    }

}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    
    state.bg->renderbg(&program);
    
    for (int i = 0; i < PLATFORM_COUNT; i++) state.platforms[i].render(&program);
    state.player->render(&program);
    for (int i = 0; i < ENEMY_COUNT; i++) state.enemies[i].render(&program);
    if(state.player->isActive && areEnemiesActive(state.enemies) == false) {
            DrawText(&program, load_texture("assets/font1.png"), "You Win!", 0.5, -0.05, glm::vec3(-1.75, 2, 0));
            
        }
    else if (state.player->isActive ==  false && areEnemiesActive(state.enemies)) {
            DrawText(&program, load_texture("assets/font1.png"), "You Lose...", 0.5, -0.05, glm::vec3(-2, 2, 0));
        }
        
    
    SDL_GL_SwapWindow(display_window);
}

void shutdown()
{    
    SDL_Quit();
    
    delete [] state.platforms;
    delete [] state.enemies;
    delete    state.player;
    Mix_FreeChunk(state.jump_sfx);
    Mix_FreeMusic(state.bgm);
}

/**
 DRIVER GAME LOOP
 */
int main(int argc, char* argv[])
{
    initialise();
    
    while (game_is_running)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
