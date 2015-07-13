#include "global.h"

uint8_t map[MAP_SIZE * MAP_SIZE];  // 0-255, 0-127 are squaretypes[], high bit set means ray hits wall
TextureStruct texture[MAX_TEXTURES];
TextureStruct sprite[7];

GBitmap *sprite_image[1];
GBitmap *sprite_mask[1];

SquareTypeStruct squaretype[MAX_SQUARETYPES]; // note squaretype[0]=out of bounds ceiling/floor rendering
PlayerStruct player;
ObjectStruct object[MAX_OBJECTS];
RayStruct ray;


// square root
#define root_depth 10          // How many iterations square root function performs
int32_t sqrt32(int32_t a) {int32_t b=a; for(int8_t i=0; i<root_depth; i++) b=(b+(a/b))/2; return b;} // Square Root
int32_t sqrt_int(int32_t a, int8_t depth){int32_t b=a; for(int8_t i=0; i<depth; i++) b=(b+(a/b))/2; return b;} // Square Root

// absolute value
int32_t abs32(int32_t x) {return (x^(x>>31)) - (x>>31);}
int16_t abs16(int16_t x) {return (x^(x>>15)) - (x>>15);}
int8_t  abs8 (int8_t  x) {return (x^(x>> 7)) - (x>> 7);}
int32_t abs_int(int32_t a){return (a<0 ? 0 - a : a);} // Absolute Value (might be faster than above)

// sign function returns: -1 or 0 or 1 if input is <0 or =0 or >0
int8_t  sign8 (int8_t  x){return (x > 0) - (x < 0);}
int16_t sign16(int16_t x){return (x > 0) - (x < 0);}
int32_t sign32(int32_t x){return (x > 0) - (x < 0);}

uint8_t log16 (uint16_t x){uint8_t l=0; while (x>>=1) l++; return l;}
  

// get_gbitmapformat_text from: https://github.com/rebootsramblings/GBitmap-Colour-Palette-Manipulator/blob/master/src/gbitmap_color_palette_manipulator.c
#ifdef PBL_COLOR
char* get_gbitmapformat_text(GBitmapFormat format) {
	switch (format) {
		case GBitmapFormat1Bit: return "GBitmapFormat1Bit";
		case GBitmapFormat8Bit: return "GBitmapFormat8Bit";
		case GBitmapFormat1BitPalette: return "GBitmapFormat1BitPalette";
		case GBitmapFormat2BitPalette: return "GBitmapFormat2BitPalette";
		case GBitmapFormat4BitPalette: return "GBitmapFormat4BitPalette";
		default: return "UNKNOWN FORMAT";
	}
}
#else
char* get_gbitmapformat_text(GBitmapFormat format) {
  return "B&W";
}
#endif

#ifdef PBL_COLOR
  GColor black_white_palette[2]={{.argb=0b11000000},{.argb=0b11111111}};  // Used when converting GBitmapFormat1Bit (OG Pebble) to GBitmapFormat1BitPalette (Color Pebble)
#endif
uint8_t blank_palette = IF_BWCOLOR(0b00000000, 0b11000011);
uint8_t blank_texture[] = {0,0};

