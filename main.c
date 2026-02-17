#include <stddef.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "raylib.h"
#include "raymath.h"

#define MAX_THINGS                                  1024
#define MAX_ANIMATIONS                              1024
#define MAX_SPRITES                                 100

#define SCREEN_WIDTH                                800 * 1
#define SCREEN_HEIGHT                               450 * 1

#define HIT_TEXT_POSITION_Y                         SCREEN_HEIGHT*0.2
#define HIT_TEXT_HEIGHT                             SCREEN_HEIGHT*0.1
#define HIT_TEXT_CAPACITY                           500

#define STAGE_COORDINATE                            SCREEN_HEIGHT*0.9 

#define GOOD_CPM                                    300

#define FRAMERATE                                   60
#define MS_PER_FRAME                                (1000/FRAMERATE)

#define HIT_DURATION_MS                             300.0f
#define HIT_DURATION_FRAMES                         (int)((int)HIT_DURATION_MS / (int)MS_PER_FRAME)
#define HIT_ANIMATION_FRAMES                        3

#define IDLE_DURATION_MS                            1000                               
#define IDLE_DURATION_FRAMES                        (int)((int)IDLE_DURATION_MS / (int)MS_PER_FRAME)
#define IDLE_ANIMATION_FRAMES                       4

#define INPUT_MODE_DURATION_MS                      300 
#define INPUT_MODE_DURATION_FRAMES                  (int)((int)INPUT_MODE_DURATION_MS / (int)MS_PER_FRAME)
#define INPUT_ANIMATION_FRAMES                      1

#define WALK_ANIMATION_FRAMES                       8
#define WALK_ANIMATION_DURATION_MS                  500 
#define WALK_ANIMATION_DURATION_FRAMES              (int)((int)WALK_ANIMATION_DURATION_MS / (int)MS_PER_FRAME)
#define WALK_SPEED_PERC_PER_MS                      0.02f 
#define WALK_INCREMENT_PIXEL_PER_FRAME              ((MS_PER_FRAME*WALK_SPEED_PERC_PER_MS) / 100.0f) * SCREEN_WIDTH

#define NUM_COLUMNS                                 40
#define COLUMN_CELL_WIDTH                           SCREEN_WIDTH/NUM_COLUMNS
#define COLUMN_CELL_HEIGHT                          30

typedef int thing_idx;

Vector2 default_orientation = {.x = 1, .y = 0};
const char CHARSET[] = 
    "abcdefghijklmnopqrstuvwxyz"   // lowercase
    // "ABCDEFGHIJKLMNOPQRSTUVWXYZ"   // uppercase
    "!@#$%^&*()-_=+[]{};:,.<>/?"; // special characters

#define CHARSET_SIZE                                (sizeof(CHARSET)/sizeof(CHARSET[0]) - 1)


typedef enum
{
    POSITIONABLE = (1<<0),
    CONTROLLABLE = (1<<1),
    CAN_MOVE = (1<<2),
    HAS_DIRECTION = (1<<3),
    CAN_HIT = (1<<4),
    NPC = (1<<5),
    ENEMY = (1<<6),
} Traits;

typedef enum
{
    IDLING = (1<<0),
    HITTING = (1<<1),
    MOVING = (1<<2),
    LOOKS_LEFT = (1<<3),
    INPUTTING = (1<<4),
    MOVING_FAST = (1<<5),
    INPUT_MOVE = (1<<6),
} Attributes;

Traits ENEMY_TRAITS_DEFAULT = ENEMY|NPC|POSITIONABLE|HAS_DIRECTION|CAN_HIT;
Traits player_traits = HAS_DIRECTION | POSITIONABLE | CONTROLLABLE | CAN_HIT;


typedef enum{
    DEFAULT = 0,
    KNIGHT,
    ORC,
    COLUMN_CELL,
    THING_KIND_NUM
} ThingKind;

typedef struct{
    Traits traits;
    ThingKind kind;
    Attributes attr;
    size_t state_cnt;
    int damage;
    int health;
    Vector2 position;
    Vector2 orientation;
    float walk_speed;
    thing_idx hit_text_idx;
    size_t hit_text_size;
    Rectangle hitbox;
    float reach; // in percent from total stage len
    size_t default_walk_speed_px;
} Thing;

typedef enum
{
    IDLE_IMAGE,
    ATTACK_IMAGE,
    WALK_IMAGE,
    IMAGE_KIND_NUM
} ImageKind;   

typedef struct
{
    ImageKind kind;
    const char* path;
    size_t num; 
} Sprite;

