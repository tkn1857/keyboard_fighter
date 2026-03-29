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
#define MAX_SPRITES_PER_SPRITE_SHEET                24

#define SCREEN_WIDTH                                800 * 1
#define SCREEN_HEIGHT                               450 * 1
#define DEFAULT_THING_TEX_HEIGHT		            SCREEN_HEIGHT*0.199 	 

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

#define FLY_ANIMATION_FRAMES                       8
#define FLY_ANIMATION_DURATION_MS                  500 
#define FLY_ANIMATION_DURATION_FRAMES              (int)((int)FLY_ANIMATION_DURATION_MS / (int)MS_PER_FRAME)
#define FLY_SPEED_PERC_PER_MS                      0.02f 
#define FLY_INCREMENT_PIXEL_PER_FRAME              ((MS_PER_FRAME*FLY_SPEED_PERC_PER_MS) / 100.0f) * SCREEN_WIDTH


#define ADD_ANCHORS(sprite_set, IMAGE_KIND, ...) \
    do { \
        int _anchors_[] = {__VA_ARGS__}; \
        assert(sizeof(_anchors_)/sizeof(_anchors_[0]) == (sprite_set).sprites[(IMAGE_KIND)].frame_num); \
        memcpy((sprite_set).sprites[(IMAGE_KIND)].anchors, _anchors_, sizeof(_anchors_)); \
    } while (0)

typedef int thing_idx;

Vector2 default_orientation = {.x = 1, .y = 0};
const char CHARSET[] = 
    // "abcdefghijklmnopqrstuvwxyz"   // lowercase
    // "ABCDEFGHIJKLMNOPQRSTUVWXYZ"   // uppercase
    // "ilh"   // lowercase
    "Ii";
    // "ILH";   // uppercase
    // "!@#$%^&*()-_=+[]{};:,.<>/?"; // special characters



#define CHARSET_SIZE                                (sizeof(CHARSET)/sizeof(CHARSET[0]) - 1)

#define NUM_COLUMNS                                 CHARSET_SIZE
#define COLUMN_CELL_WIDTH                           SCREEN_WIDTH/NUM_COLUMNS
#define COLUMN_CELL_HEIGHT                          30

typedef enum
{
    POSITIONABLE = (1<<0),
    CONTROLLABLE = (1<<1),
    CAN_MOVE = (1<<2),
    HAS_DIRECTION = (1<<3),
    CAN_HIT = (1<<4),
    NPC = (1<<5),
    ENEMY = (1<<6),
    CAN_FLY = (1<<7),
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
    FLYING = (1<<7),
    // TAKING_OFF = (1<<7),
} Attributes;

Traits ENEMY_TRAITS_DEFAULT = ENEMY|NPC|POSITIONABLE|HAS_DIRECTION|CAN_HIT;
Traits PLAYER_TRAITS = HAS_DIRECTION | POSITIONABLE | CONTROLLABLE | CAN_HIT | CAN_FLY;


typedef enum{
    DEFAULT = 0,
    KNIGHT,
    ORC,
    YAMABUSHI,
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
    float movement_speed;
    thing_idx hit_text_idx;
    size_t hit_text_size;
    Rectangle hitbox;
    float reach; // in percent from total stage len
    size_t default_movement_speed_px;
} Thing;

typedef enum
{
    IDLE_IMAGE,
    ATTACK_IMAGE,
    WALK_IMAGE,
    JUMP_IMAGE,
    IMAGE_KIND_NUM
} ImageKind;   

typedef struct
{
    Image image;
    const char* image_path;
    size_t frame_num; 
    int anchors[MAX_SPRITES_PER_SPRITE_SHEET];
    int widths[MAX_SPRITES_PER_SPRITE_SHEET];
} Sprite;

typedef struct
{
    Sprite sprites[IMAGE_KIND_NUM];
    ThingKind kind;
    size_t figure_width; 
} SpriteSet;