void LoadMapTextures() {
  for(uint8_t i=0; i<MAX_TEXTURES; ++i) {
    texture[i].data = blank_texture + 1; // +1 cause the wall rendering does -1.
    texture[i].palette = &blank_palette; // Blank palette in which to render
    texture[i].format=3;                 // Currently Format 3 is "unknown" or blank
    texture[i].bytes_per_row=33;         // 33 to fully shift 32bit ray.offset (y on texture) to 0
    texture[i].pixels_per_byte = 8;      // pixels_per_byte is set for 8 to shift mapxmapy&63 to 0
    texture[i].colormax=0;               // Blank has only 1 color (index starts at 0, so max is 0)
  }
  
//   const int Texture_Resources1[] = {
//     RESOURCE_ID_STONE,          //0
//    RESOURCE_ID_WALL_FIFTY,      //1
//     RESOURCE_ID_WALL_CIRCLE,    //1
//     RESOURCE_ID_FLOOR_TILE,     //2
//     RESOURCE_ID_CEILING_LIGHTS, //3
//     RESOURCE_ID_WALL_BRICK,     //4
//     //RESOURCE_ID_GRASS,
//     RESOURCE_ID_REDBRICK,       //5
//     RESOURCE_ID_WOOD16,         //6
//     RESOURCE_ID_PrizeBox,       //7
//     RESOURCE_ID_GRASS64,        //8
//     RESOURCE_ID_TEST2           //9
//   };
  
  const int Texture_Resources[] = {
    RESOURCE_ID_REDBRICK,
    RESOURCE_ID_WOOD,
    RESOURCE_ID_GRASS,
    RESOURCE_ID_STONE,
    RESOURCE_ID_PrizeBox,
    RESOURCE_ID_TILE
  };
  
  if(app_logging) APP_LOG(APP_LOG_LEVEL_DEBUG, "Loading Textures");
  for(uint8_t i=0, NumberOfTextures=sizeof(Texture_Resources)/sizeof(Texture_Resources[0]); i<NumberOfTextures; ++i) {
    //if(app_logging) APP_LOG(APP_LOG_LEVEL_DEBUG, "Loading: %d", i);
    texture[i].bmp = gbitmap_create_with_resource(Texture_Resources[i]);

    if(texture[i].bmp==NULL) {
      if(app_logging) APP_LOG(APP_LOG_LEVEL_DEBUG, "Texture %d: Failed to load texture", i);
    } else {
      if(app_logging) APP_LOG(APP_LOG_LEVEL_DEBUG, "Texture %d: %d (2<<%d) Bytes/row, %s", i, gbitmap_get_bytes_per_row(texture[i].bmp), (uint16_t)log16(gbitmap_get_bytes_per_row(texture[i].bmp)), get_gbitmapformat_text(gbitmap_get_format(texture[i].bmp)));
      texture[i].width  = gbitmap_get_bounds(texture[i].bmp).size.w;
      texture[i].height = gbitmap_get_bounds(texture[i].bmp).size.h;
      texture[i].data   = gbitmap_get_data(texture[i].bmp);
      
      #ifdef PBL_COLOR
        if(gbitmap_get_format(texture[i].bmp)==GBitmapFormat1Bit) {
          gbitmap_set_data(texture[i].bmp, texture[i].data, GBitmapFormat1BitPalette, gbitmap_get_bytes_per_row(texture[i].bmp), true);
          gbitmap_set_palette(texture[i].bmp, black_white_palette, false);
        }
        //if(app_logging) APP_LOG(APP_LOG_LEVEL_DEBUG, "%d: %d %s %lx", i, gbitmap_get_bytes_per_row(texture[i]), get_gbitmapformat_text(gbitmap_get_format(texture[i])), (uint32_t)gbitmap_get_palette(texture[i]));
        texture[i].palette = (uint8_t*)gbitmap_get_palette(texture[i].bmp);
        switch (gbitmap_get_format(texture[i].bmp)) {
          //case GBitmapFormat1Bit: break;
          //case GBitmapFormat8Bit: break;
          case GBitmapFormat1BitPalette: texture[i].format=0; texture[i].bytes_per_row=3; texture[i].pixels_per_byte = 3; texture[i].colormax= 1; break;
          case GBitmapFormat2BitPalette: texture[i].format=1; texture[i].bytes_per_row=4; texture[i].pixels_per_byte = 2; texture[i].colormax= 3; break;
          case GBitmapFormat4BitPalette: texture[i].format=2; texture[i].bytes_per_row=5; texture[i].pixels_per_byte = 1; texture[i].colormax=15; break;
          default: if(app_logging) APP_LOG(APP_LOG_LEVEL_DEBUG, "Texture %d: Unsupported Format", i); break;//Crash("Unsupported Texture Format");
        }
      #endif
      texture[i].bytes_per_row= log16(gbitmap_get_bytes_per_row(texture[i].bmp));
/*
Format  (Bytes/Row)  (Bytes/Row)/2   px/byte   px/byte-1(mask)   bit/px  number_of_colors-1     Bytes/Row  px/byte px/byte-1   bit/px  number_of_colors-1
1bit      <<3= *8        4-1= 3      >>3 = /8       7            <<0=*1       2-1= 1                3         3       7        0            1
2bit      <<4=*16        8-1= 7      >>2 = /4       3            <<1=*2       4-1= 3                4         2       3        1            3
4bit      <<5=*32       16-1=15      >>1 = /2       1            <<2=*4      16-1=15                5         1       1        2            15
invalid                                                                     1-1=&0
*/
        
    // popup message
    // Test:
    // Is width & height = 64px
    // If full 64 color image, disregard as 3D function won't render it
    // is successful at loading into memory
    }
  }
  
  // TODO: Change to: SPRITE[number of sprites][2] (image/mask)
  //sprite_image[0] = gbitmap_create_with_resource(RESOURCE_ID_SPRITE_SMILEY);
  //sprite_mask[0] = gbitmap_create_with_resource(RESOURCE_ID_SPRITE_SMILEY_MASK);
  const int Sprite_Resources[] = {
    RESOURCE_ID_TROGDOR,
    //RESOURCE_ID_SPRITE_64_1B,
    //RESOURCE_ID_SPRITE_32_1B,
    //RESOURCE_ID_SPRITE_32_2B,
    //RESOURCE_ID_SPRITE_32_4B,
    //RESOURCE_ID_SPRITE_64_2B,
    //RESOURCE_ID_SPRITE_64_4B,
    RESOURCE_ID_BALL_8,
    RESOURCE_ID_BALL_16,
    RESOURCE_ID_BALL_32,
    RESOURCE_ID_BALL_64
  };

  if(app_logging) APP_LOG(APP_LOG_LEVEL_DEBUG, "Loading Sprites");
  
  //sprite[0].bmp = gbitmap_create_with_resource(RESOURCE_ID_SPRITE_SMILEY);
  //sprite[1].bmp = gbitmap_create_with_resource(RESOURCE_ID_SPRITE_SMILEY_MASK);
  //sprite[2].bmp = gbitmap_create_with_resource(RESOURCE_ID_TROGDOR);

  //for(uint8_t i=0; i<3; ++i) {
  for(uint8_t i=0, NumberOfSprites=sizeof(Sprite_Resources)/sizeof(Sprite_Resources[0]); i<NumberOfSprites; ++i) {
    sprite[i].bmp = gbitmap_create_with_resource(Sprite_Resources[i]);
    
    if(sprite[i].bmp==NULL) {
      if(app_logging) APP_LOG(APP_LOG_LEVEL_DEBUG, "Sprite %d: Failed to load sprite", i);
    } else {
      sprite[i].data = (uint8_t*)gbitmap_get_data(sprite[i].bmp);
      //GRect rect = gbitmap_get_bounds(sprite[i].bmp);
      sprite[i].width = gbitmap_get_bounds(sprite[i].bmp).size.w;
      sprite[i].height = gbitmap_get_bounds(sprite[i].bmp).size.h;
      if(app_logging) APP_LOG(APP_LOG_LEVEL_DEBUG, "Sprite %d: %d Bytes/row = 2<<%d (%d x %d)", i, gbitmap_get_bytes_per_row(sprite[i].bmp), (uint16_t)log16(gbitmap_get_bytes_per_row(sprite[i].bmp)), sprite[i].width, sprite[i].height);
      
      #ifdef PBL_COLOR
        if(gbitmap_get_format(sprite[i].bmp)==GBitmapFormat1Bit) {
        gbitmap_set_data(sprite[i].bmp, sprite[i].data, GBitmapFormat1BitPalette, gbitmap_get_bytes_per_row(sprite[i].bmp), true);
        gbitmap_set_palette(sprite[i].bmp, black_white_palette, false);
      }
      sprite[i].palette = (uint8_t*)gbitmap_get_palette(sprite[i].bmp);
      switch (gbitmap_get_format(sprite[i].bmp)) {
        //case GBitmapFormat1Bit: break;
        //case GBitmapFormat8Bit: break;
        case GBitmapFormat1BitPalette: sprite[i].format=0; sprite[i].bytes_per_row=3; sprite[i].pixels_per_byte = 3; sprite[i].colormax= 1; break;
        case GBitmapFormat2BitPalette: sprite[i].format=1; sprite[i].bytes_per_row=4; sprite[i].pixels_per_byte = 2; sprite[i].colormax= 3; break;
        case GBitmapFormat4BitPalette: sprite[i].format=2; sprite[i].bytes_per_row=5; sprite[i].pixels_per_byte = 1; sprite[i].colormax=15; break;
        default: if(app_logging) APP_LOG(APP_LOG_LEVEL_DEBUG, "Sprite %d: Unsupported Format", i); break;//Crash("Unsupported Texture Format");
      }
      #endif
        sprite[i].bytes_per_row= log16(gbitmap_get_bytes_per_row(sprite[i].bmp));
    }
  } // end for
 sprite_image[0] = sprite[0].bmp;
 sprite_mask[0]  = sprite[1].bmp;
}

