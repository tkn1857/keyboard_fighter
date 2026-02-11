#include <stddef.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "raylib.h"

#define MAX_THINGS                                  1024

#define SCREEN_WIDTH                                800 * 1
#define SCREEN_HEIGHT                               450 * 1

#define HIT_TEXT_POSITION_Y                         SCREEN_HEIGHT*0.2
#define HIT_TEXT_HEIGHT                             SCREEN_HEIGHT*0.1
#define HIT_TEXT_CAPACITY                           500

#define STAGE_COORDINATE                            SCREEN_HEIGHT*0.8 

#define GOOD_CPM                                    300

#define FRAMERATE                                   60 
#define MS_PER_FRAME                                (1000/FRAMERATE)

#define PLAYER_STATE_CAPACITY                       12

#define KNIGHT_FIGURE_WIDTH_PX                      64
#define KNIGHT_FIGURE_OFFSET                        64

#define HIT_DURATION_MS                             300.0f
#define HIT_DURATION_FRAMES                         ((int)HIT_DURATION_MS / (int)MS_PER_FRAME)
#define HIT_ANIMATION_FRAMES                        3

#define IDLE_DURATION_MS                            1000                               
#define IDLE_DURATION_FRAMES                        ((int)IDLE_DURATION_MS / (int)MS_PER_FRAME)
#define IDLE_ANIMATION_FRAMES                       4

#define INPUT_MODE_DURATION_MS                      300 
#define INPUT_MODE_DURATION_FRAMES                  ((int)INPUT_MODE_DURATION_MS / (int)MS_PER_FRAME)
#define INPUT_ANIMATION_FRAMES                      1

#define WALK_ANIMATION_FRAMES                       8
#define WALK_ANIMATION_DURATION_MS                  500 
#define WALK_ANIMATION_DURATION_FRAMES              ((int)WALK_ANIMATION_DURATION_MS / (int)MS_PER_FRAME)
#define WALK_SPEED_PERC_PER_MS                      0.02f 
#define WALK_INCREMENT_PIXEL_PER_FRAME              ((MS_PER_FRAME*WALK_SPEED_PERC_PER_MS) / 100.0f) * SCREEN_WIDTH

#define LEFT_HEADING_ANIMATION_OFFSET STATE_NUM
#define KEY_PRESSED_BUF_CAP                         12
typedef int thing_idx;

Vector2 default_orientation = {.x = 1, .y = 0};

typedef enum
{
    POSITIONABLE = (1<<0),
    CONTROLLABLE = (1<<1),
    CAN_WALK = (1<<2),
    HAS_DIRECTION = (1<<3),
    CAN_HIT = (1<<4),
    NPC = (1<<5),
    ENEMY = (1<<6),
} Traits;

typedef enum 
{
    IDLE = 0,
    INPUT_MODE,
    HIT,
    RECOVERY,
    WALK,
    DEFEND,
    RUN,
    TAKING_DAMAGE,
    DYING,
    STATE_NUM
} State;

typedef enum{
    KNIGHT = 0,
    DEMON,
    THING_KIND_NUM
} ThingKind;

typedef struct{
    Traits traits;
    ThingKind kind;
    thing_idx animations_start_idx;
    State state;
    size_t state_cnt;
    int damage;
    int health;
    Vector2 position;
    Vector2 orientation;
    size_t state_dur[STATE_NUM];
    float walk_speed; // in percent from total stage per 1 ms
    thing_idx hit_text_idx;
    Rectangle hitbox;
    float reach; // in percent from total stage len
} Thing;

typedef struct
{
    Texture2D texture;
    size_t stages_num;
    size_t start_offset_pixel;
    size_t offset_between_stages;
    size_t figure_width;
    size_t player_position_offset;
} Animation;

typedef struct 
{
    State kind;
    size_t duration;
    size_t cnt;
} PlayerState;

typedef struct 
{
    Vector2 position;
    PlayerState state[PLAYER_STATE_CAPACITY];
    size_t state_index;
    bool heading_right;
    char hit_text[HIT_TEXT_CAPACITY];
    size_t hit_text_idx;
    size_t current_hit_str;
} Player;

typedef struct 
{
    Thing things[MAX_THINGS];
    Animation animations[STATE_NUM * THING_KIND_NUM * 2];
    char hit_text[HIT_TEXT_CAPACITY];
    char key_pressed;
    thing_idx player_idx;
} Game;

