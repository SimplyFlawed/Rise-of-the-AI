#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define ENEMY_COUNT 3
#define LEVEL1_WIDTH 23
#define LEVEL1_HEIGHT 14

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
#include "Map.h"

// ————— GAME STATE ————— //
struct GameState
{
    Entity* player;
    Entity* enemies;

    Map* map;

    Mix_Music* bgm;
    Mix_Chunk* jump_sfx;
    Mix_Chunk* pop_sfx;
};

// ————— CONSTANTS ————— //
const int   WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

const float BG_RED = 194.0f/255.0f,
BG_GREEN = 227.0f/255.0f,
BG_BLUE = 232.0f/255.0f,
BG_OPACITY = 1.0f;

const int   VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char GAME_WINDOW_NAME[] = "Rise of the AI";

const char  V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;

const char  PLAYER_SPRITESHEET_FILEPATH[]       = "assets/images/tilemap-player.png",
            BUG_SPRITESHEET_FILEPATH[]          = "assets/images/tilemap-bug.png",
            ENEMY_SPRITESHEET_FILEPATH[]        = "assets/images/tilemap-characters_packed.png",
            MAP_TILESET_FILEPATH[]              = "assets/images/tilemap_packed.png",
            BGM_FILEPATH[]                      = "assets/audio/dooblydoo.wav",
            JUMP_SFX_FILEPATH[]                 = "assets/audio/jump1.wav",
            POP_SFX_FILEPATH[]                  = "assets/audio/bubble1.wav";

const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL = 0;
const GLint TEXTURE_BORDER = 0;


unsigned int LEVEL_1_DATA[] =
{
    122, 122, 122, 122, 122, 122, 122, 122, 123, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0,
    122, 122, 122, 122, 122, 122, 122, 122, 123, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 69, 0, 0, 0,
    122, 122, 122, 122, 122, 4, 142, 142, 143, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 89, 0, 0, 0,
    122, 122, 122, 122, 122, 123, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 89, 0, 0, 0,
    122, 122, 122, 122, 122, 123, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 109, 0, 0, 0,
    4, 142, 142, 142, 142, 143, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 48, 49, 49, 49, 50, 0,
    123, 0, 0, 0, 69, 0, 0, 0, 0, 0, 0, 124, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    123, 0, 0, 0, 89, 0, 0, 0, 0, 0, 21, 22, 22, 23, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    123, 0, 0, 0, 89, 0, 0, 0, 125, 0, 121, 122, 122, 123, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    123, 0, 0, 0, 109, 0, 0, 21, 22, 22, 25, 122, 122, 123, 0, 0, 0, 0, 0, 21, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 25, 122, 122, 122, 122, 122, 24, 22, 22, 22, 22, 22, 25, 122, 122, 122,
    22, 22, 23, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 21, 22, 23, 122, 122, 122,
    122, 122, 123, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 121, 122, 123, 122, 122, 122,
    122, 122, 123, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 121, 122, 123, 122, 122, 122
};

// ————— VARIABLES ————— //
GameState g_game_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float   g_previous_ticks = 0.0f,
g_accumulator = 0.0f;

// ————— GENERAL FUNCTIONS ————— //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint texture_id;
    glGenTextures(NUMBER_OF_TEXTURES, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return texture_id;
}