void UnLoadMapTextures() {
  for(uint8_t i=0; i<MAX_TEXTURES; i++)
    if(texture[i].bmp)
      gbitmap_destroy(texture[i].bmp);
  
  gbitmap_destroy(sprite[0].bmp);
}

void GenerateSquareMap() {
  //Type 0 is how to render out-of-bounds
   squaretype[0].ceiling = 10; // outside sky
   squaretype[0].floor   =  2; // outside grass
   squaretype[0].face[0]=squaretype[0].face[1]=squaretype[0].face[2]=squaretype[0].face[3] = 10; // Outside wall (at infinate distance so 1px high)

   squaretype[1].face[0]=squaretype[1].face[1]=squaretype[1].face[2]=squaretype[1].face[3] = 0;
   squaretype[1].ceiling = 3;
   squaretype[1].floor   = 1;
  
//   squaretype[0].face[0]=squaretype[0].face[1]=squaretype[0].face[2]=squaretype[0].face[3]=squaretype[0].floor=squaretype[0].ceiling=0;
//   squaretype[1].face[0]=squaretype[1].face[1]=squaretype[1].face[2]=squaretype[1].face[3]=squaretype[1].floor=squaretype[1].ceiling=1;
//   squaretype[2].face[0]=squaretype[2].face[1]=squaretype[2].face[2]=squaretype[2].face[3]=squaretype[2].floor=squaretype[2].ceiling=2;
//   squaretype[3].face[0]=squaretype[3].face[1]=squaretype[3].face[2]=squaretype[3].face[3]=squaretype[3].floor=squaretype[3].ceiling=3;
  
  for (int16_t i=0; i<MAP_SIZE*MAP_SIZE; i++)
    map[i] = 1;                            // inside floor/ceiling
  
  for (int16_t i=0; i<MAP_SIZE; i++) {
    map[i*MAP_SIZE + 0]           = 128+1;  // west wall
    map[i*MAP_SIZE + MAP_SIZE - 1] = 128+1;  // east wall
    map[i]                       = 128+1;  // north wall
    map[(MAP_SIZE-1)*MAP_SIZE + i] = 128+1;  // south wall
  }
  map[((MAP_SIZE/2) * MAP_SIZE) + (MAP_SIZE/2)] = 128+1;  // middle block

  
   player.x = 1 * 64; player.y = (64*MAP_SIZE)/2; player.facing=0;    // start inside
   //object.x = 2 * 64; object.y = (64*MAP_SIZE)/2; object.facing=0;    // sprite position
  for(uint8_t i=0; i<MAX_OBJECTS; ++i) {
    object[i].x = rand()%(64*MAP_SIZE); object[i].y = rand()%(64*MAP_SIZE); object[i].facing=0; object[i].sprite=rand()%5;    // sprite position
  }
  
   //player.x = 6 * 32 + 16; player.y = (64*MAP_SIZE)/2; player.facing=TRIG_MAX_ANGLE/2;    // start inside
   //object.x = 3 * 32;      object.y = (64*MAP_SIZE)/2; object.facing=0;    // sprite position
//  player.x = ((64*MAP_SIZE)/2)-64; player.y = (64*MAP_SIZE)/2; player.facing=0;    // start inside
//  object.x = (64*MAP_SIZE)/2; object.y = (64*MAP_SIZE)/2; object.facing=0;    // sprite position
  //setmap(object.x, object.y, 0);
}