typedef struct
{
    Sprite sprites[IMAGE_KIND_NUM];
    ThingKind kind;
    size_t start_offset_pixel; 
    size_t offset_between_stages;
    size_t figure_width; 
    size_t player_position_offset;
} SpriteSet;

typedef struct
{
    Texture2D texture;
    Attributes attr;
    ThingKind kind;
    size_t sprite_num;
    size_t start_offset_pixel;
    size_t offset_between_stages;
    size_t figure_width;
    size_t player_position_offset;
    size_t duration_frames;
} Animation;

typedef struct 
{
    Thing things[MAX_THINGS];
    Animation animations[MAX_ANIMATIONS];
    char hit_text[HIT_TEXT_CAPACITY];
    char key_pressed;
    thing_idx player_idx;
    size_t thing_num;
    size_t animation_num;
} Game;

void state_transition(Game* game, thing_idx idx)
{
    Thing* thing = &game->things[idx];
    thing->state_cnt = 0;
}
bool check_bitmask(int bitmask, int flag)
{
    return (bitmask & flag) != 0;
}


int count_bits(int x) {
    int count = 0;
    while (x) {
        count += x & 1;
        x >>= 1;
    }
    return count;
}

int set_bit(int bitmask, int flag)
{
    return bitmask | flag;
}

int clear_bit(int bitmask, int flag)
{
    return bitmask & ~flag;
}

int toggle_bit(int bitmask, int flag)
{
    return bitmask ^ flag;
}

size_t init_animation(
    Game* game,
    Traits traits, 
    Attributes attr,
    Image img, 
    SpriteSet sprite_set,
    size_t sprite_num,
    size_t duration_frames
)
{
    size_t animation_idx = game->animation_num++;
    assert(animation_idx < MAX_ANIMATIONS);
    Animation* anim = &game->animations[animation_idx];
    anim->texture = LoadTextureFromImage(img);
    anim->kind = sprite_set.kind;
    anim->attr = attr;
    anim->sprite_num = sprite_num;
    anim->start_offset_pixel = sprite_set.start_offset_pixel;
    anim->offset_between_stages = sprite_set.offset_between_stages;
    anim->figure_width = sprite_set.figure_width;
    anim->player_position_offset = sprite_set.player_position_offset;
    anim->duration_frames = duration_frames;

    if ((traits & HAS_DIRECTION) == HAS_DIRECTION)
    {    
        animation_idx = game->animation_num++;
        assert(animation_idx < MAX_ANIMATIONS);
        size_t width = anim->offset_between_stages + anim->figure_width;
        anim = &game->animations[animation_idx];
        Image inversed_image = ImageCopy(img);
        ImageFlipHorizontal(&inversed_image);
        anim->texture = LoadTextureFromImage(inversed_image);
        anim->kind = sprite_set.kind;
        anim->attr = attr | LOOKS_LEFT;
        anim->sprite_num = sprite_num;
        anim->start_offset_pixel = sprite_set.start_offset_pixel; 
        anim->offset_between_stages = sprite_set.offset_between_stages;
        anim->figure_width = sprite_set.figure_width;
        anim->player_position_offset = width - sprite_set.player_position_offset;
        anim->duration_frames = duration_frames;

        UnloadImage(inversed_image);
    }
    return 0;
}


size_t load_animations(Game* game, SpriteSet sprites, Traits traits)
{
    size_t num_of_animations = 0;
    for(ImageKind kind = 0; kind < IMAGE_KIND_NUM; kind++)
    {
        Sprite sprite = sprites.sprites[kind]; 
        Image image = LoadImage(sprites.sprites[kind].path);
        assert(sprite.num != 0);

        switch(kind)
        {
            case IDLE_IMAGE:
            {
                num_of_animations = init_animation(game, traits, IDLING, image, sprites, sprite.num, IDLE_DURATION_FRAMES);
                break;
            }   
            case ATTACK_IMAGE:
            {
                size_t sprite_num = sprite.num;
                Image input_image = ImageCopy(image);
                ImageCrop(&input_image, (Rectangle){.x = 0, .y = 0, .width = image.width/sprite_num, .height = image.height});
                Image hit_image = ImageCopy(image);
                ImageCrop(&hit_image, (Rectangle){.x = image.width/sprite_num, .y = 0, .width = ((sprite_num - 1)*image.width)/sprite_num, .height = image.height});
                num_of_animations = init_animation(game, traits, INPUTTING, input_image, sprites, 1, INPUT_MODE_DURATION_FRAMES);
                num_of_animations = init_animation(game, traits, HITTING, hit_image, sprites, sprite.num - 1, HIT_DURATION_FRAMES);
                UnloadImage(input_image);
                UnloadImage(hit_image);
                break;
            }   
            case WALK_IMAGE:
            {
                num_of_animations = init_animation(game, traits, MOVING, image, sprites, sprite.num, WALK_ANIMATION_DURATION_FRAMES);
                break;
            }
            default:
            {
                assert(false);
            }
        }
        UnloadImage(image);
    }   
    return num_of_animations;
}

