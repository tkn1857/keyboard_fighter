#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "raylib.h"

#define HIT_FRAMES_NUM      4
#define IDLE_DURATION       20
#define INPUT_MODE_DURATION 15
#define HIT_DURATION        20
#define PLAYER_STATE_CAPACITY 12
#define IDLE_ANIMATION_FRAMES 4
#define HIT_ANIMATION_FRAMES  3
#define INPUT_ANIMATION_FRAMES 1


#define WALK_ANIMATION_FRAMES 8
#define WALK_ANIMATION_DURATION_FRAMES 100
#define WALK_INCREMENT_PIXEL_PER_FRAME 1 

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
    WALK_LEFT,
    WALK_RIGHT,
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
PlayerState INPUT_MODE_STATE = {.kind = INPUT_MODE, .duration = INPUT_MODE_DURATION, .cnt = 0};
PlayerState HIT_MODE_STATE = {.kind = HIT, .duration = HIT_DURATION, .cnt = 0};
PlayerState WALK_MODE_STATE = {.kind = WALK_LEFT, .duration = WALK_ANIMATION_DURATION_FRAMES, .cnt = 0};




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
    if ((current_state->kind == WALK_LEFT) || (current_state->kind == WALK_RIGHT))
    {
        memcpy(&player->state[player->state_index], &IDLE_STATE, sizeof(PlayerState));
    }
}


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

bool init_animation_from_texture(Texture2D texture, Animation* anim, size_t stages_num, size_t frames_dur)
{
    anim->texture = texture;
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

bool init_animation_from_image_roi(const char * path, Animation* anim, size_t stages_num, size_t frames_dur, Rectangle roi)
{
    anim->texture = LoadTexture(path);
    anim->stages_num = stages_num;
    anim->frame_rect.x = roi.x;
    anim->frame_rect.y = roi.y;
    anim->frame_rect.width = roi.width/stages_num;
    anim->frame_rect.height = anim->texture.height;
    anim->frames_dur = frames_dur;
    anim->current_frame_cnt = 0;
    anim->start_frame_cnt = 0;
    return true;
}

void enter_input_mode(Player* player)
{
    PlayerState* current_state = &player->state[player->state_index];
    memcpy(&player->state[player->state_index], &INPUT_MODE_STATE, sizeof(PlayerState));
}

void process_game_state(Player* player)
{
    PlayerState* current_state = &player->state[player->state_index];
    if ((current_state->kind == INPUT_MODE) && (current_state->duration == current_state->cnt))
    { 
        memcpy(&player->state[player->state_index], &HIT_MODE_STATE, sizeof(PlayerState));
    }

    if ((current_state->kind == WALK_LEFT) || (current_state->kind == WALK_RIGHT))
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
        case WALK_LEFT:
        {
            player->position.x -= WALK_INCREMENT_PIXEL_PER_FRAME;
            break;
        }
        case WALK_RIGHT:
        {

            player->position.x += WALK_INCREMENT_PIXEL_PER_FRAME;
            break;        
        }
    }
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
    Image idle_image = LoadImage("assets/Knight_1/Idle.png");
    Image attack_image = LoadImage("assets/Knight_1/Attack 3.png");
    Image input_image = ImageCopy(attack_image);
    Image hit_image = ImageCopy(attack_image);
    ImageCrop(&input_image, (Rectangle){.x = 0, .y = 0, .width = attack_image.width/4, .height = attack_image.height});
    ImageCrop(&hit_image, (Rectangle){.x = attack_image.width/4, .y = 0, .width = 3*attack_image.width/4, .height = attack_image.height});
    Texture2D input_texture = LoadTextureFromImage(input_image);
    Texture2D hit_texture = LoadTextureFromImage(hit_image);
    Image walk_right_image = LoadImage("assets/Knight_1/Walk.png");
    Image walk_left_image = ImageCopy(walk_right_image);
    ImageFlipHorizontal(&walk_left_image);
    Texture2D walk_left_texture = LoadTextureFromImage(walk_left_image);

    Texture2D walk_right_texture = LoadTextureFromImage(walk_right_image);


    init_animation_from_image("assets/Knight_1/Idle.png", &animations[IDLE], IDLE_ANIMATION_FRAMES, IDLE_DURATION);
    init_animation_from_texture(hit_texture, &animations[HIT], HIT_ANIMATION_FRAMES, HIT_DURATION);
    init_animation_from_texture(input_texture, &animations[INPUT_MODE], INPUT_ANIMATION_FRAMES, INPUT_MODE_DURATION);
    init_animation_from_texture(walk_left_texture , &animations[WALK_LEFT], WALK_ANIMATION_FRAMES, WALK_ANIMATION_DURATION_FRAMES );
    init_animation_from_texture(walk_right_texture , &animations[WALK_RIGHT], WALK_ANIMATION_FRAMES, WALK_ANIMATION_DURATION_FRAMES );


    UnloadImage(idle_image);
    UnloadImage(attack_image);
    UnloadImage(input_image);
    UnloadImage(hit_image);
    UnloadImage(walk_right_image);
    UnloadImage(walk_left_image);
    int currentFrame = 0;

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
        if (IsKeyDown(KEY_L)) walk_start(&player, WALK_RIGHT);
        if (IsKeyDown(KEY_H)) walk_start(&player, WALK_LEFT);


        // RLAPI bool IsKeyReleased(int key);                            // Check if a key has been released once
        if(IsKeyReleased(KEY_L)) walk_stop(&player);
        if(IsKeyReleased(KEY_H)) walk_stop(&player);

        process_game_state(&player);
        draw_player(&player, framesCounter, animations);
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