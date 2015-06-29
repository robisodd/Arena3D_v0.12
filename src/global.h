#pragma once
#include "pebble.h"

  
#define IF_COLOR(x)     COLOR_FALLBACK(x, (void)0)
#define IF_BW(x)        COLOR_FALLBACK((void)0, x)
#define IF_BWCOLOR(x,y) COLOR_FALLBACK(x, y)
  
//#define mapsize 21             // Map is mapsize * mapsize squares big
#define mapsize 5             // Map is mapsize * mapsize squares big
#define MAX_TEXTURES 15        // Most number of textures there's likely to be.  Feel free to increase liberally, but no more than 254.
#define MAX_SQUARETYPES 10     // Most number of different map square types (square type 0 is always out-of-bounds plus wherever else on the map uses square type 0)
#define IDCLIP false           // Walk thru walls
#define view_border true       // Draw border around 3D viewing window

#define Format1Bit 0  // Note: On Color, all 1bit images need to be converted to GBitmapFormat1BitPalette
#define Format2Bit 1
#define Format4Bit 2
// Note: 64-color textures not supported (well, no >16 color textures)
  
typedef struct SquareTypeStruct {
  uint8_t face[4]; // texture[] number
  uint8_t ceiling; // texture[] number (255 = no ceiling / sky.  Well, 255 or any number >MAX_TEXTURES)
  uint8_t floor;   // texture[] number (255 = no floor texture)
  // other characteristics like walk thru and stuff
} SquareTypeStruct;

typedef struct PlayerStruct {
  int32_t x;                  // Player's X Position (64 pixels per square)
  int32_t y;                  // Player's Y Position (64 pixels per square)
  int16_t facing;             // Player Direction Facing (from 0 - TRIG_MAX_ANGLE)
} PlayerStruct;

typedef struct ObjectStruct {
  int32_t x;                  // Player's X Position (64 pixels per square)
  int32_t y;                  // Player's Y Position (64 pixels per square)
  int16_t health;             //
  uint8_t type;               // Enemy, Lamp, Coin, etc
  uint8_t sprite;             // sprite_image[] and sprite_mask[] for object
  int32_t data1;              // 
  int32_t data2;              // 
} ObjectStruct;

typedef struct RayStruct {
   int32_t x;                 // x coordinate the ray hit
   int32_t y;                 // y coordinate the ray hit
  uint32_t dist;              // length of the ray, i.e. distance ray traveled
   uint8_t hit;               // square_type the ray hit [0-127]
   int32_t offset;            // horizontal spot on texture the ray hit [0-63] (used in memory pointers so int32_t)
   uint8_t face;              // face of the block it hit (00=East Wall, 01=North, 10=West, 11=South Wall)
} RayStruct;


typedef union TextureStruct {
  struct {
  uint8_t format;        // (color only)            texture type    (0=Format1Bit, 1=Format2Bit, 2=Format4Bit)
  GBitmap *bmp;          //              Pointer to texture
  uint8_t *data;         //              Pointer to texture data
  uint8_t *palette;      // (color only) Pointer to texture palette

  uint8_t width;
  uint8_t height;
  uint8_t bytes_per_row; //
  uint8_t pixels_per_byte; // Technically stores bit shifted. So instead of [8px/B, 4px/B, 2px/B] it stores [3,2,1] (8=1<<3, 4=1<<2, 2=1<<1)
  uint8_t colormax;       // largest color in palette.  it is (1<<XbitPalette)-1 (e.g. =15 in 4bit|16color palette, =3 in 2bit|4color palette.)
  };

// Format     (Bytes/Row)  (Bytes/Row)/2   px/byte   px/byte-1(mask)   bit/px  number_of_colors-1     Bytes/Row  px/byte px/byte-1   bit/px  number_of_colors-1
//0 = 1bit      <<3= *8        4-1= 3      >>3 = /8       7            <<0=*1       2-1= 1                3         3       7        0            1
//1 = 2bit      <<4=*16        8-1= 7      >>2 = /4       3            <<1=*2       4-1= 3                4         2       3        1            3
//2 = 4bit      <<5=*32       16-1=15      >>1 = /2       1            <<2=*4      16-1=15                5         1       1        2            15
//3 = invalid                                                          <<3=*8       1-1=&0                                           3

  struct {
  uint8_t bits_per_pixel;
  };
} TextureStruct;


int32_t sqrt32(int32_t a);
int32_t sqrt_int(int32_t a, int8_t depth);

// absolute value
int32_t abs32(int32_t x);
int16_t abs16(int16_t x);
int8_t  abs8 (int8_t  x);
int32_t abs_int(int32_t a);

// sign function returns: -1 or 0 or 1 if input is <0 or =0 or >0
int8_t  sign8 (int8_t  x);
int16_t sign16(int16_t x);
int32_t sign32(int32_t x);


void LoadMapTextures();
void UnLoadMapTextures();
void GenerateSquareMap();
void GenerateRandomMap();
void GenerateMazeMap(int32_t startx, int32_t starty);

void walk(int32_t direction, int32_t distance);  
void shoot_ray(int32_t start_x, int32_t start_y, int32_t angle);

uint8_t getmap(int32_t x, int32_t y);
void setmap(int32_t x, int32_t y, uint8_t value);


void draw_textbox(GContext *ctx, GRect textframe, char *text);
void draw_map(GContext *ctx, GRect box, int32_t zoom);
void draw_3D(GContext *ctx, GRect box); //, int32_t zoom);