PlayerState IDLE_STATE = {.kind = IDLE, .duration = IDLE_DURATION_FRAMES, .cnt = 0};
PlayerState INPUT_MODE_STATE = {.kind = INPUT_MODE, .duration = INPUT_MODE_DURATION_FRAMES, .cnt = 0};
PlayerState HIT_MODE_STATE = {.kind = HIT, .duration = HIT_DURATION_FRAMES, .cnt = 0};
PlayerState WALK_MODE_STATE = {.kind = WALK, .duration = WALK_ANIMATION_DURATION_FRAMES, .cnt = 0};

void set_state(Game* game, thing_idx idx, State state)
{
    assert(idx < MAX_THINGS);
    game->things[idx].state = state; 
    game->things[idx].state_cnt = 0;
}

void walk_start(Player* player, State kind)
{

    PlayerState* current_state = &player->state[player->state_index];
    if (current_state->kind == IDLE)
    {
        memcpy(&player->state[player->state_index], &WALK_MODE_STATE, sizeof(PlayerState));
        current_state->kind = kind;
    }
}

void walk_stop(Player* player)
{

    PlayerState* current_state = &player->state[player->state_index];
    if (current_state->kind == WALK) 
    {
        memcpy(&player->state[player->state_index], &IDLE_STATE, sizeof(PlayerState));
    }
}

Rectangle get_rect_from_animation(Animation* anim, size_t frame_num)
{
    Rectangle frame_rect = {0};
    frame_rect.y = 0;
    frame_rect.x = anim->start_offset_pixel + frame_num * (anim->offset_between_stages + anim->figure_width);
    frame_rect.width =  anim->figure_width * 2;
    frame_rect.height = anim->texture.height;
    return frame_rect;
}

void draw_hit_text(Game* game)
{
    #define RENDER_TEXT_SIZE 5
    size_t font_size = 16*1.5;
    static char render_text[RENDER_TEXT_SIZE + 1];  
    Thing * player = &game->things[game->player_idx];
    size_t start = player->hit_text_idx;
    for (size_t i = 0; i < RENDER_TEXT_SIZE; i++)
    {
        render_text[i] = game->hit_text[(start + i)%HIT_TEXT_CAPACITY];
    }     
    render_text[RENDER_TEXT_SIZE] = '\0';
    int text_len_px = MeasureText(render_text, font_size);
    // DrawLine(0, HIT_TEXT_POSITION_Y, SCREEN_WIDTH, HIT_TEXT_POSITION_Y, BLACK);
    // DrawText(&player->hit_text[player->hit_text_idx % HIT_TEXT_CAPACITY], 0, HIT_TEXT_POSITION_Y, SCREEN_WIDTH/10, BLACK);
    DrawText(render_text, player->position.x - text_len_px/2, player->position.y - 96, font_size, BLACK);

    // DrawLine(0, HIT_TEXT_POSITION_Y + HIT_TEXT_HEIGHT, SCREEN_WIDTH, HIT_TEXT_POSITION_Y + HIT_TEXT_HEIGHT, BLACK);
}

bool is_vec2_equal(Vector2 v1, Vector2 v2)
{
    if ((v1.x == v2.x) && (v1.y == v2.y)) return true;
    return false;
}

void draw_stage()
{
    DrawLine(0, STAGE_COORDINATE, SCREEN_WIDTH, STAGE_COORDINATE, BLACK);
    //DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color);                // Draw a line 
}

thing_idx get_animation(Game* game, thing_idx idx)
{
    Thing * player = &game->things[idx];
    thing_idx animation_idx = player->state + player->kind * STATE_NUM;
    if (!is_vec2_equal(player->orientation, default_orientation))
    {
        animation_idx += STATE_NUM;
    }        
    TraceLog(LOG_DEBUG,  "Animation idx:   %d", animation_idx);
    return animation_idx;
}

bool draw_player(Game * game)
{
    Thing * player = &game->things[game->player_idx];
    // PlayerState* current_state = player->state[player->state_index];
    // size_t animation_offset = 0;
    // if (!player->heading_right) animation_offset = STATE_NUM;

    thing_idx animation_idx = get_animation(game, game->player_idx);
    Animation* animation = &game->animations[animation_idx];
    int state_duration = player->state_dur[player->state];
    assert(state_duration != 0);
    size_t animation_frame = ((float)player->state_cnt/(float)state_duration) * animation->stages_num;
    Rectangle frame_rect = get_rect_from_animation(animation, animation_frame);

    //printf("%ld,%ld, %f %f \n", player->state_cnt, animation_frame,  frame_rect.x, frame_rect.y);

    Vector2 texture_position = {.x = player->position.x - animation->player_position_offset ,  .y = player->position.y - animation->texture.height};
    DrawTextureRec(animation->texture, frame_rect, texture_position, WHITE);
    DrawCircle(player->position.x , player->position.y, 5, GREEN);
    frame_rect.x = texture_position.x;
    frame_rect.y = texture_position.y;
    // DrawRectangleLinesEx(frame_rect, 2, RED);
    // current_state->cnt++;
    return true;
}


