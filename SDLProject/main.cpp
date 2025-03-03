/**
* Author: [ Aayush Daftary]
* Assignment: Pong Clone
* Date due: 2025-3-01, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"                // 4x4 Matrix
#include "glm/gtc/matrix_transform.hpp"  // Matrix transformation methods
#include "ShaderProgram.h"               // We'll talk about these later in the course
#include "stb_image.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>

enum AppStatus { RUNNING, TERMINATED };

// Our window dimensions
constexpr int WINDOW_WIDTH  = 640 * 2,
              WINDOW_HEIGHT = 480 * 2;

// Background color components
constexpr float BG_RED     = 0.9765f,
                BG_GREEN   = 0.9726f,
                BG_BLUE    = 0.9609f,
                BG_OPACITY = 1.0f;

// Our viewport—or our "camera"'s—position and dimensions
constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

// Our shader filepaths; these are necessary for a number of things
// Not least, to actually draw our shapes
// We'll have a whole lecture on these later
constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr glm::vec3 PADDLE_SCALE = glm::vec3(1.5f, 2.0f, 1.0f);
constexpr glm::vec3 BALL_SCALE = glm::vec3(0.5f, 0.5f, 1.0f);

constexpr float PADDLE_SPEED= 3.0f;
constexpr float AI_PADDLE_SPEED = 2.0f;
constexpr float BALL_SPEED_X = 2.0f;
constexpr float BALL_SPEED_Y = 2.0f;
// for collisions
constexpr float PADDLE_HALF_WIDTH = PADDLE_SCALE.x / 2.0f;
constexpr float PADDLE_HALF_HEIGHT = PADDLE_SCALE.y / 2.0f;
constexpr float BALL_HALF_WIDTH = BALL_SCALE.x /2.0f;
constexpr float BALL_HALF_HEIGHT= BALL_SCALE.y /2.0f;

constexpr float VERTICAL_TOP= 3.5f;
constexpr float VERTICAL_BOTTOM = -3.5f;
constexpr float PADDLE_VERTICAL_LIMIT = VERTICAL_TOP - PADDLE_HALF_HEIGHT;
constexpr float BALL_TOP_BOUNDARY = VERTICAL_TOP - BALL_HALF_HEIGHT;
constexpr float BALL_BOTTOM_BOUNDARY = VERTICAL_BOTTOM + BALL_HALF_HEIGHT;
constexpr float LEFT_BOUNDARY = -5.0f;
constexpr float RIGHT_BOUNDARY = 5.0f;

constexpr char LEFT_PADDLE_TEXTURE_PATH[] = "texture1.png";
constexpr char RIGHT_PADDLE_TEXTURE_PATH[] = "texture2.png";
constexpr char BALL_TEXTURE_PATH[] = "ball.png";
constexpr char GAME_OVER_TEXTURE_PATH[] = "gameover1.png";
constexpr char USA_WIN_PATH[] = "usa.png";
constexpr char CANADA_WIN_PATH[] = "canada.png";

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;
float g_previous_ticks = 0.0f;

GLuint g_left_paddle_texture_id,
       g_right_paddle_texture_id,
       g_ball_texture_id,
       g_game_over_texture_id,
       g_usa_win_id,
       g_canada_win_id;

glm::vec3 left_paddle_pos(-4.5f, 0.0f, 0.0f);
glm::vec3 right_paddle_pos(4.5f, 0.0f, 0.0f);
glm::mat4 left_paddle_matrix = glm::mat4(1.0f);
glm::mat4 right_paddle_matrix = glm::mat4(1.0f);
std::vector<glm::mat4> ball_matrices;
glm::mat4 game_over_matrix = glm::mat4(1.0f);
glm::mat4 flag_matrix = glm::mat4(1.0f);

struct Ball {
    glm::vec3 pos;
    glm::vec3 velocity;
};
std::vector<Ball> balls;

bool game_over = false;
bool right_paddle_ai = false;
int ai_direction = 1;
enum Winner { NONE, LEFT, RIGHT };
Winner g_winner = NONE;

GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    if (image == nullptr)
    {
        LOG("Unable to load image: " << filepath);
        assert(false);
    }
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    stbi_image_free(image);
    return textureID;
}

void reset_game()
{
    game_over = false;
    left_paddle_pos = glm::vec3(-4.5f, 0.0f, 0.0f);
    right_paddle_pos = glm::vec3(4.5f, 0.0f, 0.0f);
    balls.clear();
    Ball initial_ball;
    initial_ball.pos = glm::vec3(0.0f, 0.0f, 0.0f);
    initial_ball.velocity = glm::vec3(BALL_SPEED_X, BALL_SPEED_Y, 0.0f);
    balls.push_back(initial_ball);
    ai_direction = 1;
    g_previous_ticks = SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    g_winner = NONE;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    
    g_display_window = SDL_CreateWindow("Pong Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }
#ifdef _WINDOWS
    glewInit();
#endif
    
    // Initialise our camera
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    // Load up our shaders
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    // Initialise our view, model, and projection matrices
    g_view_matrix       = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);   // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    // Notice we haven't set our model matrix yet!
    
    // Each object has its own unique ID
    glUseProgram(g_shader_program.get_program_id());
    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);
    g_left_paddle_texture_id  = load_texture(LEFT_PADDLE_TEXTURE_PATH);
    g_right_paddle_texture_id = load_texture(RIGHT_PADDLE_TEXTURE_PATH);
    g_ball_texture_id         = load_texture(BALL_TEXTURE_PATH);
    g_game_over_texture_id    = load_texture(GAME_OVER_TEXTURE_PATH);
    g_usa_win_id = load_texture(USA_WIN_PATH);
    g_canada_win_id = load_texture(CANADA_WIN_PATH);
    
    Ball ball;
    ball.pos = glm::vec3(0.0f, 0.0f, 0.0f);
    ball.velocity = glm::vec3(BALL_SPEED_X, BALL_SPEED_Y, 0.0f);
    balls.push_back(ball);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_t:
                        right_paddle_ai = !right_paddle_ai;
                        break;
                    case SDLK_r:
                        reset_game(); //may need to hit 'r' to start game on launch
                        break;
                    //multilpe balls
                    case SDLK_1:
                        balls.resize(1);
                        balls[0].pos = glm::vec3(0.0f, 0.0f, 0.0f);
                        balls[0].velocity = glm::vec3(BALL_SPEED_X, BALL_SPEED_Y, 0.0f);
                        break;
                    case SDLK_2:
                        balls.resize(2);
                        for (int i = 0; i < 2; i++)
                        {
                            balls[i].pos = glm::vec3(0.0f, 0.0f, 0.0f);
                            if (i % 2 == 0)
                                balls[i].velocity = glm::vec3(-BALL_SPEED_X, BALL_SPEED_Y, 0.0f);
                            else
                                balls[i].velocity = glm::vec3(BALL_SPEED_X, -BALL_SPEED_Y, 0.0f);
                        }
                        break;
                    case SDLK_3:
                        balls.resize(3);
                        for (int i = 0; i < 3; i++)
                        {
                            balls[i].pos = glm::vec3(0.0f, 0.0f, 0.0f);
                            if (i == 0)
                                balls[i].velocity = glm::vec3(BALL_SPEED_X - 0.5f, BALL_SPEED_Y, 0.0f);
                            else if (i == 1)
                                balls[i].velocity = glm::vec3(BALL_SPEED_X, -BALL_SPEED_Y, 0.0f);
                            else
                                balls[i].velocity = glm::vec3(-BALL_SPEED_X + 0.5f, BALL_SPEED_Y, 0.0f);
                        }
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }

    
    //verticval boundaries
    if (left_paddle_pos.y > PADDLE_VERTICAL_LIMIT)
    {
        left_paddle_pos.y = PADDLE_VERTICAL_LIMIT;
    }
    if (left_paddle_pos.y < -PADDLE_VERTICAL_LIMIT)
    {
        left_paddle_pos.y = -PADDLE_VERTICAL_LIMIT;
    }
    if (right_paddle_pos.y > PADDLE_VERTICAL_LIMIT)
    {
        right_paddle_pos.y = PADDLE_VERTICAL_LIMIT;
    }
    if (right_paddle_pos.y < -PADDLE_VERTICAL_LIMIT){
        right_paddle_pos.y = -PADDLE_VERTICAL_LIMIT;
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    const Uint8* key_state = SDL_GetKeyboardState(NULL);
    if (key_state[SDL_SCANCODE_W])
    {
        left_paddle_pos.y += PADDLE_SPEED*delta_time;
    }
    else if (key_state[SDL_SCANCODE_S])
    {
        left_paddle_pos.y -= PADDLE_SPEED *delta_time;
    }
    if (!right_paddle_ai)
    {
        if (key_state[SDL_SCANCODE_UP])
        {
            right_paddle_pos.y += PADDLE_SPEED*delta_time;
        }
        else if (key_state[SDL_SCANCODE_DOWN])
        {
            right_paddle_pos.y -= PADDLE_SPEED * delta_time;
        }
    }
    if (game_over)
    {
        return;
    }
    //ai movement
    if (right_paddle_ai)
    {
        right_paddle_pos.y += ai_direction * AI_PADDLE_SPEED * delta_time;
        if (right_paddle_pos.y > PADDLE_VERTICAL_LIMIT)
        {
            right_paddle_pos.y = PADDLE_VERTICAL_LIMIT;
            ai_direction = -1;
        }
        else if (right_paddle_pos.y < -PADDLE_VERTICAL_LIMIT)
        {
            right_paddle_pos.y = -PADDLE_VERTICAL_LIMIT;
            ai_direction = 1;
        }
    }
    
    for (Ball &ball : balls)
    {
        ball.pos += ball.velocity * delta_time;
        
        if (ball.pos.y + BALL_HALF_HEIGHT > VERTICAL_TOP)
        {
            
            ball.pos.y = VERTICAL_TOP - BALL_HALF_HEIGHT;
            ball.velocity.y = -ball.velocity.y;
        }
        if (ball.pos.y - BALL_HALF_HEIGHT < VERTICAL_BOTTOM)
        {
            ball.pos.y = VERTICAL_BOTTOM + BALL_HALF_HEIGHT;
            ball.velocity.y = -ball.velocity.y;
        }
        
        if (ball.velocity.x < 0)
        {
            float x_distance = fabs(ball.pos.x - left_paddle_pos.x) - (PADDLE_HALF_WIDTH + BALL_HALF_WIDTH);
            float y_distance = fabs(ball.pos.y - left_paddle_pos.y) - (PADDLE_HALF_HEIGHT + BALL_HALF_HEIGHT);
            if (x_distance < 0.0f && y_distance < 0.0f)
            {
                ball.pos.x = left_paddle_pos.x + (PADDLE_HALF_WIDTH + BALL_HALF_WIDTH);
                ball.velocity.x = -ball.velocity.x;
            }
        }
        if (ball.velocity.x > 0)
        {
            float x_distance = fabs(ball.pos.x - right_paddle_pos.x) - (PADDLE_HALF_WIDTH + BALL_HALF_WIDTH);
            float y_distance = fabs(ball.pos.y - right_paddle_pos.y) - (PADDLE_HALF_HEIGHT + BALL_HALF_HEIGHT);
            if (x_distance < 0.0f && y_distance < 0.0f)
            {
                ball.pos.x = right_paddle_pos.x - (PADDLE_HALF_WIDTH + BALL_HALF_WIDTH);
                ball.velocity.x = -ball.velocity.x;
            }
        }
        if (ball.pos.x - BALL_HALF_WIDTH < LEFT_BOUNDARY)
        {
            game_over = true;
            g_winner = RIGHT;
        }
        if (ball.pos.x + BALL_HALF_WIDTH > RIGHT_BOUNDARY)
        {
            game_over = true;
            g_winner = LEFT;
        }
    }
    
    left_paddle_matrix = glm::translate(glm::mat4(1.0f), left_paddle_pos);
    left_paddle_matrix = glm::scale(left_paddle_matrix, PADDLE_SCALE);
    right_paddle_matrix = glm::translate(glm::mat4(1.0f), right_paddle_pos);
    right_paddle_matrix = glm::scale(right_paddle_matrix, PADDLE_SCALE);
    
    ball_matrices.clear();
    for (const Ball &ball :balls)
    {
        glm::mat4 mat = glm::translate(glm::mat4(1.0f), ball.pos);
        mat = glm::scale(mat, BALL_SCALE);
        ball_matrices.push_back(mat);
    }
    if (game_over)
    {
        game_over_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f));
        game_over_matrix = glm::scale(game_over_matrix, glm::vec3(4.0f, 1.0f, 1.0f));
        flag_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        flag_matrix = glm::scale(flag_matrix, glm::vec3(2.0f, 2.0f, 1.0f));
    }
}

void draw_object(glm::mat4 &model_matrix, GLuint &texture_id)
{
    g_shader_program.set_model_matrix(model_matrix);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    float vertices[] = {
        -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,   // Triangle 1
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f    // Triangle 2
    };
    float texture_coordinates[] = {
        0.0f, 1.0f,  1.0f, 1.0f,  1.0f, 0.0f,   // Triangle 1
        0.0f, 1.0f,  1.0f, 0.0f,  0.0f, 0.0f    // Triangle 2
    };
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    draw_object(left_paddle_matrix, g_left_paddle_texture_id);
    draw_object(right_paddle_matrix, g_right_paddle_texture_id);
    
    for (const glm::mat4 &ball_matrix : ball_matrices)
    {
        draw_object(const_cast<glm::mat4&>(ball_matrix), g_ball_texture_id);
    }
    if (game_over)
    {
        draw_object(game_over_matrix, g_game_over_texture_id);
        if(g_winner == LEFT)
        {
            draw_object(flag_matrix, g_usa_win_id);
        }
        if (g_winner== RIGHT)
        {
            draw_object(flag_matrix, g_canada_win_id);
        }
    }
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }
/**
 Start here—we can see the general structure of a game loop without worrying too much about the details yet.
 */
int main(int argc, char* argv[])
{
    // Initialise our program—whatever that means
    initialise();
    
    while (g_app_status == RUNNING)
    {
        process_input();  // If the player did anything—press a button, move the joystick—process it
        update();         // Using the game's previous state, and whatever new input we have, update the game's state
        render();         // Once updated, render those changes onto the screen
    }
    
    shutdown();  // The game is over, so let's perform any shutdown protocols
    return 0;
}
