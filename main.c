#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "raylib.h"

#define KNIGHT_FIGURE_WIDTH_PX  64
#define KNIGHT_FIGURE_OFFSET    64

#define HIT_FRAMES_NUM      4
#define IDLE_DURATION       20
#define INPUT_MODE_DURATION 15
#define HIT_DURATION        20
#define PLAYER_STATE_CAPACITY 12
#define IDLE_ANIMATION_FRAMES 4
#define HIT_ANIMATION_FRAMES  3
#define INPUT_ANIMATION_FRAMES 1


#define WALK_ANIMATION_FRAMES 8
#define WALK_ANIMATION_DURATION_FRAMES 50 
#define WALK_INCREMENT_PIXEL_PER_FRAME 2 

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
    bool heading_forward;
} Player;

PlayerState IDLE_STATE = {.kind = IDLE, .duration = IDLE_DURATION, .cnt = 0};
PlayerState INPUT_MODE_STATE = {.kind = INPUT_MODE, .duration = INPUT_MODE_DURATION, .cnt = 0};
PlayerState HIT_MODE_STATE = {.kind = HIT, .duration = HIT_DURATION, .cnt = 0};
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
    if (!player->heading_forward) animation_offset = PLAYER_STATE_KIND_CAPACITY;
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

// bool init_animation_from_image(const char * path, Animation* anim, size_t stages_num, size_t frames_dur)
// {
//     anim->texture = LoadTexture(path);
//     anim->stages_num = stages_num;
//     anim->frame_rect.x = 0;
//     anim->frame_rect.y = 0;
//     anim->frame_rect.width = anim->texture.width/stages_num;
//     anim->frame_rect.height = anim->texture.height;
//     anim->frames_dur = frames_dur;
//     anim->current_frame_cnt = 0;
//     anim->start_frame_cnt = 0;
//     return true;
// }

// bool init_animation_from_image_roi(const char * path, Animation* anim, size_t stages_num, size_t frames_dur, Rectangle roi)
// {
//     anim->texture = LoadTexture(path);
//     anim->stages_num = stages_num;
//     anim->frame_rect.x = roi.x;
//     anim->frame_rect.y = roi.y;
//     anim->frame_rect.width = roi.width/stages_num;
//     anim->frame_rect.height = anim->texture.height;
//     anim->frames_dur = frames_dur;
//     anim->current_frame_cnt = 0;
//     anim->start_frame_cnt = 0;
//     return true;
// }

void enter_input_mode(Player* player)
{
    PlayerState* current_state = &player->state[player->state_index];
    if ((current_state->kind != INPUT_MODE) && (current_state->kind != HIT))
    {
        memcpy(&player->state[player->state_index], &INPUT_MODE_STATE, sizeof(PlayerState));
    }
}

void process_game_state(Player* player)
{
    PlayerState* current_state = &player->state[player->state_index];
    if ((current_state->kind == INPUT_MODE) && (current_state->duration == current_state->cnt))
    { 
        memcpy(&player->state[player->state_index], &HIT_MODE_STATE, sizeof(PlayerState));
    }
    if (current_state->kind == WALK)
    {

    }
    switch(current_state->kind)
    {
        case INPUT_MODE:
        {
            if (current_state->duration == current_state->cnt)
            {
                memcpy(&player->state[player->state_index], &HIT_MODE_STATE, sizeof(PlayerState));
            }
            break;
        }
        case WALK:
        {
            if (player->heading_forward)
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


int main(void)
{

    Vector2 player_position = { 350.0f, 400.0f };
    Player player = {.position = player_position, .state_index = 0};    
    player.heading_forward = true;
    for(size_t i = 0; i < PLAYER_STATE_CAPACITY; i++)
    {
        memcpy(&player.state[i], &IDLE_STATE, sizeof(PlayerState));
    }
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [textures] example - sprite animation");
    Animation animations[PLAYER_STATE_KIND_CAPACITY * 2];
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
    // bool init_animation_from_texture(Texture2D texture, Animation* anim, size_t stages_num, size_t start_offset_pixel, size_t offset_between_stages, size_t figure_width)
    init_animation_from_texture(idle_texture, &animations[IDLE], 4, 0, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset);
    init_animation_from_texture(hit_texture, &animations[HIT], 3, 0, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset);
    init_animation_from_texture(input_texture, &animations[INPUT_MODE], 1, 0, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset);
    init_animation_from_texture(walk_right_texture , &animations[WALK], 8, 0, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset);


    size_t player_offset_left = 96; 
    init_animation_from_texture(idle_texture_left, &animations[LEFT_HEADING_ANIMATION_OFFSET + IDLE], 4, 64, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset);
    init_animation_from_texture(hit_texture_left, &animations[LEFT_HEADING_ANIMATION_OFFSET + HIT], 3, 0, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset_left);
    init_animation_from_texture(input_texture_left, &animations[LEFT_HEADING_ANIMATION_OFFSET + INPUT_MODE], 1, 0, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset_left);
    init_animation_from_texture(walk_left_texture , &animations[LEFT_HEADING_ANIMATION_OFFSET + WALK], 8, 64, KNIGHT_FIGURE_OFFSET, KNIGHT_FIGURE_WIDTH_PX, player_offset);
    // PLAYER_STATE_KIND_CAPACITY

    UnloadImage(idle_image);
    UnloadImage(attack_image);
    UnloadImage(input_image);
    UnloadImage(hit_image);
    UnloadImage(walk_right_image);
    UnloadImage(walk_left_image);

    int framesCounter = 0;

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        framesCounter++;

        BeginDrawing();

        ClearBackground(RAYWHITE);
        if (IsKeyPressed(KEY_I))
        {
            enter_input_mode(&player);
        }
        if (IsKeyDown(KEY_L)) 
        {
            player.heading_forward = true;
            walk_start(&player, WALK);
        }
        if (IsKeyDown(KEY_H)) {
            player.heading_forward = false;
            walk_start(&player, WALK);
        }

        if(IsKeyReleased(KEY_L)) walk_stop(&player);
        if(IsKeyReleased(KEY_H)) walk_stop(&player);

        process_game_state(&player);
        draw_player(&player, animations);
        // DrawTextureRec(scarfy, frameRec, position, WHITE);  // Draw part of the texture


        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTexture(animations[IDLE].texture);
    UnloadTexture(animations[HIT].texture);
    UnloadTexture(animations[INPUT_MODE].texture);

    CloseWindow();                // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}