typedef struct
{
    Texture2D textures[MAX_SPRITES];
    Attributes attr;
    ThingKind kind;
    size_t sprite_num;
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

void init_animation(
    Animation* anim,
    Attributes attr,
    SpriteSet sprite_set,
    // size_t sprite_idx,
    size_t duration_frames
)
{
    anim->kind = sprite_set.kind;
    anim->attr = attr;
    anim->duration_frames = duration_frames;
}

size_t sprite_to_animation(
    Game* game,
    Traits traits, 
    Attributes attr,
    SpriteSet sprite_set,
    size_t sprite_idx,
    size_t duration_frames,
    bool* use_anchor // array indicates if anchor should be used or not
)
{
    size_t animation_idx = game->animation_num++;
    assert(animation_idx < MAX_ANIMATIONS);
    Sprite* sprite = &sprite_set.sprites[sprite_idx]; 
    size_t anchors_num = sprite->frame_num;
    Animation* anim = &game->animations[animation_idx];
    Animation* inversed_anim = NULL;
    init_animation(anim, attr, sprite_set, duration_frames);
    if ((traits & HAS_DIRECTION) == HAS_DIRECTION)
    {
        game->animation_num++;
        assert(game->animation_num < MAX_ANIMATIONS);
        inversed_anim = &game->animations[animation_idx + 1];
        init_animation(inversed_anim, attr | LOOKS_LEFT, sprite_set, duration_frames);
    }
    Image* img = &sprite->image;

    size_t frames_num = 0;
    size_t animation_frame_idx = 0;
    for(size_t anchor_index = 0; anchor_index < anchors_num; anchor_index++)
    {
        if (use_anchor[anchor_index] == false) continue;
        else frames_num++;

        Image cropped_image = ImageCopy(*img);
        size_t width = sprite->widths[anchor_index];
        // assert(width != 0);
        if (width == 0) width = sprite_set.figure_width;
        size_t anchor = sprite->anchors[anchor_index];
        assert(anchor != 0);
        Rectangle crop_rect = {.height = img->height, .width = width, .x = anchor - width/2, .y = 0}; 
        ImageCrop(&cropped_image, crop_rect);
        float resize_coef = (float)DEFAULT_THING_TEX_HEIGHT/cropped_image.height;   
        // float resize_coef = 0.5;   
        int new_width = resize_coef * (float) cropped_image.width;
        int new_height = resize_coef * (float) cropped_image.height;
        ImageResize(&cropped_image, new_width, new_height); 
        // ExportImage(cropped_image, "test.png"); 
        // asm("int3");
        anim->textures[animation_frame_idx] = LoadTextureFromImage(cropped_image);
        if ((traits & HAS_DIRECTION) == HAS_DIRECTION)
        {    
            Image inversed_image = ImageCopy(cropped_image);
            ImageFlipHorizontal(&inversed_image);
            inversed_anim->textures[animation_frame_idx]= LoadTextureFromImage(inversed_image);
            UnloadImage(inversed_image);
        }
        animation_frame_idx++;
    }
    assert(frames_num != 0);
    anim->sprite_num = frames_num;
    if (inversed_anim != NULL) inversed_anim->sprite_num = frames_num;
    return 0;
}


size_t load_animations(Game* game, SpriteSet sprites, Traits traits)
{
    size_t num_of_animations = 0;
    bool use_anchors[MAX_SPRITES_PER_SPRITE_SHEET] = {0};
    for(int i = 0;i < MAX_SPRITES_PER_SPRITE_SHEET; i++) {use_anchors[i] = true;}
    for(ImageKind kind = 0; kind < IMAGE_KIND_NUM; kind++)
    {
        Sprite sprite = sprites.sprites[kind]; 
        if (sprites.sprites[kind].image_path == NULL) continue;
        Image image = LoadImage(sprites.sprites[kind].image_path);
        assert(image.width != 0);
        sprites.sprites[kind].image = image;
        assert(sprite.frame_num != 0);
        for(int i = 0;i < MAX_SPRITES_PER_SPRITE_SHEET; i++) {use_anchors[i] = true;}

        switch(kind)
        {
            case IDLE_IMAGE:
            {
                num_of_animations = sprite_to_animation(game, traits, IDLING, sprites, IDLE_IMAGE, IDLE_DURATION_FRAMES, use_anchors);
                break;
            }   
            case ATTACK_IMAGE:
            {

                for(int i = 0;i < MAX_SPRITES_PER_SPRITE_SHEET; i++) {use_anchors[i] = false;}
                use_anchors[0] = true;
                num_of_animations = sprite_to_animation(game, traits, INPUTTING, sprites, ATTACK_IMAGE, INPUT_MODE_DURATION_FRAMES, use_anchors);
                for(int i = 0;i < MAX_SPRITES_PER_SPRITE_SHEET; i++) {use_anchors[i] = true;}
                use_anchors[0] = false;
                num_of_animations = sprite_to_animation(game, traits, HITTING, sprites, ATTACK_IMAGE, HIT_DURATION_FRAMES, use_anchors);
                break;
            }   
            case WALK_IMAGE:
            {
                num_of_animations = sprite_to_animation(game, traits, MOVING, sprites, WALK_IMAGE, WALK_ANIMATION_DURATION_FRAMES, use_anchors);
                break;
            }
            case JUMP_IMAGE:
            {
                for(int i = 0;i < MAX_SPRITES_PER_SPRITE_SHEET; i++) {use_anchors[i] = false;}
                for(int i = 5;i <= 10; i++) {use_anchors[i] = true;}
                num_of_animations = sprite_to_animation(
                        game,
                        traits,
                        FLYING | IDLING | MOVING,
                        sprites,
                        JUMP_IMAGE,
                        FLY_ANIMATION_DURATION_FRAMES,
                        use_anchors);
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

// Rectangle get_rect_from_animation(Animation* anim, size_t frame_num)
// {
//     // assert(anim->stages_num - 1 >= frame_num);
//     Rectangle frame_rect = {0};
//     if (frame_num > anim->sprite_num - 1) frame_num = anim->sprite_num - 1; // understand why this needed
//     {
//         frame_rect.x = anchor - anim->figure_width/2;
//         frame_rect.width =  anim->figure_width;
//         frame_rect.height = anim->texture.height;
//     }
//     return frame_rect;
// }

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
        size_t animation_frame = (size_t)(((float)thing->state_cnt/(float)state_duration) * (float)animation->sprite_num);
        if (animation_frame >= animation->sprite_num) animation_frame = animation->sprite_num - 1;
        Texture2D* texture = &animation->textures[animation_frame];
        if (texture->id == 0) continue;
        Vector2 texture_position = {.x = thing->position.x - texture->width/2 ,.y = thing->position.y - texture->height};

        DrawTextureV(*texture, texture_position, WHITE);
        DrawCircle(thing->position.x , thing->position.y, 5, GREEN);
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
        if ((thing->movement_speed != 0) && (!check_bitmask(thing->attr, MOVING)))
        {  
            thing->attr = MOVING; 
            state_transition(game, i);
        }   
        if (!Vector2Equals(thing->orientation, default_orientation))
        {
            thing->attr |= LOOKS_LEFT;
        }
        if (thing->position.y < STAGE_COORDINATE)
        {
            thing->attr |= FLYING;
        }
    }   
}

void process_game(Game* game)
{
    calc_attributes(game);
    for(thing_idx i = 0; i < (thing_idx)game->thing_num; i++)
    {
        Thing* thing = &game->things[i];
        int anim_idx = get_animation_idx(game, i);
        if (anim_idx == -1) return;
        Animation* anim  = &game->animations[anim_idx];
        size_t current_state_dur = anim->duration_frames; 
        
        if (check_bitmask(thing->attr, INPUTTING))
        {
            if ((current_state_dur == thing->state_cnt) && (thing->damage != 0))
            {
                thing->attr = HITTING;
                state_transition(game, i);
                continue;
            }
            if (game->key_pressed == game->hit_text[thing->hit_text_idx])
            {
                thing->damage += 1;
                thing->hit_text_idx = (thing->hit_text_idx + 1) % HIT_TEXT_CAPACITY;
                continue;
            }
        }
        if (check_bitmask(thing->attr, MOVING))
        {
            // needed to stop immidiately after after walk speed is 0
            if ((thing->movement_speed == 0) && (check_bitmask(thing->attr, MOVING)))
            {
                thing->state_cnt = anim->duration_frames;
            }

            thing->position.x += thing->movement_speed * thing->orientation.x;
            thing->position.y += thing->movement_speed * thing->orientation.y;
            if(thing->position.y >= STAGE_COORDINATE) thing->position.y = STAGE_COORDINATE;
            // if (Vector2Equals(thing->orientation, default_orientation))
            // {
            //     thing->position.x += thing->movement_speed;
            // }
            // else
            // {
            //     thing->position.x -= thing->movement_speed;
            // }
            continue;
        }
    }   
}   


void increment_game(Game* game)
{
    for(int i = 0; i < (thing_idx)game->thing_num; i++)
    {
        Thing* thing = &game->things[i];
        int anim_idx = get_animation_idx(game, i);
        if (anim_idx == -1) return;
        Animation* anim  = &game->animations[anim_idx];
        size_t current_state_dur = anim->duration_frames;
        if (current_state_dur <= thing->state_cnt) 
        {
            thing->damage = 0;
            if(thing->movement_speed == 0)
            {
                thing->attr = IDLING;
                state_transition(game, i);
            }
            else
            {
                thing->attr = MOVING;
                state_transition(game, i);
            }
            // Keep the first frame after a loop/state transition.
            // Incrementing immediately here causes a visible hitch at loop boundaries.
            continue;
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
    game->things[idx].movement_speed = 0;
    game->things[idx].traits = PLAYER_TRAITS;
    game->things[idx].kind = YAMABUSHI;
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


void init_knight_enemy(Game* game)
{
    Vector2 position = {SCREEN_WIDTH/4, STAGE_COORDINATE};
    thing_idx idx = game->thing_num;
    game->things[idx].position = position;
    game->things[idx].orientation.x = 1;
    game->things[idx].orientation.y = 0;
    game->things[idx].traits = ENEMY_TRAITS_DEFAULT;
    game->things[idx].kind = KNIGHT;
    game->thing_num++;
}

Game init_game()
{
    Game game = {0};
    generate_hit_text(&game);

    game.player_idx = 0;
    init_player(&game, game.player_idx);
    init_orc(&game);
    // init_knight_enemy(&game); 

   
    SpriteSet knigth_set = {0};
    knigth_set.kind = KNIGHT;
    knigth_set.sprites[IDLE_IMAGE].image_path = "assets/Knight_3/Idle.png";
    knigth_set.sprites[ATTACK_IMAGE].image_path = "assets/Knight_3/Attack 2.png";
    knigth_set.sprites[WALK_IMAGE].image_path = "assets/Knight_2/Walk.png";
    knigth_set.sprites[IDLE_IMAGE].frame_num = 4;
    knigth_set.sprites[ATTACK_IMAGE].frame_num = 4;
    knigth_set.sprites[WALK_IMAGE].frame_num = 8;
    knigth_set.figure_width = 64;
    ADD_ANCHORS(knigth_set, IDLE_IMAGE, 64, 192, 320, 448);
    ADD_ANCHORS(knigth_set, WALK_IMAGE, 64, 192, 320, 448, 576, 704, 832, 960);
    ADD_ANCHORS(knigth_set, ATTACK_IMAGE, 64, 192, 320, 448);
    load_animations(&game, knigth_set, PLAYER_TRAITS);
    {
        SpriteSet set = {0};
        set.kind = ORC;
        set.sprites[IDLE_IMAGE].image_path = "assets/Craftpix_Orc/Orc_Berserk/Idle.png";
        set.sprites[ATTACK_IMAGE].image_path = "assets/Craftpix_Orc/Orc_Berserk/Attack_1.png";
        set.sprites[WALK_IMAGE].image_path = "assets/Craftpix_Orc/Orc_Berserk/Walk.png";
        set.sprites[IDLE_IMAGE].frame_num = 5;
        set.sprites[ATTACK_IMAGE].frame_num = 4;
        set.sprites[WALK_IMAGE].frame_num = 7;
        set.figure_width = 96; 

        ADD_ANCHORS(set, IDLE_IMAGE, 48, 144, 240, 336, 432);
        ADD_ANCHORS(set, WALK_IMAGE, 48, 144, 240, 336, 432, 528, 624);
        ADD_ANCHORS(set, ATTACK_IMAGE, 45, 140, 245, 341);
        load_animations(&game, set, PLAYER_TRAITS);
    }
    { 
        SpriteSet set = {0};
        set.kind = YAMABUSHI;
        set.sprites[IDLE_IMAGE].image_path = "assets/Yamabushi/Idle.png";
        set.sprites[ATTACK_IMAGE].image_path = "assets/Yamabushi/Attack_1.png";
        set.sprites[WALK_IMAGE].image_path = "assets/Yamabushi/Walk.png";
        set.sprites[JUMP_IMAGE].image_path = "assets/Yamabushi/Jump.png";
        set.sprites[IDLE_IMAGE].frame_num = 6;
        set.sprites[ATTACK_IMAGE].frame_num = 3;
        set.sprites[WALK_IMAGE].frame_num = 8;
        set.sprites[JUMP_IMAGE].frame_num = 15;
        set.figure_width = 100; 

        ADD_ANCHORS(set, IDLE_IMAGE, 68, 200, 324, 452, 580, 708);
        ADD_ANCHORS(set, WALK_IMAGE, 64, 192, 320, 448, 576, 704, 832, 960);
        ADD_ANCHORS(set, ATTACK_IMAGE, 55, 189, 313);
        ADD_ANCHORS(set, JUMP_IMAGE, 64, 192, 320, 448, 576, 704, 832, 960, 1088, 1216, 1344, 1472, 1600, 1728, 1856);
        for (size_t i = 0; i < set.sprites[JUMP_IMAGE].frame_num; i++)
        {
            set.sprites[JUMP_IMAGE].widths[i] = (int)set.figure_width;
        }
        load_animations(&game, set, PLAYER_TRAITS);
    }
    // generate unique characters
    int start_pos_x = COLUMN_CELL_WIDTH/2;
    int seen[256] = {0};
    assert(CHARSET_SIZE >= NUM_COLUMNS);
    for (thing_idx i = 0; i < (thing_idx)NUM_COLUMNS; i++)
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
        while (seen[(unsigned char)game.hit_text[idx]] != 0);

        seen[(unsigned char)game.hit_text[idx]] = 1;
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
    // make sure that hit animation and input animation is not canceled by input
    if ((check_bitmask(player->attr, INPUTTING)) || (check_bitmask(player->attr, HITTING))) return;
    if (game->key_pressed == 'i')
    {
        if ((check_bitmask(player->attr, IDLING) || (check_bitmask(player->attr, MOVING))))
        {
            player->attr = INPUTTING;
            state_transition(game, game->player_idx);
            game->key_pressed = 0;
            return;
        }
    }
    if (player->movement_speed == 0)
    {
        if ((IsKeyDown(KEY_L)) || (IsKeyDown(KEY_H)|| IsKeyDown(KEY_K) || IsKeyDown(KEY_J)))
        {
            player->movement_speed = WALK_INCREMENT_PIXEL_PER_FRAME;
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
        if (IsKeyDown(KEY_K)) 
        {
            player->orientation.x = 0;
            player->orientation.y = -1;
        }   
        if (IsKeyDown(KEY_J)) 
        {
            player->orientation.x = 0;
            player->orientation.y = 1;
        }
    }
    else
    {
        if ((!IsKeyDown(KEY_L)) && (!IsKeyDown(KEY_H) && !IsKeyDown(KEY_K) && !IsKeyDown(KEY_J)))
        {
            player->movement_speed = 0;
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
                    if ((thing->traits & CAN_FLY) != CAN_FLY) thing->orientation.y = 0;
                    thing->orientation = Vector2Normalize(thing->orientation);
                }
            }
            float dist_to_player = Vector2Distance(thing->position, player->position);
            if (dist_to_player > 64)
            {
                thing->movement_speed = WALK_INCREMENT_PIXEL_PER_FRAME/2;
            }
            else
            {
                thing->movement_speed = 0;
            }
        }
    }
}

int main(void)
{
    srand(time(0));
    int framesCounter = 0;

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Keyboard Fighter");
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
        // print_capthing->orientation = tured_text(&game); 
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
    // for(size_t i = 0; i < game.animation_num; i++)
    // {
    //     UnloadTexture(game.animations[i].texture);
    // }
    // UnloadTexture(animations[IDLE].texture);
    // UnloadTexture(animations[HIT].texture);
    // UnloadTexture(animations[INPUT_MODE].texture);

    CloseWindow();                
    return 0;
}