void GenerateRandomMap() {
  //squaretype[0].face[0] = 0; squaretype[0].face[1] = 0; squaretype[0].face[2] = 0; squaretype[0].face[3] = 0;
  squaretype[0].ceiling = 255;
  squaretype[0].floor = 6;

  squaretype[1].face[0] = 0;
  squaretype[1].face[1] = 5;
  squaretype[1].face[2] = 5;
  squaretype[1].face[3] = 0;
  squaretype[1].ceiling = 4;
  squaretype[1].floor = 3;
  
  //squaretype[2].face[0] = 0; squaretype[2].face[1] = 0; squaretype[2].face[2] = 0; squaretype[2].face[3] = 0;
  squaretype[2].ceiling = 2;
  squaretype[2].floor = 4;

  
  for (int16_t i=0; i<MAP_SIZE*MAP_SIZE; i++) map[i] = rand() % 3 == 0 ? 128+1 : 1;       // Randomly 1/3 of spots are [type 2] blocks, the rest are [type 1]
  for (int16_t i=0; i<MAP_SIZE*MAP_SIZE; i++) if(map[i]==1 && rand()%10==0) map[i]=128+2; // Change 10% of [type 2] blocks to [type 3] blocks
  //for (int16_t i=0; i<MAP_SIZE*MAP_SIZE; i++) if(map[i]==2 && rand()%2==0) map[i]=3;  // Changes 50% of [type 2] blocks to [type 3] blocks
}

