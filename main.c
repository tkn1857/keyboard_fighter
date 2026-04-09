#include <stddef.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
//enable debug view of thing position, hitbox, reach
#define DEBUG_THINGS
#define DEBUG_ATTR

#define MAX_THINGS                                  1024
#define MAX_ANIMATIONS                              1024
#define MAX_SPRITES                                 100
#define MAX_SPRITES_PER_SPRITE_SHEET                24
#define MAX_TEXTURES_PER_ANIMATION                  24 * 2

#define SCREEN_WIDTH                                1024 * 1
#define SCREEN_HEIGHT                               1024 * 1
#define LINE_NUMBER_OFFSET                          (int)(SCREEN_WIDTH*0.1)

#define GRID_Y                                      10 
#define GRID_X                                      30 

#define CELL_HEIGHT                                 (int)(SCREEN_HEIGHT/GRID_Y)  
#define CELL_WIDTH                                  (int)(SCREEN_WIDTH/GRID_X)
#define GRID_TRANSPARENCY                           20 

#define THING_HEIGHT_DEFAULT		                (int)(CELL_HEIGHT * 0.7)

#define HIT_TEXT_POSITION_Y                         (int)(CELL_HEIGHT * 0.9)
#define HIT_TEXT_HEIGHT                             (int)(CELL_HEIGHT * 0.1)
#define HIT_TEXT_CAPACITY                           500

#define STAGE_COORDINATE                            CELL_HEIGHT * (GRID_Y - 2)

#define GOOD_CPM                                    300

#define FRAMERATE                                   60
#define MS_PER_FRAME                                (1000/FRAMERATE)

#define HIT_DURATION_MS                             300.0f
#define HIT_DURATION_FRAMES                         (int)((int)HIT_DURATION_MS / (int)MS_PER_FRAME)

#define IDLE_DURATION_MS                            1000                               
#define IDLE_DURATION_FRAMES                        (int)((int)IDLE_DURATION_MS / (int)MS_PER_FRAME)

#define INPUT_MODE_DURATION_MS                      300 
#define INPUT_MODE_DURATION_FRAMES                  (int)((int)INPUT_MODE_DURATION_MS / (int)MS_PER_FRAME)

#define DEFEND_ANIMATION_DURATION_MS                300
#define DEFEND_ANIMATION_DURATION_FRAMES            (int)((int)DEFEND_ANIMATION_DURATION_MS / (int)MS_PER_FRAME)

#define TAKING_DAMAGE_ANIMATION_DURATION_MS         300
#define TAKING_DAMAGE_ANIMATION_DURATION_FRAMES     (int)((int)TAKING_DAMAGE_ANIMATION_DURATION_MS / (int)MS_PER_FRAME)

#define WALK_ANIMATION_DURATION_MS                  500 
#define WALK_ANIMATION_DURATION_FRAMES              (int)((int)WALK_ANIMATION_DURATION_MS / (int)MS_PER_FRAME)
#define WALK_SPEED_PERC_PER_MS                      0.02f 
#define WALK_INCREMENT_PIXEL_PER_FRAME              ((MS_PER_FRAME*WALK_SPEED_PERC_PER_MS) / 100.0f) * SCREEN_WIDTH

#define FLY_ANIMATION_DURATION_MS                  700 
#define FLY_ANIMATION_DURATION_FRAMES              (int)((int)FLY_ANIMATION_DURATION_MS / (int)MS_PER_FRAME)
#define FLY_SPEED_PERC_PER_MS                      0.02f 
#define FLY_INCREMENT_PIXEL_PER_FRAME              ((MS_PER_FRAME*FLY_SPEED_PERC_PER_MS) / 100.0f) * SCREEN_WIDTH

#define DEFENDING_STATE_DURATION_MS                     500
#define DEFENDING_STATE_FRAMES                      (int)((int)INPUT_MODE_DURATION_MS / (int)MS_PER_FRAME)



#define ADD_ANCHORS(sprite_set, IMAGE_KIND, ...) \
    do { \
        int _anchors_[] = {__VA_ARGS__}; \
        assert(sizeof(_anchors_)/sizeof(_anchors_[0]) == (sprite_set).sprites[(IMAGE_KIND)].frame_num); \
        memcpy((sprite_set).sprites[(IMAGE_KIND)].anchors, _anchors_, sizeof(_anchors_)); \
    } while (0)

#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#define MAX(i, j) (((i) > (j)) ? (i) : (j))

typedef int thing_idx;