void initialise()
{
    // ————— GENERAL ————— //
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow(GAME_WINDOW_NAME,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    // ————— VIDEO SETUP ————— //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);

    // ————— MAP SET-UP ————— //
    GLuint map_texture_id = load_texture(MAP_TILESET_FILEPATH);
    g_game_state.map = new Map(LEVEL1_WIDTH, LEVEL1_HEIGHT, LEVEL_1_DATA, map_texture_id, 1.0f, 20, 9);

    // ————— PLAYER SET-UP ————— //
    // Existing
    g_game_state.player = new Entity();
    g_game_state.player->set_entity_type(PLAYER);
    g_game_state.player->set_position(glm::vec3(1.0f, -8.0f, 0.0f));
    g_game_state.player->set_movement(glm::vec3(0.0f));
    g_game_state.player->set_speed(2.5f);
    g_game_state.player->set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));
    g_game_state.player->m_texture_id = load_texture(PLAYER_SPRITESHEET_FILEPATH);

    // Animation
    g_game_state.player->m_walking[g_game_state.player->RIGHT] = new int[2] { 0, 1 };
    g_game_state.player->m_walking[g_game_state.player->LEFT] = new int[2] { 2, 3 };

    g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->RIGHT];  
    g_game_state.player->m_animation_frames = 2;
    g_game_state.player->m_animation_index = 0;
    g_game_state.player->m_animation_time = 0.0f;
    g_game_state.player->m_animation_cols = 4;
    g_game_state.player->m_animation_rows = 1;
    g_game_state.player->set_height(0.9f);
    g_game_state.player->set_width(0.9f);

    // Jumping
    g_game_state.player->m_jumping_power = 8.0f;

    // ————— ENEMY SET-UP ————— //
    //BUG
    g_game_state.enemies = new Entity[ENEMY_COUNT];

    g_game_state.enemies[0].set_entity_type(ENEMY);
    g_game_state.enemies[0].set_ai_type(BUG);
    g_game_state.enemies[0].set_ai_state(PATROL);
    g_game_state.enemies[0].m_texture_id = load_texture(BUG_SPRITESHEET_FILEPATH);
    g_game_state.enemies[0].set_position(glm::vec3(18.0f, -9.0f, 0.0f));
    g_game_state.enemies[0].set_movement(glm::vec3(-1.0f, 0.0f, 0.0f));
    g_game_state.enemies[0].set_speed(0.5f);
    g_game_state.enemies[0].set_acceleration(glm::vec3(0.0f, -9.81f, 0.0f));

    g_game_state.enemies[0].m_walking[g_game_state.enemies[0].LEFT] = new int[2] { 0, 1 };
    g_game_state.enemies[0].m_walking[g_game_state.enemies[0].RIGHT] = new int[2] { 2, 3 };
                           
    g_game_state.enemies[0].m_animation_indices = g_game_state.enemies[0].m_walking[g_game_state.enemies[0].LEFT];
    g_game_state.enemies[0].m_animation_frames = 2;
    g_game_state.enemies[0].m_animation_index = 0;
    g_game_state.enemies[0].m_animation_time = 0.0f;
    g_game_state.enemies[0].m_animation_cols = 4;
    g_game_state.enemies[0].m_animation_rows = 1;
    g_game_state.enemies[0].set_height(1.0f);
    g_game_state.enemies[0].set_width(1.0f);

    //WASP
    g_game_state.enemies[1].set_entity_type(ENEMY);
    g_game_state.enemies[1].set_ai_type(WASP);
    g_game_state.enemies[1].set_ai_state(FLY);
    g_game_state.enemies[1].m_texture_id = load_texture(ENEMY_SPRITESHEET_FILEPATH);
    g_game_state.enemies[1].set_position(glm::vec3(8.0f, -5.0f, 0.0f));
    g_game_state.enemies[1].set_movement(glm::vec3(0.0f, -1.0f, 0.0f));
    g_game_state.enemies[1].set_speed(1.5f);
    g_game_state.enemies[1].set_acceleration(glm::vec3(0.0f, 0.0f, 0.0f));

    g_game_state.enemies[1].m_walking[g_game_state.enemies[1].IDLE] = new int[3] { 24, 25, 26 };

    g_game_state.enemies[1].m_animation_indices = g_game_state.enemies[1].m_walking[g_game_state.enemies[1].IDLE];
    g_game_state.enemies[1].m_animation_frames = 3;
    g_game_state.enemies[1].m_animation_index = 0;
    g_game_state.enemies[1].m_animation_time = 0.0f;
    g_game_state.enemies[1].m_animation_cols = 9;
    g_game_state.enemies[1].m_animation_rows = 3;
    g_game_state.enemies[1].set_height(1.0f);
    g_game_state.enemies[1].set_width(1.0f);

    g_game_state.enemies[2].set_entity_type(ENEMY);
    g_game_state.enemies[2].set_ai_type(WASP);
    g_game_state.enemies[2].set_ai_state(CIRCLE);
    g_game_state.enemies[2].m_texture_id = load_texture(ENEMY_SPRITESHEET_FILEPATH);
    g_game_state.enemies[2].set_position(glm::vec3(19.0f, -4.0f, 0.0f));
    g_game_state.enemies[2].set_movement(glm::vec3(0.0f));
    g_game_state.enemies[2].set_speed(1.5f);
    g_game_state.enemies[2].set_radius(3.0f);
    g_game_state.enemies[2].set_acceleration(glm::vec3(0.0f, 0.0f, 0.0f));

    g_game_state.enemies[2].m_walking[g_game_state.enemies[2].IDLE] = new int[3] { 24, 25, 26 };

    g_game_state.enemies[2].m_animation_indices = g_game_state.enemies[2].m_walking[g_game_state.enemies[2].IDLE];
    g_game_state.enemies[2].m_animation_frames = 3;
    g_game_state.enemies[2].m_animation_index = 0;
    g_game_state.enemies[2].m_animation_time = 0.0f;
    g_game_state.enemies[2].m_animation_cols = 9;
    g_game_state.enemies[2].m_animation_rows = 3;
    g_game_state.enemies[2].set_height(1.0f);
    g_game_state.enemies[2].set_width(1.0f);

    // ————— AUDIO SET-UP ————— //
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

    g_game_state.bgm = Mix_LoadMUS(BGM_FILEPATH);
    Mix_PlayMusic(g_game_state.bgm, 1);
    Mix_VolumeMusic(MIX_MAX_VOLUME / 32.0f);

    g_game_state.jump_sfx = Mix_LoadWAV(JUMP_SFX_FILEPATH);
    Mix_VolumeChunk(g_game_state.jump_sfx, MIX_MAX_VOLUME / 32.0f);

    g_game_state.pop_sfx = Mix_LoadWAV(POP_SFX_FILEPATH);
    Mix_VolumeChunk(g_game_state.pop_sfx, MIX_MAX_VOLUME / 32.0f);

    // ————— BLENDING ————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_game_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                // Quit the game with a keystroke
                g_game_is_running = false;
                break;

            case SDLK_SPACE:
                // Jump
                if (g_game_state.player->m_collided_bottom)
                {
                    g_game_state.player->m_is_jumping = true;
                    Mix_PlayChannel(-1, g_game_state.jump_sfx, 0);
                }
                break;

            default:
                break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT] || key_state[SDL_SCANCODE_A])
    {
        g_game_state.player->move_left();
        g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->LEFT];
    }
    else if (key_state[SDL_SCANCODE_RIGHT] || key_state[SDL_SCANCODE_D])
    {
        g_game_state.player->move_right();
        g_game_state.player->m_animation_indices = g_game_state.player->m_walking[g_game_state.player->RIGHT];
    }

    // This makes sure that the player can't move faster diagonally
    if (glm::length(g_game_state.player->get_movement()) > 1.0f)
    {
        g_game_state.player->set_movement(glm::normalize(g_game_state.player->get_movement()));
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    delta_time += g_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP)
    {
        g_game_state.player->update(FIXED_TIMESTEP, g_game_state.player, g_game_state.enemies, ENEMY_COUNT, g_game_state.map, g_game_state.pop_sfx);
        
        for (int i = 0; i < ENEMY_COUNT; i++) g_game_state.enemies[i].update(FIXED_TIMESTEP, g_game_state.player, g_game_state.enemies, ENEMY_COUNT, g_game_state.map, g_game_state.pop_sfx);
        
        delta_time -= FIXED_TIMESTEP;
    }


    g_accumulator = delta_time;

    g_view_matrix = glm::mat4(1.0f);
    g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-g_game_state.player->get_position().x, -g_game_state.player->get_position().y, 0.0f));
}

void render()
{
    g_shader_program.set_view_matrix(g_view_matrix);

    glClear(GL_COLOR_BUFFER_BIT);

    g_game_state.player->render(&g_shader_program);
    for (int i = 0; i < ENEMY_COUNT; i++) g_game_state.enemies[i].render(&g_shader_program);
    g_game_state.map->render(&g_shader_program);

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();

    delete[] g_game_state.enemies;
    delete    g_game_state.player;
    delete    g_game_state.map;
    Mix_FreeChunk(g_game_state.jump_sfx);
    Mix_FreeMusic(g_game_state.bgm);
    Mix_FreeChunk(g_game_state.pop_sfx);
}

// ————— GAME LOOP ————— //
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