// Generates maze starting from startx, starty, filling map with (0=empty, 1=wall, -1=special)
void GenerateMazeMap(int32_t startx, int32_t starty) {
  // Outside Type
  squaretype[0].face[0]=squaretype[0].face[1]=squaretype[0].face[2]=squaretype[0].face[3]=10; // No Wall
  squaretype[0].ceiling = 10; // Sky
  squaretype[0].floor   =  2; // Grass

  //Wall and Empty
  squaretype[1].face[0]=squaretype[1].face[1]=squaretype[1].face[2]=squaretype[1].face[3] = 0;
  squaretype[1].ceiling = 3;
  squaretype[1].floor   = 5;
  
  // Special
  squaretype[2].ceiling = 4;
  squaretype[2].floor   = 4;

  int32_t x, y;
  int8_t try;
  int32_t cursorx, cursory, next=1;
  
  cursorx = startx; cursory=starty;  
  for (int16_t i=0; i<MAP_SIZE*MAP_SIZE; i++) map[i] = 0; // Fill map with 0s
  
  while(true) {
    int32_t current = cursory * MAP_SIZE + cursorx;
    if((map[current] & 15) == 15) {              // If all directions have been tried, then go to previous cell (unless you're back at the start)
      if(cursory==starty && cursorx==startx) {   // If back at the start, then we're done.
        map[current]=1;
        for (int16_t i=0; i<MAP_SIZE*MAP_SIZE; i++)
          if(map[i]==0) map[i] = 128+1;          // invert map bits (1=empty, 128+1=wall, 2=special)
        setmap(player.x, player.y, getmap(player.x, player.y)&127); // make sure the square the player occupies is traversable
        return;                                  // Maze is completed!
      }
      switch(map[current] >> 4) {                // Else go back to the previous cell:  NOTE: If the 1st two bits are used, need to "&3" mask this
       case 0: cursorx++; break;
       case 1: cursory++; break;
       case 2: cursorx--; break;
       case 3: cursory--; break;
      }
      map[current]=next; next=1;   // cells which have been double-traversed are not the end of a dead end, cause we backtracked through it, so set it to square-type 1
    } else {                       // not all directions have been tried
      do try = rand()%4; while (map[current] & (1<<try));  // Pick random direction until that direction hasn't been tried
      map[current] |= (1<<try); // turn on bit in this cell saying this path has been tried
      // below is just: x=0, y=0; if(try=0)x=1; if(try=1)y=1; if(try=2)x=-1; if(try=3)y=-1;
      y=(try&1); x=y^1; if(try&2){y=(~y)+1; x=(~x)+1;} //  y = try's 1st bit, x=y with 1st bit xor'd (toggled).  Then "Two's Complement Negation" if try's 2nd bit=1
      
      // Move if spot is blank and every spot around it is blank (except where it came from)
      if((cursory+y)>0 && (cursory+y)<MAP_SIZE-1 && (cursorx+x)>0 && (cursorx+x)<MAP_SIZE-1) // Make sure not moving to or over boundary
        if(map[(cursory+y) * MAP_SIZE + cursorx + x]==0)                                    // Make sure not moving to a dug spot
          if((map[(cursory+y-1) * MAP_SIZE + cursorx+x]==0 || try==1))                      // Nothing above (unless came from above)
            if((map[(cursory+y+1) * MAP_SIZE + cursorx+x]==0 || try==3))                    // nothing below (unless came from below)
              if((map[(cursory+y) * MAP_SIZE + cursorx+x - 1]==0 || try==0))                // nothing to the left (unless came from left)
                if((map[(cursory+y) * MAP_SIZE + cursorx + x + 1]==0 || try==2)) {          // nothing to the right (unless came from right)
                  cursorx += x; cursory += y;                                              // All's good!  Let's move
                  next=2;                                                                  // Set dead end spots as square-type 2
                  map[cursory * MAP_SIZE + cursorx] |= ((try+2)%4) << 4; //record in new cell where ya came from -- the (try+2)%4 is because when you move west, you came from east
                }
    }
  } //End While True
}

