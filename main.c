#include <stddef.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "raylib.h"

#define SCREEN_WIDTH                                800 * 1
#define SCREEN_HEIGHT                               450 * 1

#define HIT_TEXT_POSITION_Y                         SCREEN_HEIGHT*0.2
#define HIT_TEXT_HEIGHT                             SCREEN_HEIGHT*0.1
#define HIT_TEXT_CAPACITY                           3000

#define STAGE_COORDINATE                            SCREEN_HEIGHT*0.8 

#define GOOD_CPM                                    300

#define FRAMERATE                                   60 
#define MS_PER_FRAME                                (1000/FRAMERATE)

#define PLAYER_STATE_CAPACITY                       12

#define KNIGHT_FIGURE_WIDTH_PX                      64
#define KNIGHT_FIGURE_OFFSET                        64

#define HIT_DURATION_MS                             800.0f
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
#define WALK_INCREMENT_PIXEL_PER_FRAME              2 

typedef enum 
{
    IDLE = 0,
    INPUT_MODE,
    HIT,
    WALK,
    PLAYER_STATE_KIND_CAPACITY
} PlayerStateKind;

#define LEFT_HEADING_ANIMATION_OFFSET PLAYER_STATE_KIND_CAPACITY
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
    PlayerStateKind kind;
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
    Player p1;
    Player p2;
    int p1_last_captured_key;
    Animation animations[PLAYER_STATE_KIND_CAPACITY * 2];
} Game;

PlayerState IDLE_STATE = {.kind = IDLE, .duration = IDLE_DURATION_FRAMES, .cnt = 0};
PlayerState INPUT_MODE_STATE = {.kind = INPUT_MODE, .duration = INPUT_MODE_DURATION_FRAMES, .cnt = 0};
PlayerState HIT_MODE_STATE = {.kind = HIT, .duration = HIT_DURATION_FRAMES, .cnt = 0};
PlayerState WALK_MODE_STATE = {.kind = WALK, .duration = WALK_ANIMATION_DURATION_FRAMES, .cnt = 0};


void walk_start(Player* player, PlayerStateKind kind)
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

void draw_hit_text(Player* player)
{

    DrawLine(0, HIT_TEXT_POSITION_Y, SCREEN_WIDTH, HIT_TEXT_POSITION_Y, BLACK);
    DrawText(&player->hit_text[player->hit_text_idx % HIT_TEXT_CAPACITY], 0, HIT_TEXT_POSITION_Y, SCREEN_WIDTH/10, BLACK);
    // DrawLine(0, HIT_TEXT_POSITION_Y + HIT_TEXT_HEIGHT, SCREEN_WIDTH, HIT_TEXT_POSITION_Y + HIT_TEXT_HEIGHT, BLACK);
}