Vector2 default_orientation = {.x = 1, .y = 0};
const char CHARSET[] = 
    "abcdefghijklmnopqrstuvwxyz";   // lowercase
    // "ABCDEFGHIJKLMNOPQRSTUVWXYZ"   // uppercase
    // "ilh"   // lowercase
    // "Ii";
    // "ILH";   // uppercase
    // "!@#$%^&*()-_=+[]{};:,.<>/?"; // special characters



#define CHARSET_SIZE                                (sizeof(CHARSET)/sizeof(CHARSET[0]) - 1)
#define DEFEND_TEXT_CAPACITY                        8
typedef enum
{
    DEFAULT_TRAIT = 0,
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
    DEFAULT_ATTR = 0,
    IDLING = (1<<0),
    HITTING = (1<<1),
    MOVING = (1<<2),
    LOOKS_LEFT = (1<<3),
    INPUTTING = (1<<4),
    MOVING_FAST = (1<<5),
    INPUT_MOVE = (1<<6),
    FLYING = (1<<7),
    DEFENDING = (1<<8),
    TAKING_DAMAGE = (1<<9),
    // TAKING_OFF = (1<<7),
} Attributes;

Traits ENEMY_TRAITS_DEFAULT = ENEMY|NPC|POSITIONABLE|HAS_DIRECTION|CAN_HIT;
Traits PLAYER_TRAITS = HAS_DIRECTION | POSITIONABLE | CONTROLLABLE | CAN_HIT | CAN_FLY;


typedef enum{
    DEFAULT_THING_KIND = 0,
    KNIGHT,
    ORC,
    YAMABUSHI,
    GRID_CELL,
    THING_KIND_NUM
} ThingKind;

//TODO: change orientation and movement speed to velocity
//
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
    float reach; // in percent from total stage len
    size_t default_movement_speed_px;
    float height; // in percent from CELL_HEIGHT
    float width; // in percent from CELL_HEIGHT
    int cpm;     // simulated input speed for NPC
    char damage_text[DEFEND_TEXT_CAPACITY]; // text that thing inputted to successfylly attack
    char defend_text[DEFEND_TEXT_CAPACITY]; // text that thing must input to successfully defend
} Thing;

typedef enum
{
    DEFAULT_IMAGE,
    IDLE_IMAGE,
    ATTACK_IMAGE,
    WALK_IMAGE,
    JUMP_IMAGE,
    HURT_IMAGE,
    DEAD_IMAGE,
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
    size_t figure_height;
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
    int recorded_num;
    char input[HIT_TEXT_CAPACITY];
    thing_idx player_idx;
    thing_idx thing_num;
    size_t animation_num;
} Game;

void print_debug_attributes(Attributes attr)
{
    if (attr == DEFAULT_ATTR)
    {
        TraceLog(LOG_INFO, "attrs: DEFAULT_ATTR");
        return;
    }
    TraceLog(LOG_INFO, "attrs mask: 0x%X", attr);
    for (int bit = 0; bit < 32; bit++)
    {
        int flag = 1 << bit;
        if ((attr & flag) == 0) continue;
        switch (flag)
        {
            case IDLING: TraceLog(LOG_INFO, "  IDLING"); break;
            case HITTING: TraceLog(LOG_INFO, "  HITTING"); break;
            case MOVING: TraceLog(LOG_INFO, "  MOVING"); break;
            case LOOKS_LEFT: TraceLog(LOG_INFO, "  LOOKS_LEFT"); break;
            case INPUTTING: TraceLog(LOG_INFO, "  INPUTTING"); break;
            case MOVING_FAST: TraceLog(LOG_INFO, "  MOVING_FAST"); break;
            case INPUT_MOVE: TraceLog(LOG_INFO, "  INPUT_MOVE"); break;
            case FLYING: TraceLog(LOG_INFO, "  FLYING"); break;
            case DEFENDING: TraceLog(LOG_INFO, "  DEFENDING"); break;
            case TAKING_DAMAGE: TraceLog(LOG_INFO, "  TAKING_DAMAGE"); break;
            default: TraceLog(LOG_INFO, "  UNKNOWN_FLAG(0x%X)", flag); break;
        }
    }
}