// ------------------------------------------------------------------------ //
//  General game functions
// ------------------------------------------------------------------------ //
void walk(int32_t direction, int32_t distance) {
  int32_t dx = (cos_lookup(direction) * distance) >> 16;
  int32_t dy = (sin_lookup(direction) * distance) >> 16;
  if(getmap(player.x + dx, player.y) < 128 || IDCLIP) player.x += dx;  // currently <128 so blocks rays hit user hits.  will change to walkthru type blocks
  if(getmap(player.x, player.y + dy) < 128 || IDCLIP) player.y += dy;
}


//shoot_ray(x, y, angle)
//  x, y = position on map to shoot the ray from
//  angle = direction to shoot the ray (in Pebble angle notation)
//modifies: global RayStruct ray
//    uses: getmap(), abs32()
void shoot_ray(int32_t start_x, int32_t start_y, int32_t angle) {
  int32_t sin, cos, dx, dy, nx, ny;  // sine & cosine, difference x&y, next x&y

    sin = sin_lookup(angle);
    cos = cos_lookup(angle);
  ray.x = start_x;// + (cos>>11); // fixes fisheye, but puts you inside walls if you're too close. was ((32*cos)>>16), 32 being dist from player to edge of view plane
  ray.y = start_y;// + (sin>>11); //
     ny = sin>0 ? 64 : -1;
     nx = cos>0 ? 64 : -1;

  do {
    do {
      dy = ny - (ray.y & 63);                        //   north-south component of distance to next east-west wall
      dx = nx - (ray.x & 63);                        //   east-west component of distance to next north-south wall
  
      if(abs32(dx * sin) < abs32(dy * cos)) {        // if(distance to north-south wall < distance to east-west wall) See Footnote 1
        ray.x += dx;                                   // move ray to north-south wall: x part
        ray.y += ((dx * sin) / cos);                   // move ray to north-south wall: y part
        ray.hit = getmap(ray.x, ray.y);                // see what the ray is at on the map
        if(ray.hit > 127) {                            // if ray hits a wall (a block)
          ray.face = cos>0 ? 2 : 0;                      // hit west or east face of block
          ray.offset = cos>0 ? 63-(ray.y&63) : ray.y&63; // Offset is where on wall ray hits: 0 (left edge) to 63 (right edge)
          ray.dist = ((ray.x - start_x) << 16) / cos;    // Distance ray traveled.    <<16 = * TRIG_MAX_RATIO
          return;                                      // Exit
        }
      } else {                                       // else: distance to Y wall < distance to X wall
        ray.x += (dy * cos) / sin;                     // move ray to east-west wall: x part
        ray.y += dy;                                   // move ray to east-west wall: y part
        ray.hit = getmap(ray.x, ray.y);                // see what the ray is at on the map
        if(ray.hit > 127) {                            // if ray hits a wall (a block)
            ray.face = sin>0 ? 3 : 1;                    // hit south or north face of block
          ray.offset = sin>0 ? ray.x&63 : 63-(ray.x&63); // Get offset: offset is where on wall ray hits: 0 (left edge) to 63 (right edge)
            ray.dist = ((ray.y - start_y) << 16) / sin;  // Distance ray traveled.    <<16 = * TRIG_MAX_RATIO
          return;                                      // Exit
        }
      }                                              // End if/then/else (x dist < y dist)
      
    } while(ray.hit>0);  //loop while ray is not out of bounds
  } while (!((sin<0&&ray.y<0) || (sin>0&&ray.y>=(MAP_SIZE<<6)) || (cos<0&&ray.x<0) || (cos>0&&ray.x>=(MAP_SIZE<<6))) ); // loop if ray is not going further out of bounds
  
  // ray will never hit a wall (out of bounds AND going further out of bounds)
  ray.hit  = 0;           // Never hit, so set to out-of-bounds block type (0)
  ray.face = 0;           // TODO: set face to face wall hit on block type 0 //ray.face = sin>0 ? (cos>0 ? 2 : 0) : (sin>0 ? 3 : 1);
  ray.dist = 0x7FFFFFFF;  // Never hits makes distance effectively infinity. 7F instead of FF cause of signed/unsigned conversion issues
  return;
}

uint8_t getmap(int32_t x, int32_t y) {
  x>>=6; y>>=6;
  return (x<0 || x>=MAP_SIZE || y<0 || y>=MAP_SIZE) ? 0 : map[(y * MAP_SIZE) + x];
}

void setmap(int32_t x, int32_t y, uint8_t value) {
  x>>=6; y>>=6;
  if((x >= 0) && (x < MAP_SIZE) && (y >= 0) && (y < MAP_SIZE))
    map[y * MAP_SIZE + x] = value;
}