void draw_stage()
{
    DrawLine(0, STAGE_COORDINATE, SCREEN_WIDTH, STAGE_COORDINATE, BLACK);
    //DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color);                // Draw a line 
}
bool draw_player(Player* player, Animation* animations)
{
    PlayerState* current_state = &player->state[player->state_index];
    if (current_state->duration == current_state->cnt)
    {
        if (current_state->kind != WALK)
        {
            memcpy(&player->state[player->state_index], &IDLE_STATE, sizeof(PlayerState));
            player->state_index = (player->state_index + 1) % PLAYER_STATE_CAPACITY;
            current_state = &player->state[player->state_index];
        }
    } 
    size_t animation_offset = 0;
    if (!player->heading_right) animation_offset = PLAYER_STATE_KIND_CAPACITY;
    Animation * animation = &animations[animation_offset + current_state->kind];
    size_t animation_frame = ((float)current_state->cnt/(float)current_state->duration) * animation->stages_num;
    Rectangle frame_rect = get_rect_from_animation(animation, animation_frame);
    // printf("%ld,%ld,%ld, %f %f \n", current_state->cnt, current_state->duration,  animation_frame,  frame_rect.x, frame_rect.y);
    Vector2 texture_position = {.x = player->position.x - animation->player_position_offset ,  .y = player->position.y - animation->texture.height};
    DrawTextureRec(animation->texture, frame_rect, texture_position, WHITE);
    DrawCircle(player->position.x , player->position.y, 5, GREEN);
    frame_rect.x = texture_position.x;
    frame_rect.y = texture_position.y;
    // DrawRectangleLinesEx(frame_rect, 2, RED);
    current_state->cnt++;
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

void generate_random_text(Player* player)
{
    for(size_t i = 0; i < HIT_TEXT_CAPACITY; i++) 
    {
        // player->hit_text[i] = rand() % 95 + 32;
        player->hit_text[i] = rand() % 25 + 97;
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
    Player* player = &game->p1;
    PlayerState* current_state = &player->state[player->state_index];
    // TraceLog(LOG_INFO,  "Hit strength:   %d", player->current_hit_str);
    switch(current_state->kind)
    {
        case INPUT_MODE:
        {
            if ((current_state->duration == current_state->cnt) && (player->current_hit_str != 0))
            {
                memcpy(&player->state[player->state_index], &HIT_MODE_STATE, sizeof(PlayerState));
            }
            if (game->p1_last_captured_key == player->hit_text[player->hit_text_idx])
            {
                player->hit_text_idx++;
                player->current_hit_str++;
            }
            break;
        }
        case HIT:
        {
            if (current_state->duration == current_state->cnt)
            {
                player->current_hit_str = 0;
            }
            break;
        }
        case WALK:
        {
            if (player->heading_right)
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
Game init_game()
{
    Game game = {0};
    Vector2 player_position = { SCREEN_WIDTH/2, STAGE_COORDINATE };
    game.p1.position = player_position,
    game.p1.state_index = 0;    
    game.p1.heading_right = true;
    for(size_t i = 0; i < PLAYER_STATE_CAPACITY; i++)
    {
        memcpy(&game.p1.state[i], &IDLE_STATE, sizeof(PlayerState));
    }
    generate_random_text(&game.p1);

    Image idle_image = LoadImage("assets/Knight_1/Idle.png");
    Image idle_image_left = ImageCopy(idle_image);
    ImageFlipHorizontal(&idle_image_left);

    Image attack_image = LoadImage("assets/Knight_1/Attack 3.png");
    Image input_image = ImageCopy(attack_image);
    ImageCrop(&input_image, (Rectangle){.x = 0, .y = 0, .width = attack_image.width/4, .height = attack_image.height});
    Image input_image_left = ImageCopy(input_image);
    ImageFlipHorizontal(&input_image_left);

    Image hit_image = ImageCopy(attack_image);
    ImageCrop(&hit_image, (Rectangle){.x = attack_image.width/4, .y = 0, .width = 3*attack_image.width/4, .height = attack_image.height});
    Image hit_image_left = ImageCopy(hit_image);
    ImageFlipHorizontal(&hit_image_left);

    Image walk_right_image = LoadImage("assets/Knight_1/Walk.png");
    Image walk_left_image = ImageCopy(walk_right_image);
    ImageFlipHorizontal(&walk_left_image);


    Texture2D input_texture = LoadTextureFromImage(input_image);
    Texture2D input_texture_left = LoadTextureFromImage(input_image_left);
    Texture2D hit_texture = LoadTextureFromImage(hit_image);
    Texture2D hit_texture_left = LoadTextureFromImage(hit_image_left);
    Texture2D idle_texture = LoadTextureFromImage(idle_image);
    Texture2D idle_texture_left = LoadTextureFromImage(idle_image_left);
    Texture2D walk_left_texture = LoadTextureFromImage(walk_left_image);
    Texture2D walk_right_texture = LoadTextureFromImage(walk_right_image);

    size_t player_offset = 32; 
    init_animation_from_texture(idle_texture, &game.animations[IDLE], 4, 0, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset);
    init_animation_from_texture(hit_texture, &game.animations[HIT], 3, 0, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset);
    init_animation_from_texture(input_texture, &game.animations[INPUT_MODE], 1, 0, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset);
    init_animation_from_texture(walk_right_texture , &game.animations[WALK], 8, 0, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset);


    size_t player_offset_left = 96; 
    init_animation_from_texture(idle_texture_left, &game.animations[LEFT_HEADING_ANIMATION_OFFSET + IDLE], 4, 64, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset);
    init_animation_from_texture(hit_texture_left, &game.animations[LEFT_HEADING_ANIMATION_OFFSET + HIT], 3, 0, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset_left);
    init_animation_from_texture(input_texture_left, &game.animations[LEFT_HEADING_ANIMATION_OFFSET + INPUT_MODE], 1, 0, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset_left);
    init_animation_from_texture(walk_left_texture , &game.animations[LEFT_HEADING_ANIMATION_OFFSET + WALK], 8, 64, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset);
    // PLAYER_STATE_KIND_CAPACITY
   ; 
    UnloadImage(idle_image);
    UnloadImage(attack_image);
    UnloadImage(input_image);
    UnloadImage(hit_image);
    UnloadImage(walk_right_image);
    UnloadImage(walk_left_image);
    return game;
}
void draw_game(Game* game)
{
    Player* player = &game->p1;

    draw_hit_text(player);
    draw_stage();
    draw_player(player, game->animations);
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
        if (IsKeyPressed(KEY_I))
        {
            enter_input_mode(&game.p1);
        }
        if (IsKeyDown(KEY_L)) 
        {
            game.p1.heading_right = true;
            walk_start(&game.p1, WALK);
        }
        if (IsKeyDown(KEY_H)) {
           game.p1.heading_right = false;
           walk_start(&game.p1, WALK);
        }
        int ch = GetCharPressed(); // Get pressed char for text input, using OS mapping
        if (ch > 0) TraceLog(LOG_INFO,  "KEYBOARD TESTBED: CHAR PRESSED:   %c (%d)", ch, ch);
        if(IsKeyReleased(KEY_L)) walk_stop(&game.p1);
        if(IsKeyReleased(KEY_H)) walk_stop(&game.p1);
        game.p1_last_captured_key = ch; 
        process_game_state(&game.p1);
        draw_game(&game);

        EndDrawing();
    }

    // UnloadTexture(animations[IDLE].texture);
    // UnloadTexture(animations[HIT].texture);
    // UnloadTexture(animations[INPUT_MODE].texture);

    CloseWindow();                // Close window and OpenGL context

    return 0;
}