void state_transition(Game* game, thing_idx idx)
{
    Thing* thing = &game->things[idx];
    thing->state_cnt = 0;
    game->recorded_num = 0;
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
    int* anchors // array indicates if anchor should be used or not
)
{
    size_t animation_idx = game->animation_num++;
    assert(animation_idx < MAX_ANIMATIONS);
    Sprite* sprite = &sprite_set.sprites[sprite_idx]; 
    // size_t anchors_num = sprite->frame_num;
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
    for(size_t anchor_index = 0; anchor_index < MAX_TEXTURES_PER_ANIMATION; anchor_index++)
    {
        if (anchors[anchor_index] == 0) continue;
        else frames_num++;

        Image cropped_image = ImageCopy(*img);
        size_t width = sprite->widths[anchor_index];
        // assert(width != 0);
        if (width == 0) width = sprite_set.figure_width;
        size_t anchor = anchors[anchor_index];
        assert(anchor != 0);
        Rectangle crop_rect = {.height = img->height, .width = width, .x = anchor - width/2, .y = img->height - sprite_set.figure_height}; 
        ImageCrop(&cropped_image, crop_rect);
        float resize_coef = (float)THING_HEIGHT_DEFAULT/cropped_image.height;   
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
    
    int anchors[MAX_TEXTURES_PER_ANIMATION] = {0};
    // for(int i = 0;i < MAX_SPRITES_PER_SPRITE_SHEET; i++) {use_anchors[i] = true;}
    for(ImageKind kind = 0; kind < IMAGE_KIND_NUM; kind++)
    {
        Sprite sprite = sprites.sprites[kind]; 
        if (sprites.sprites[kind].image_path == NULL) continue;
        Image image = LoadImage(sprites.sprites[kind].image_path);
        assert(image.width != 0);
        sprites.sprites[kind].image = image;
        assert(sprite.frame_num != 0);
        for(int i = 0;i < MAX_TEXTURES_PER_ANIMATION; i++) {anchors[i] = 0;}

        switch(kind)
        {
            case IDLE_IMAGE:
            {
                for(size_t i = 0;i < sprites.sprites[kind].frame_num; i++) {anchors[i] = sprites.sprites[kind].anchors[i];}
                num_of_animations = sprite_to_animation(game, traits, IDLING, sprites, IDLE_IMAGE, IDLE_DURATION_FRAMES, anchors);
                break;
            }   
            case ATTACK_IMAGE:
            {
                anchors[0] = sprites.sprites[kind].anchors[0];
                num_of_animations = sprite_to_animation(game, traits, INPUTTING, sprites, ATTACK_IMAGE, INPUT_MODE_DURATION_FRAMES, anchors);
                for(size_t i = 0;i < sprites.sprites[kind].frame_num; i++) {anchors[i] = sprites.sprites[kind].anchors[i];}
                anchors[0] = 0;
                num_of_animations = sprite_to_animation(game, traits, HITTING, sprites, ATTACK_IMAGE, HIT_DURATION_FRAMES, anchors);
                break;
            }   
            case WALK_IMAGE:
            {
                for(size_t i = 0;i < sprites.sprites[kind].frame_num; i++) {anchors[i] = sprites.sprites[kind].anchors[i];}
                num_of_animations = sprite_to_animation(game, traits, MOVING, sprites, WALK_IMAGE, WALK_ANIMATION_DURATION_FRAMES, anchors);
                break;
            }
            case JUMP_IMAGE:
            {
                size_t fly_anim_texture_index = 0;
                for(int i = 5;i <= 10; i++) 
                {
                    anchors[fly_anim_texture_index] = sprites.sprites[kind].anchors[i];
                    fly_anim_texture_index++;
                }
                for(int i = 10;i >= 5; i--) 
                {
                    anchors[fly_anim_texture_index] = sprites.sprites[kind].anchors[i];
                    fly_anim_texture_index++;
                }
                num_of_animations = sprite_to_animation(
                        game,
                        traits,
                        FLYING | IDLING | MOVING,
                        sprites,
                        JUMP_IMAGE,
                        FLY_ANIMATION_DURATION_FRAMES,
                        anchors);
                break;
            }
            case HURT_IMAGE:
            {
                for(size_t i = 0;i < sprites.sprites[kind].frame_num; i++) {anchors[i] = sprites.sprites[kind].anchors[i];}
                num_of_animations = sprite_to_animation(game, traits, DEFENDING, sprites, HURT_IMAGE, DEFEND_ANIMATION_DURATION_FRAMES, anchors);
                break;
            }
            case DEAD_IMAGE:
            {
                for(size_t i = 0;i < sprites.sprites[kind].frame_num; i++) {anchors[i] = sprites.sprites[kind].anchors[i];}
                num_of_animations = sprite_to_animation(game, traits, TAKING_DAMAGE, sprites, DEAD_IMAGE, TAKING_DAMAGE_ANIMATION_DURATION_FRAMES, anchors);
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

    int best_index = 0;
    int max_overlap = -1;

    for (size_t i = 0; i < game->animation_num; i++) {
        if (!(game->animations[i].kind == thing->kind)) continue;
        int overlap = count_bits(game->animations[i].attr & thing->attr);

        if (overlap > max_overlap) {
            max_overlap = overlap;
            best_index = i;
        }
    }

    return best_index;
}

// thing_idx get_state_duration(Game* game, thing_idx idx)
// {
//     Thing* thing = &game->things[idx];
//
//     int best_index = 0;
//     int max_overlap = -1;
//
//     for (size_t i = 0; i < game->animation_num; i++) {
//         if (!(game->animations[i].kind == thing->kind)) continue;
//         int overlap = count_bits(game->animations[i].attr & thing->attr);
//
//         if (overlap > max_overlap) {
//             max_overlap = overlap;
//             best_index = i;
//         }
//     }
//
//     return best_index;
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
    DrawText(render_text, player->position.x - text_len_px/2, player->position.y - HIT_TEXT_POSITION_Y, font_size, BLACK);

    // DrawLine(0, HIT_TEXT_POSITION_Y + HIT_TEXT_HEIGHT, SCREEN_WIDTH, HIT_TEXT_POSITION_Y + HIT_TEXT_HEIGHT, BLACK);
}

void draw_stage(Game* game)
{
    (void)game;
    DrawLine(LINE_NUMBER_OFFSET, STAGE_COORDINATE, SCREEN_WIDTH, STAGE_COORDINATE, BLACK);

    // static char render_text[2];  
    // size_t font_size = COLUMN_CELL_HEIGHT - 5;
    // render_text[1] = '\0';
    // for(size_t i = 0; i < game->thing_num; i++)
    // {
    //     Thing* thing = &game->things[i];
    //     if(thing->kind != GRID_CELL) continue;
    //     render_text[0] = game->hit_text[thing->hit_text_idx];
    //     int text_len_px = MeasureText(render_text, font_size);
    //     // DrawRectangleLines(thing->position.x - COLUMN_CELL_WIDTH/2, STAGE_COORDINATE, COLUMN_CELL_WIDTH, COLUMN_CELL_HEIGHT, RED);
    //     DrawText(render_text, thing->position.x - text_len_px/2, SCREEN_HEIGHT - font_size , font_size, BLACK);
    //     // RLAPI void DrawRectangle(int posX, int posY, int width, int height, Color color);
    // }
}

void draw_grid(Game* game)
{
    // DrawLine(0, STAGE_COORDINATE, SCREEN_WIDTH, STAGE_COORDINATE, BLACK);
    static char render_text[2];  
    size_t font_size = CELL_HEIGHT * 0.4;
    Color outline_color = BLACK;
    outline_color.a = GRID_TRANSPARENCY;
    render_text[1] = '\0';
    for(thing_idx i = 1; i <= game->thing_num; i++)
    {
        Thing* thing = &game->things[i];
        if(thing->kind != GRID_CELL) continue;
        render_text[0] = game->hit_text[thing->hit_text_idx];
        int text_len_px = MeasureText(render_text, font_size);
        DrawRectangleLines(thing->position.x - CELL_WIDTH/2.0f, thing->position.y - CELL_HEIGHT/2.0f, CELL_WIDTH, CELL_HEIGHT, outline_color);
        DrawText(render_text, thing->position.x - text_len_px/2.0f, thing->position.y - font_size/2.0f, font_size, outline_color);
        // RLAPI void DrawRectangle(int posX, int posY, int width, int height, Color color);
    }
}

bool draw_things(Game * game)
{
    for(thing_idx i = 0; i <= game->thing_num; i++)
    {
        Thing * thing = &game->things[i];
        if(thing->kind == DEFAULT_THING_KIND) continue;
        thing_idx animation_idx = get_animation_idx(game, i);
        if (animation_idx == 0) continue;
        Animation* animation = &game->animations[animation_idx];
        int state_duration = animation->duration_frames;
        if (state_duration == 0) continue;
        size_t animation_frame = (size_t)(((float)thing->state_cnt/(float)state_duration) * (float)animation->sprite_num);
        if (animation_frame >= animation->sprite_num) animation_frame = animation->sprite_num - 1;
        Texture2D* texture = &animation->textures[animation_frame];
        if (texture->id == 0) continue;
        Vector2 texture_position = {.x = thing->position.x - texture->width/2.0 ,.y = thing->position.y - texture->height};
        DrawTextureV(*texture, texture_position, WHITE);
#ifdef DEBUG_THINGS
        DrawCircle(thing->position.x , thing->position.y, 5, GREEN);
        Rectangle hitbox = {.height = CELL_HEIGHT * thing->height, .width = CELL_WIDTH * thing->width, .x = texture_position.x, .y = texture_position.y};
        Rectangle texture_outline = {.height = texture->height, .width = texture->width, .x = texture_position.x, .y = texture_position.y};
        DrawRectangleLinesEx(hitbox, 1, RED);
        DrawRectangleLinesEx(texture_outline, 1, BLUE);

        // Reach is shown as a vertical marker line at reach X on stage.
        if ((thing->traits & CAN_HIT) == CAN_HIT)
        {
            float reach_len_px = CELL_WIDTH * thing->reach;
            float dir_x = (thing->orientation.x < 0.0f) ? -1.0f : 1.0f;
            float reach_x = thing->position.x + dir_x * reach_len_px;
            DrawLineV((Vector2){reach_x, thing->position.y - CELL_HEIGHT},
                      (Vector2){reach_x, thing->position.y},
                      PURPLE);
        }
#endif //DEBUG_THINGS
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

bool is_in_reach(Game* game, thing_idx attacker_idx, thing_idx candidate_idx)
{
    bool res = false;
    Thing* attacker = &game->things[attacker_idx];
    Thing* candidate = &game->things[candidate_idx];
    // check for reach if attacker can attack 
    if ( ((attacker->traits & CAN_HIT) == CAN_HIT) && ((attacker->traits & ENEMY) != (candidate->traits & ENEMY)) )
    {
        if (fabs(candidate->position.y - attacker->position.y) > candidate->height*CELL_HEIGHT/2.0) return false;

        float reach_len_px = CELL_WIDTH * attacker->reach;
        float dir_x = (attacker->orientation.x < 0.0f) ? -1.0f : 1.0f;
        float reach_x = attacker->position.x + dir_x * reach_len_px;
        float min_x = MIN(attacker->position.x, reach_x);
        float max_x = MAX(attacker->position.x, reach_x);

        if ((candidate->position.x >= min_x) && (candidate->position.x <= max_x)) return true;
    }
    return res;
}

void calc_attributes(Game* game)
{
    for(thing_idx i = 1; i <= (thing_idx)game->thing_num; i++)
    {
        Thing* thing = &game->things[i];
           
        if (check_bitmask(thing->attr, HITTING))
        {
            int anim_idx = get_animation_idx(game, i);
            Animation* anim  = &game->animations[anim_idx];
            size_t current_state_dur = anim->duration_frames; 
            if ((current_state_dur == thing->state_cnt) && (thing->damage != 0))
            {
                for(thing_idx check_for_hit_thing_idx = 1; check_for_hit_thing_idx  <= (thing_idx)game->thing_num; check_for_hit_thing_idx++)
                {
                    if (i == check_for_hit_thing_idx) continue;  
                    if (is_in_reach(game, i, check_for_hit_thing_idx))
                    {
                        Thing* attacked = &game->things[check_for_hit_thing_idx]; 
                        attacked->attr = DEFENDING;
                        for (int char_idx = 0; char_idx < DEFEND_TEXT_CAPACITY; char_idx++) {attacked->defend_text[char_idx] = 0;}
                        for (int char_idx = 0; char_idx < thing->damage; char_idx++)
                        {
                            attacked->defend_text[char_idx] = thing->damage_text[char_idx];
                        }
                        state_transition(game, check_for_hit_thing_idx);
                    }
                }
            }
        }
        if ((thing->movement_speed != 0) && (!check_bitmask(thing->attr, MOVING)))
        {  
            thing->attr |= MOVING; 
        }
        else
        {
            thing->attr = clear_bit(thing->attr, MOVING);
        }
        if (!Vector2Equals(thing->orientation, default_orientation)) 
        {
            thing->attr |= LOOKS_LEFT;
        }
        else 
        {
            thing->attr = clear_bit(thing->attr, LOOKS_LEFT);
        }
        if (thing->position.y != STAGE_COORDINATE) 
        {
            thing->attr |= FLYING;
        } 
        else 
        {
            thing->attr = clear_bit(thing->attr, FLYING);
        }
    }   
}

void process_game(Game* game)
{
    calc_attributes(game);
    for(thing_idx i = 1; i <= (thing_idx)game->thing_num; i++)
    {
        Thing* thing = &game->things[i];
        int anim_idx = get_animation_idx(game, i);
        // if (anim_idx == -1) continue;
        Animation* anim  = &game->animations[anim_idx];
        size_t current_state_dur = anim->duration_frames; 
        
        if (check_bitmask(thing->attr, DEFENDING))
        {
            thing->movement_speed = 0;
            thing->damage = 0;
            for (int char_idx = 0; char_idx < DEFEND_TEXT_CAPACITY; char_idx++) {thing->damage_text[char_idx] = 0;}
            continue;
        }
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
                thing->hit_text_idx = (thing->hit_text_idx + 1) % HIT_TEXT_CAPACITY;
                thing->damage_text[thing->damage] = game->key_pressed;
                thing->damage += 1;
                continue;
            }
        }
        // if (check_bitmask(thing->attr, HITTING))
        // {
        //     if (current_state_dur == thing->state_cnt)
        //     {
        //         for (int char_idx = 0; char_idx < DEFEND_TEXT_CAPACITY; char_idx++) {thing->damage_text[char_idx] = 0;}
        //     }
        //     continue;
        // }
        
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
            continue;
        }
    }   
}   
char get_first_char(char* arr, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        if (arr[i] != 0) return arr[i];
    }
    return 0;
}

void increment_game(Game* game)
{
    for(thing_idx i = 1; i <= game->thing_num; i++)
    {
        Thing* thing = &game->things[i];
        size_t state_cnt = thing->state_cnt++;
        Attributes old_attr = thing->attr;
        int anim_idx = get_animation_idx(game, i);
        // if (anim_idx == -1) continue;
        Animation* anim  = &game->animations[anim_idx];
        size_t current_state_dur = anim->duration_frames;
        if (current_state_dur <= state_cnt) 
        {
            thing->damage = 0;
            if(thing->movement_speed == 0)
            {
                thing->attr = clear_bit(thing->attr, MOVING);
                state_transition(game, i);
            }
            else
            {
                thing->attr |= MOVING;
                state_transition(game, i);
            }
            // if (check_bitmask(thing->attr, DEFENDING) && (get_first_char(thing->defend_text, DEFEND_TEXT_CAPACITY) != 0))
            if (check_bitmask(thing->attr, DEFENDING))
            {
                thing->attr = TAKING_DAMAGE;
                // thing->attr = clear_bit(thing->attr, DEFENDING);
                state_transition(game, i);
                // continue;
            }
            if (check_bitmask(thing->attr, DEFAULT_ATTR))
            {
                thing->attr = IDLING;
                state_transition(game, i);
            }

        }
        Attributes new_attr = thing->attr;
#ifdef DEBUG_ATTR
        if (new_attr != old_attr)
        {
            if(thing->kind == ORC)
            {
                TraceLog(LOG_INFO,  "Attributes changed for index %d: old: %d, new: %d ", i, old_attr, new_attr);
                print_debug_attributes(new_attr);
            }
        }
#endif //DEBUG_ATTR
    }

}

void init_player(Game* game, thing_idx idx)
{
    assert(idx == game->thing_num + 1);
    Vector2 player_position = { SCREEN_WIDTH/2.0, STAGE_COORDINATE };
    game->things[idx].position = player_position;
    game->things[idx].orientation.x = 1;
    game->things[idx].orientation.y = 0;
    game->things[idx].attr = IDLING;
    game->things[idx].movement_speed = 0;
    game->things[idx].traits = PLAYER_TRAITS;
    game->things[idx].kind = YAMABUSHI;
    // game->things[idx].kind = KNIGHT;
    game->thing_num++;
}

void init_orc(Game* game)
{
    Vector2 position = {3 * SCREEN_WIDTH/4, STAGE_COORDINATE};
    thing_idx idx = ++game->thing_num;
    game->things[idx].position = position;
    game->things[idx].orientation.x = 1;
    game->things[idx].orientation.y = 0;
    game->things[idx].traits = ENEMY_TRAITS_DEFAULT;
    game->things[idx].kind = ORC;
} 


void init_knight_enemy(Game* game)
{
    Vector2 position = {SCREEN_WIDTH/4, STAGE_COORDINATE};
    thing_idx idx = ++game->thing_num;
    game->things[idx].position = position;
    game->things[idx].orientation.x = 1;
    game->things[idx].orientation.y = 0;
    game->things[idx].traits = ENEMY_TRAITS_DEFAULT;
    game->things[idx].kind = KNIGHT;
}

Game init_game()
{
    Game game = {0};
    //default texture is useless curently
    Image default_texture_image = GenImageColor(CELL_WIDTH, CELL_HEIGHT, PURPLE);
    game.animations[0].textures[0] = LoadTextureFromImage(default_texture_image);
    assert( game.animations[0].textures[0].width == CELL_WIDTH);
    game.animations[0].sprite_num = 1;
    game.animation_num++;

    generate_hit_text(&game);

    game.player_idx = 1;
    init_player(&game, game.player_idx);
    init_orc(&game);

    { 
        SpriteSet set = {0};
        set.kind = KNIGHT;
        set.sprites[IDLE_IMAGE].image_path = "assets/Knight_3/Idle.png";
        set.sprites[ATTACK_IMAGE].image_path = "assets/Knight_3/Attack 2.png";
        set.sprites[WALK_IMAGE].image_path = "assets/Knight_2/Walk.png";
        set.sprites[IDLE_IMAGE].frame_num = 4;
        set.sprites[ATTACK_IMAGE].frame_num = 4;
        set.sprites[WALK_IMAGE].frame_num = 8;
        //fix this
        set.figure_width = 100;
        set.figure_height = 64;
        ADD_ANCHORS(set, IDLE_IMAGE, 64, 192, 320, 448);
        ADD_ANCHORS(set, WALK_IMAGE, 64, 192, 320, 448, 576, 704, 832, 960);
        ADD_ANCHORS(set, ATTACK_IMAGE, 64, 192, 320, 448);
        load_animations(&game, set, PLAYER_TRAITS);
    }
    {
        SpriteSet set = {0};
        set.kind = ORC;
        set.figure_height = 65;
        set.sprites[IDLE_IMAGE].image_path = "assets/Craftpix_Orc/Orc_Berserk/Idle.png";
        set.sprites[ATTACK_IMAGE].image_path = "assets/Craftpix_Orc/Orc_Berserk/Attack_1.png";
        set.sprites[WALK_IMAGE].image_path = "assets/Craftpix_Orc/Orc_Berserk/Walk.png";
        set.sprites[HURT_IMAGE].image_path = "assets/Craftpix_Orc/Orc_Berserk/Hurt.png";
        set.sprites[DEAD_IMAGE].image_path = "assets/Craftpix_Orc/Orc_Berserk/Dead.png";
        set.sprites[IDLE_IMAGE].frame_num = 5;
        set.sprites[ATTACK_IMAGE].frame_num = 4;
        set.sprites[WALK_IMAGE].frame_num = 7;
        set.sprites[HURT_IMAGE].frame_num = 2;
        set.sprites[DEAD_IMAGE].frame_num = 4;
        set.figure_width = 64; 

        ADD_ANCHORS(set, IDLE_IMAGE, 48, 144, 240, 336, 432);
        ADD_ANCHORS(set, WALK_IMAGE, 48, 144, 240, 336, 432, 528, 624);
        ADD_ANCHORS(set, ATTACK_IMAGE, 45, 140, 245, 341);
        ADD_ANCHORS(set, HURT_IMAGE, 48, 144);
        ADD_ANCHORS(set, DEAD_IMAGE, 48, 144, 240, 336);
        load_animations(&game, set, PLAYER_TRAITS);
    }
    { 
        SpriteSet set = {0};
        set.kind = YAMABUSHI;
        set.figure_height = 100;
        set.sprites[IDLE_IMAGE].image_path = "assets/Yamabushi/Idle.png";
        set.sprites[ATTACK_IMAGE].image_path = "assets/Yamabushi/Attack_1.png";
        set.sprites[WALK_IMAGE].image_path = "assets/Yamabushi/Walk.png";
        set.sprites[JUMP_IMAGE].image_path = "assets/Yamabushi/Jump.png";
        set.sprites[HURT_IMAGE].image_path = "assets/Yamabushi/Hurt.png";
        set.sprites[DEAD_IMAGE].image_path = "assets/Yamabushi/Dead.png";
        set.sprites[IDLE_IMAGE].frame_num = 6;
        set.sprites[ATTACK_IMAGE].frame_num = 3;
        set.sprites[WALK_IMAGE].frame_num = 8;
        set.sprites[JUMP_IMAGE].frame_num = 15;
        set.sprites[HURT_IMAGE].frame_num = 3;
        set.sprites[DEAD_IMAGE].frame_num = 6;
        set.figure_width = 100; 

        ADD_ANCHORS(set, IDLE_IMAGE, 68, 200, 324, 452, 580, 708);
        ADD_ANCHORS(set, WALK_IMAGE, 64, 192, 320, 448, 576, 704, 832, 960);
        ADD_ANCHORS(set, ATTACK_IMAGE, 55, 189, 313);
        ADD_ANCHORS(set, JUMP_IMAGE, 64, 192, 320, 448, 576, 704, 832, 960, 1088, 1216, 1344, 1472, 1600, 1728, 1856);
        ADD_ANCHORS(set, HURT_IMAGE, 64, 192, 320);
        ADD_ANCHORS(set, DEAD_IMAGE, 64, 192, 320, 448, 576, 704);
        for (size_t i = 0; i < set.sprites[JUMP_IMAGE].frame_num; i++)
        {
            set.sprites[JUMP_IMAGE].widths[i] = (int)set.figure_width;
        }
        for (size_t i = 0; i < set.sprites[ATTACK_IMAGE].frame_num; i++)
        {
            set.sprites[ATTACK_IMAGE].widths[i] = (int)set.figure_width;
        }

        set.sprites[ATTACK_IMAGE].widths[1] = set.figure_width + 30;
        load_animations(&game, set, PLAYER_TRAITS);
    }
    for(thing_idx i = 1; i <= game.thing_num; i++)
    {
        {
            // calculate reach from attack animation size
            Thing* thing = &game.things[i];
            thing->attr = HITTING;
            thing_idx anim_idx = get_animation_idx(&game, i);
            Animation* anim = &game.animations[anim_idx];
            int max_attack_width = 0;
            for (size_t frame = 0; frame < anim->sprite_num; frame++)
            {
                Texture2D* attack_texture = &anim->textures[frame];
                if (attack_texture->id == 0) continue;
                if ((int)attack_texture->width > max_attack_width) max_attack_width = attack_texture->width;
            }
            if (max_attack_width == 0) max_attack_width = anim->textures[0].width;
            thing->reach = (float)max_attack_width / (float)CELL_WIDTH / 2.0f;
        }
        {
            // calculate hitbox from idle animation size
            Thing* thing = &game.things[i];
            thing->attr = IDLING;
            thing_idx anim_idx = get_animation_idx(&game, i);
            Animation* anim = &game.animations[anim_idx];
            thing->height = (float)(anim->textures[0].height) / (float)(CELL_HEIGHT);
            thing->width = (float)(anim->textures[0].width) / (float)(CELL_WIDTH);
        }
    }
    for (int column = 0; column < (int)GRID_X; column++)
    {
        for (int line = 0; line < (int)GRID_Y; line++)
        {
            Thing* thing = &game.things[++game.thing_num]; 
            thing->position.x = LINE_NUMBER_OFFSET + CELL_WIDTH*column + CELL_WIDTH/2.0;
            thing->position.y = CELL_HEIGHT*line + CELL_HEIGHT/2.0;
            thing->kind = GRID_CELL;
            int idx = 0;
            idx = rand() % HIT_TEXT_CAPACITY;
            thing->hit_text_idx = idx;
        }
    }
    return game;
}

void draw_game(Game* game)
{
    draw_stage(game);
    draw_grid(game);
    draw_hit_text(game);
    draw_things(game);
}

bool is_num_pressed(char key)
{
    // KEY_ZERO            = 48,       // Key: 0
    // KEY_ONE             = 49,       // Key: 1
    // KEY_TWO             = 50,       // Key: 2
    // KEY_THREE           = 51,       // Key: 3
    // KEY_FOUR            = 52,       // Key: 4
    // KEY_FIVE            = 53,       // Key: 5
    // KEY_SIX             = 54,       // Key: 6
    // KEY_SEVEN           = 55,       // Key: 7
    // KEY_EIGHT           = 56,       // Key: 8
    // KEY_NINE            = 57,       // Key: 9
    if ((key >= KEY_ZERO) && (key <= KEY_NINE)) return true;
    return false;
}

void process_input(Game* game)
{
    Thing* player = &game->things[game->player_idx];
    // make sure that hit animation and input animation is not canceled by input
    if ((check_bitmask(player->attr, INPUTTING)) || (check_bitmask(player->attr, HITTING))) return;
    if ((game->key_pressed == 'i') || (is_num_pressed(game->key_pressed)))
    {
        if ((check_bitmask(player->attr, IDLING) || (check_bitmask(player->attr, MOVING))))
        {
            player->attr = INPUTTING;
            state_transition(game, game->player_idx);
            game->key_pressed = 0;
            if (is_num_pressed(game->key_pressed)) 
            {
                int key_num = game->key_pressed - 48;
                game->recorded_num = game->recorded_num*10 + key_num;
            }
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
            float dist_to_player = 0;
            if ((thing->traits & CAN_FLY) == CAN_FLY) dist_to_player = Vector2Distance(thing->position, player->position);
            else dist_to_player = fabs(thing->position.x - player->position.x);
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