bool init_animation_from_texture(Texture2D texture, Animation* anim, size_t stages_num, size_t start_offset_pixel, size_t offset_between_stages, size_t figure_width, size_t player_position_offset)
{
    anim->texture = texture;
    anim->stages_num = stages_num;
    anim->start_offset_pixel = start_offset_pixel;
    anim->offset_between_stages = offset_between_stages;
    anim->figure_width = figure_width;
    anim->player_position_offset = player_position_offset;
    return true;
}

void init_animations_from_image_with_reverse(Image img, Animation* animations, size_t animation_idx, size_t stages_num, size_t start_offset_pixel, size_t offset_between_stages, size_t figure_width, size_t player_position_offset)
{
    Animation* anim = &animations[animation_idx];
    anim->texture = LoadTextureFromImage(img);
    anim->stages_num = stages_num;
    anim->start_offset_pixel = start_offset_pixel;
    anim->offset_between_stages = offset_between_stages;
    anim->figure_width = figure_width;
    anim->player_position_offset = player_position_offset;

    size_t width = anim->offset_between_stages + anim->figure_width;

    Image inversed_image = ImageCopy(img);
    ImageFlipHorizontal(&inversed_image);

    anim = &animations[animation_idx + STATE_NUM];
    anim->texture = LoadTextureFromImage(inversed_image);
    anim->stages_num = stages_num;
    anim->start_offset_pixel = start_offset_pixel;
    anim->offset_between_stages = offset_between_stages;
    anim->figure_width = figure_width;
    anim->player_position_offset = width - player_position_offset;
    UnloadImage(inversed_image);
}

void generate_hit_text(Game* game)
{
    for(size_t i = 0; i < HIT_TEXT_CAPACITY; i++) 
    {
       game->hit_text[i] = rand() % 25 + 97;
    }
}

void enter_input_mode(Player* player)
{
    PlayerState* current_state = &player->state[player->state_index];
    if ((current_state->kind != INPUT_MODE) && (current_state->kind != HIT))
    {
        memcpy(&player->state[player->state_index], &INPUT_MODE_STATE, sizeof(PlayerState));
    }
}



void process_game_state(Game* game)
{
    Thing* player = &game->things[game->player_idx];
    // PlayerState* current_state = &player->state[player->state_index];
    // TraceLog(LOG_INFO,  "Hit strength:   %d", player->current_hit_str);
    size_t current_state_dur = player->state_dur[player->state];
    switch(player->state)
    {
        case INPUT_MODE:
        {
            if ((current_state_dur == player->state_cnt) && (player->damage != 0))
            // if ((current_state_dur == player->state_cnt) )
            {
                player->state = HIT;
                player->state_cnt = 0;
                break;
            }
            if (game->key_pressed == game->hit_text[player->hit_text_idx])
            {
                player->damage += 1;
                player->hit_text_idx = (player->hit_text_idx + 1) % HIT_TEXT_CAPACITY;
            }
            break;
        }
        case HIT:
        {
            if (current_state_dur == player->state_cnt)
            {
                player->damage = 0;
            }
            break;
        }
        case WALK:
        {
            if ((player->orientation.x == default_orientation.x ) && (player->orientation.y == default_orientation.y))
            {
                player->position.x += WALK_INCREMENT_PIXEL_PER_FRAME;
            }
            else
            {
                player->position.x -= WALK_INCREMENT_PIXEL_PER_FRAME;
            }
            break;
        }
        default:
        {
            break;
        }
    }
}   

void increment_game(Game* game)
{
    Thing* player = &game->things[game->player_idx];
    size_t current_state_dur = player->state_dur[player->state];
    if (current_state_dur == player->state_cnt)
    {
        player->state_cnt = 0;
        player->state = IDLE;
    }
    player->state_cnt++;
}

void init_player(Game* game, thing_idx idx)
{
    Vector2 player_position = { SCREEN_WIDTH/2, STAGE_COORDINATE };
    game->things[idx].position = player_position;
    game->things[idx].state = IDLE;    
    game->things[idx].orientation.x = 1;
    game->things[idx].orientation.y = 0;

    game->things[idx].state_dur[IDLE] = IDLE_DURATION_FRAMES;
    game->things[idx].state_dur[INPUT_MODE] = INPUT_MODE_DURATION_FRAMES;
    game->things[idx].state_dur[HIT] = HIT_DURATION_FRAMES;
    game->things[idx].state_dur[WALK] = WALK_ANIMATION_DURATION_FRAMES;
}

