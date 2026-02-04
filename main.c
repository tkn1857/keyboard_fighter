#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "raylib.h"

#define HIT_FRAMES_NUM      4
#define IDLE_DURATION       10
#define HIT_DURATION        5
#define PLAYER_STATE_CAPACITY 12

typedef struct
{
    Texture2D texture;
    Rectangle frame_rect;
    size_t stages_num;
    size_t frames_dur;
    size_t start_frame_cnt;
    size_t current_frame_cnt; 
} Animation;

typedef enum 
{
    IDLE = 0,
    INPUT_MODE,
    HIT,
    PLAYER_STATE_KIND_CAPACITY
} PlayerStateKind;

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
} Player;
PlayerState IDLE_STATE = {.kind = IDLE, .duration = IDLE_DURATION, .cnt = 0};


bool draw_player(Player* player, size_t current_frame_cnt, Animation* animations)
{

    PlayerState* current_state = &player->state[player->state_index];
    if (current_state->duration == current_state->cnt)
    {
        memcpy(&player->state[player->state_index], &IDLE_STATE, sizeof(PlayerState));
        player->state_index = (player->state_index + 1) % PLAYER_STATE_CAPACITY;
        current_state = &player->state[player->state_index];
    } 
    Animation * animation = &animations[current_state->kind];
    size_t animation_frame = ((float)current_state->cnt/(float)current_state->duration) * animation->stages_num;
    Rectangle frame_rect = {0};
    frame_rect.x = (float)animation_frame*(float)animation->frame_rect.width;
    frame_rect.width = animation->frame_rect.width;
    frame_rect.height = animation->frame_rect.height;
    // printf("%ld,%ld,%ld, %f %f \n", current_state->cnt, current_state->duration,  animation_frame,  frame_rect.x, frame_rect.y);
    DrawTextureRec(animation->texture, frame_rect, player->position, WHITE);
    current_state->cnt++;
    return true;
}


bool init_animation_from_image(const char * path, Animation* anim, size_t stages_num, size_t frames_dur)
{
    anim->texture = LoadTexture(path);
    anim->stages_num = stages_num;
    anim->frame_rect.x = 0;
    anim->frame_rect.y = 0;
    anim->frame_rect.width = anim->texture.width/stages_num;
    anim->frame_rect.height = anim->texture.height;
    anim->frames_dur = frames_dur;
    anim->current_frame_cnt = 0;
    anim->start_frame_cnt = 0;
    return true;
}

int main(void)
{

    Vector2 player_position = { 350.0f, 280.0f };
    Player player = {.position = player_position, .state_index = 0};    
    for(size_t i = 0; i < PLAYER_STATE_CAPACITY; i++)
    {
        memcpy(&player.state[i], &IDLE_STATE, sizeof(PlayerState));
    }
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [textures] example - sprite animation");
    Animation animations[PLAYER_STATE_KIND_CAPACITY];

    init_animation_from_image("assets/Knight_1/Idle.png", &animations[IDLE], 4, IDLE_DURATION);
    init_animation_from_image("assets/Knight_1/Attack 2.png", &animations[HIT], 4, HIT_DURATION);

    int currentFrame = 0;

    int framesCounter = 0;

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        framesCounter++;

        BeginDrawing();

        ClearBackground(RAYWHITE);


        // DrawText(TextFormat("%02i FPS", framesSpeed), 575, 210, 10, DARKGRAY);

        // bool draw_player(Player* player, size_t current_frame_cnt, Animation* animations)
        draw_player(&player, framesCounter, animations);
        // DrawTextureRec(scarfy, frameRec, position, WHITE);  // Draw part of the texture


        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTexture(animations[IDLE].texture);
    UnloadTexture(animations[HIT].texture);

    CloseWindow();                // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}