thing_idx get_animation_idx(Game* game, thing_idx idx)
{
    Thing* thing = &game->things[idx];

    int best_index = -1;
    int max_overlap = -1;

    for (int i = 0; i < (int)game->animation_num; i++) {
        if (!(game->animations[i].kind == thing->kind)) continue;
        int overlap = count_bits(game->animations[i].attr & thing->attr);

        if (overlap > max_overlap) {
            max_overlap = overlap;
            best_index = i;
        }
    }

    return best_index;
}

Rectangle get_rect_from_animation(Animation* anim, size_t frame_num)
{
    // assert(anim->stages_num - 1 >= frame_num);
    if (frame_num > anim->sprite_num - 1) frame_num = anim->sprite_num - 1; // understand why this needed
    Rectangle frame_rect = {0};
    frame_rect.y = 0;
    frame_rect.x = anim->start_offset_pixel + frame_num * (anim->offset_between_stages + anim->figure_width);
    assert(frame_rect.x <= anim->texture.width);
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

void draw_stage(Game* game)
{
    DrawLine(0, STAGE_COORDINATE, SCREEN_WIDTH, STAGE_COORDINATE, BLACK);
    static char render_text[2];  
    size_t font_size = COLUMN_CELL_HEIGHT - 5;
    render_text[1] = '\0';
    for(size_t i = 0; i < game->thing_num; i++)
    {
        
        Thing* thing = &game->things[i];
        if(thing->kind != COLUMN_CELL) continue;
        render_text[0] = game->hit_text[thing->hit_text_idx];
        int text_len_px = MeasureText(render_text, font_size);
        DrawRectangleLines(thing->position.x - COLUMN_CELL_WIDTH/2, STAGE_COORDINATE, COLUMN_CELL_WIDTH, COLUMN_CELL_HEIGHT, RED);
        DrawText(render_text, thing->position.x - text_len_px/2, STAGE_COORDINATE, font_size, BLACK);
        // RLAPI void DrawRectangle(int posX, int posY, int width, int height, Color color);
    }
}

bool draw_things(Game * game)
{
    for(thing_idx i = 0; i < MAX_THINGS; i++)
    {
        Thing * thing = &game->things[i];
        if(thing->kind == DEFAULT) continue;
        thing_idx animation_idx = get_animation_idx(game, i);
        if (animation_idx < 0) return true;
        Animation* animation = &game->animations[animation_idx];
        int state_duration = animation->duration_frames;
        assert(state_duration != 0);
        size_t animation_frame = ((float)thing->state_cnt/(float)state_duration) * animation->sprite_num;
        Rectangle frame_rect = get_rect_from_animation(animation, animation_frame);
        Vector2 texture_position = {.x = thing->position.x - animation->player_position_offset ,  .y = thing->position.y - animation->texture.height};
        DrawTextureRec(animation->texture, frame_rect, texture_position, WHITE);
        DrawCircle(thing->position.x , thing->position.y, 5, GREEN);
        frame_rect.x = texture_position.x;
        frame_rect.y = texture_position.y;
    }   
    return true;
}

void generate_hit_text(Game* game)
{

    for(size_t i = 0; i < HIT_TEXT_CAPACITY; i++) 
    {
       int idx = rand() % CHARSET_SIZE;
       game->hit_text[i] = CHARSET[idx];
    }
}

void calc_attributes(Game* game)
{
    
    for(thing_idx i = 0; i < (thing_idx)game->thing_num; i++)
    {
        Thing* thing = &game->things[i];
        if ((thing->walk_speed != 0) && (!check_bitmask(thing->attr, MOVING)))
        {  
            thing->attr = MOVING; 
            state_transition(game, i);
        }   
        if (!Vector2Equals(thing->orientation, default_orientation))
        {
            thing->attr |= LOOKS_LEFT;
        }
    }   
}

void process_game(Game* game)
{
    calc_attributes(game);
    for(thing_idx i = 0; i < (thing_idx)game->thing_num; i++)
    {
        Thing* thing = &game->things[i];

        size_t anim_idx = get_animation_idx(game, i);
        Animation* anim  = &game->animations[anim_idx];
        size_t current_state_dur = anim->duration_frames; 
        
        if (check_bitmask(thing->attr, INPUTTING))
        {
            if ((current_state_dur == thing->state_cnt) && (thing->damage != 0))
            {
                thing->attr = HITTING;
                state_transition(game, i);
                // continue;
            }
            if (game->key_pressed == game->hit_text[thing->hit_text_idx])
            {
                thing->damage += 1;
                thing->hit_text_idx = (thing->hit_text_idx + 1) % HIT_TEXT_CAPACITY;
            }
        }
        if (check_bitmask(thing->attr, HITTING))
        {
            if (current_state_dur == thing->state_cnt)
            {
                thing->damage = 0;
            }
            // continue;
        }
        if (check_bitmask(thing->attr, MOVING))
        {
            // needed to stop immidiately after after walk speed is 0
            if ((thing->walk_speed == 0) && (check_bitmask(thing->attr, MOVING)))
            {
                thing->state_cnt = anim->duration_frames;
            }
            if (Vector2Equals(thing->orientation, default_orientation))
            {
                thing->position.x += thing->walk_speed;
            }
            else
            {
                thing->position.x -= thing->walk_speed;
            }
            // continue;
        }
    }   
}   


void increment_game(Game* game)
{
    for(int i = 0; i < (thing_idx)game->thing_num; i++)
    {
        Thing* thing = &game->things[i];
        size_t anim_idx = get_animation_idx(game, i);
        Animation* anim  = &game->animations[anim_idx];
        size_t current_state_dur = anim->duration_frames;
        if (current_state_dur <= thing->state_cnt) 
        {
            if(thing->walk_speed == 0)
            {
                thing->attr = IDLING;
                state_transition(game, i);
            }
            else
            {
                thing->attr = MOVING;
                state_transition(game, i);
            }
        }
        thing->state_cnt++;
    }

}

void init_player(Game* game, thing_idx idx)
{
    Vector2 player_position = { SCREEN_WIDTH/2, STAGE_COORDINATE };
    game->things[idx].position = player_position;
    game->things[idx].orientation.x = 1;
    game->things[idx].orientation.y = 0;
    game->things[idx].attr = IDLING;
    game->things[idx].walk_speed = 0;
    game->things[idx].traits = player_traits;
    game->things[idx].kind = KNIGHT;
    game->thing_num++;
}

void init_orc(Game* game)
{
    Vector2 position = {3 * SCREEN_WIDTH/4, STAGE_COORDINATE};
    thing_idx idx = game->thing_num;
    game->things[idx].position = position;
    game->things[idx].orientation.x = 1;
    game->things[idx].orientation.y = 0;
    game->things[idx].traits = ENEMY_TRAITS_DEFAULT;
    game->things[idx].kind = ORC;
    game->thing_num++;
} 


Game init_game()
{
    Game game = {0};
    generate_hit_text(&game);

    game.player_idx = 0;
    init_player(&game, game.player_idx);
    init_orc(&game) ;

    SpriteSet knigth_set = {0};
    knigth_set.kind = KNIGHT;
    knigth_set.sprites[IDLE_IMAGE].path = "assets/Knight_3/Idle.png";
    knigth_set.sprites[ATTACK_IMAGE].path = "assets/Knight_3/Attack 2.png";
    knigth_set.sprites[WALK_IMAGE].path = "assets/Knight_3/Walk.png";
    knigth_set.sprites[IDLE_IMAGE].num = 4;
    knigth_set.sprites[ATTACK_IMAGE].num = 4;
    knigth_set.sprites[WALK_IMAGE].num = 8;
    knigth_set.start_offset_pixel = 0;
    knigth_set.offset_between_stages = 64;
    knigth_set.figure_width = 64;
    knigth_set.player_position_offset = 32; 
    load_animations(&game, knigth_set, player_traits);

    SpriteSet orc_set = {0};
    orc_set.kind = ORC;
    orc_set.sprites[IDLE_IMAGE].path = "assets/Craftpix_Orc/Orc_Berserk/Idle.png";
    orc_set.sprites[ATTACK_IMAGE].path = "assets/Craftpix_Orc/Orc_Berserk/Attack_1.png";
    orc_set.sprites[WALK_IMAGE].path = "assets/Craftpix_Orc/Orc_Berserk/Walk.png";
    orc_set.sprites[IDLE_IMAGE].num = 5;
    orc_set.sprites[ATTACK_IMAGE].num = 4;
    orc_set.sprites[WALK_IMAGE].num = 7;
    orc_set.start_offset_pixel = 0;
    orc_set.offset_between_stages = 48;
    orc_set.figure_width = 48; 
    orc_set.player_position_offset = 48; 
    load_animations(&game, orc_set, ENEMY_TRAITS_DEFAULT);

    // generate unique characters
    int start_pos_x = COLUMN_CELL_WIDTH/2;
    int seen[CHARSET_SIZE] = {0};
    assert(CHARSET_SIZE >= NUM_COLUMNS);
    for (thing_idx i = 0; i < NUM_COLUMNS; i++)
    {
        Thing* thing = &game.things[game.thing_num]; 
        thing->position.x = start_pos_x;
        thing->position.y = STAGE_COORDINATE;
        thing->kind = COLUMN_CELL;
        int idx = 0;
        do
        {
            idx = rand() % HIT_TEXT_CAPACITY;
        }
        while (seen[game.hit_text[idx]] != 0);

        seen[game.hit_text[idx]] = 1;
        thing->hit_text_idx = idx;

        game.thing_num++; 
        start_pos_x += COLUMN_CELL_WIDTH;
    }
    return game;
}

void draw_game(Game* game)
{
    draw_stage(game);
    draw_hit_text(game);
    draw_things(game);
}


void process_input(Game* game)
{
    Thing* player = &game->things[game->player_idx];
    if (!check_bitmask(player->attr, INPUTTING))
    {
        if (game->key_pressed == 'i')
        {
            if ((check_bitmask(player->attr, IDLING) || (check_bitmask(player->attr, MOVING))))
            {
                player->attr = INPUTTING;
                state_transition(game, game->player_idx);
                return;
            }
        }
        if (IsKeyDown(KEY_L)) 
        {
            player->orientation.x = 1;
            player->orientation.y = 0;
        }
        if (IsKeyDown(KEY_H)) 
        {
            player->orientation.x = -1;
            player->orientation.y = 0;
        }   
        if (player->walk_speed == 0)
        {
            if ((IsKeyDown(KEY_L)) || (IsKeyDown(KEY_H)))
            {
                player->walk_speed = WALK_INCREMENT_PIXEL_PER_FRAME;
            } 
        }
        else
        {
            if ((!IsKeyDown(KEY_L)) && (!IsKeyDown(KEY_H)))
            {
                player->walk_speed = 0;
            }
        }
    }
}


void npc_ai(Game* game)
{
    for(thing_idx i = 0; i < MAX_THINGS; i++)
    {
        Thing* thing = &game->things[i];
        Thing* player = &game->things[game->player_idx];
        if ((thing->traits & ENEMY) == ENEMY)
        {
            {
                Vector2 direction_to_player = {0};
                direction_to_player.x = player->position.x - thing->position.x;
                direction_to_player.y = player->position.y - thing->position.y;
                float len = Vector2Length(direction_to_player);
                if (len > 0.0001f)
                {   
                    direction_to_player.x /= len;
                    direction_to_player.y /= len;
                    thing->orientation = direction_to_player;
                }
            }
            float dist_to_player = Vector2Distance(thing->position, player->position);
            if (dist_to_player > 64)
            {
                thing->walk_speed = WALK_INCREMENT_PIXEL_PER_FRAME/2;
            }
            else
            {
                thing->walk_speed = 0;
            }
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
        // if (ch > 0) TraceLog(LOG_INFO,  "CHAR PRESSED:   %c (%d)", ch, ch);
        // print_captured_text(&game); 
        game.key_pressed = ch;
        process_input(&game);
        npc_ai(&game);
#if 0
        Thing* player = &game.things[game.player_idx];
        size_t anim_idx = get_animation_idx(&game, game.player_idx);
        Animation* anim  = &game.animations[anim_idx];
        size_t current_state_dur = anim->duration_frames;
        TraceLog(LOG_INFO,  "Attributes:  (%d) %d %ld", player->attr, player->state_cnt, current_state_dur);
#endif
        process_game(&game);
        draw_game(&game);
        increment_game(&game);
        EndDrawing();
    }
    for(size_t i = 0; i < game.animation_num; i++)
    {
        UnloadTexture(game.animations[i].texture);
    }
    // UnloadTexture(animations[IDLE].texture);
    // UnloadTexture(animations[HIT].texture);
    // UnloadTexture(animations[INPUT_MODE].texture);

    CloseWindow();                // Close window and OpenGL context

    return 0;
}