Game init_game()
{
    Game game = {0};
    generate_hit_text(&game);
    game.player_idx = 1;
    init_player(&game, 1);
    Image idle_image = LoadImage("assets/Knight_1/Idle.png");
    Image attack_image = LoadImage("assets/Knight_1/Attack 3.png");
    Image input_image = ImageCopy(attack_image);
    ImageCrop(&input_image, (Rectangle){.x = 0, .y = 0, .width = attack_image.width/4, .height = attack_image.height});

    Image hit_image = ImageCopy(attack_image);
    ImageCrop(&hit_image, (Rectangle){.x = attack_image.width/4, .y = 0, .width = 3*attack_image.width/4, .height = attack_image.height});
    Image walk_right_image = LoadImage("assets/Knight_1/Walk.png");



    size_t player_offset = 32; 
    init_animations_from_image_with_reverse(idle_image, game.animations, IDLE, 4, 0, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset);
    init_animations_from_image_with_reverse(hit_image, game.animations, HIT, 3, 0, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset);
    init_animations_from_image_with_reverse(input_image, game.animations, INPUT_MODE, 1, 0, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset);
    init_animations_from_image_with_reverse(walk_right_image , game.animations, WALK, 8, 0, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset);



    UnloadImage(idle_image);
    UnloadImage(attack_image);
    UnloadImage(input_image);
    UnloadImage(hit_image);
    UnloadImage(walk_right_image);
    return game;
}

void draw_game(Game* game)
{
    draw_stage();
    draw_hit_text(game);
    draw_player(game);
}


void process_input(Game* game)
{
    Thing* player = &game->things[game->player_idx];
    if ( player->state != INPUT_MODE)
    {
        if (game->key_pressed == 'i')
        {
                //enter_input_mode(&game.p1);
                printf("Entering input mode\n");
                player->state = INPUT_MODE;
                player->state_cnt = 0;
        }
        if (player->state != WALK)
        {
            if (IsKeyDown(KEY_L)) 
            {
                player->orientation.x = 1;
                player->orientation.y = 0;
                player->state = WALK;
                player->state_cnt = 0;
            }
            if (IsKeyDown(KEY_H)) 
            {
                player->orientation.x = -1;
                player->orientation.y = 0;
                player->state = WALK;
                player->state_cnt = 0;
            }   
        }
        else
        {
            if ((!IsKeyDown(KEY_L)) && (!IsKeyDown(KEY_H))) set_state(game, game->player_idx, IDLE);
        }
    }
}

int main(void)
{
    srand(time(0));
    int framesCounter = 0;

    const int screenWidth = SCREEN_WIDTH;
    const int screenHeight = SCREEN_HEIGHT;
    InitWindow(screenWidth, screenHeight, "Keyboard Fighter");
    Game game = init_game();
    SetTargetFPS(FRAMERATE);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        framesCounter++;

        BeginDrawing();

        ClearBackground(RAYWHITE);

        int ch = GetCharPressed(); // Get pressed char for text input, using OS mapping
        if (ch > 0) TraceLog(LOG_INFO,  "CHAR PRESSED:   %c (%d)", ch, ch);
        // print_captured_text(&game); 
        game.key_pressed = ch;
        process_input(&game);
        Thing* player = &game.things[game.player_idx];
            //     walk_start(&game.p1, WALK);
            // }
            // if (IsKeyDown(KEY_H)) 
            // {
            //     game.p1.heading_right = false;
            //     walk_start(&game.p1, WALK);
            // }
        // printf("state: %d\n" , player->state);
        // if(IsKeyReleased(KEY_L)) walk_stop(&game.p1);
        // if(IsKeyReleased(KEY_H)) walk_stop(&game.p1);
        // game.p1_last_captured_key = ch; 
        TraceLog(LOG_DEBUG,  "STATE:  (%d) %d", player->state, player->state_cnt);
        process_game_state(&game);
        draw_game(&game);
        increment_game(&game);
        EndDrawing();
    }

    // UnloadTexture(animations[IDLE].texture);
    // UnloadTexture(animations[HIT].texture);
    // UnloadTexture(animations[INPUT_MODE].texture);

    CloseWindow();                // Close window and OpenGL context

    return 0;
}