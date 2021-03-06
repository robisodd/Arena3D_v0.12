// ------------------------------------------------------------------------ //
//  Color Drawing Functions
// ------------------------------------------------------------------------ //
#ifdef PBL_COLOR
#include "global.h"

extern uint8_t map[];
extern SquareTypeStruct squaretype[]; // note squaretype[0]=out of bounds ceiling/floor rendering
extern TextureStruct texture[];
extern TextureStruct sprite[];

extern GBitmap *sprite_image[1];
extern GBitmap *sprite_mask[1];

extern PlayerStruct player;
extern ObjectStruct object[];
extern RayStruct ray;

#define FULL_SHADOW 0b00111111 // 100% black
#define MORE_SHADOW 0b01111111 // 66%  dark
#define SOME_SHADOW 0b10111111 // 33%  shade
#define NONE_SHADOW 0b11111111 // full color
uint8_t shadowtable[] = {192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192, \
                         192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192, \
                         192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192, \
                         192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192, \
                         192,192,192,193,192,192,192,193,192,192,192,193,196,196,196,197, \
                         192,192,192,193,192,192,192,193,192,192,192,193,196,196,196,197, \
                         192,192,192,193,192,192,192,193,192,192,192,193,196,196,196,197, \
                         208,208,208,209,208,208,208,209,208,208,208,209,212,212,212,213, \
                         192,192,193,194,192,192,193,194,196,196,197,198,200,200,201,202, \
                         192,192,193,194,192,192,193,194,196,196,197,198,200,200,201,202, \
                         208,208,209,210,208,208,209,210,212,212,213,214,216,216,217,218, \
                         224,224,225,226,224,224,225,226,228,228,229,230,232,232,233,234, \
                         192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207, \
                         208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223, \
                         224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239, \
                         240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255};

uint8_t combine_colors(uint8_t bg_color, uint8_t fg_color) {
  return (shadowtable[((~fg_color)&0b11000000) + (bg_color&63)]&63) + shadowtable[fg_color];
}

// Shade a rectangular region
void shadow_rect(GContext *ctx, GRect rect, uint8_t alpha) {
  //alpha |= 0b00111111; // sanitize alpha input // commented out cause I'm careful and don't need no safety
  GBitmap* framebuffer = graphics_capture_frame_buffer(ctx);
  if(framebuffer) {   // if successfully captured the framebuffer
    uint8_t* screen = gbitmap_get_data(framebuffer);
    for(uint16_t y_addr=rect.origin.y*144, row=0; row<rect.size.h; y_addr+=144, ++row)
      for(uint16_t x_addr=rect.origin.x, x=0; x<rect.size.w; ++x_addr, ++x)
      screen[y_addr+x_addr] = shadowtable[alpha & screen[y_addr+x_addr]];
    graphics_release_frame_buffer(ctx, framebuffer);
  }
}

//screen[y_addr+x_addr] = combine_colors(screen[y_addr+x_addr], color);
//shadow_rect(ctx, GRect(0, 74, 144, 20), 0b01111111);  // shadow bar in the middle










uint8_t number_of_objects=0;
uint8_t sprite_list[MAX_OBJECTS];
// goes through all sprites, sees if they still exist
void update_and_sort_sprites() {
  int32_t dx, dy;
  number_of_objects=0;
  
  for(uint8_t i=0; i<MAX_OBJECTS; i++) {
    if(object[i].type) {  // if exists
      sprite_list[number_of_objects] = i;
      number_of_objects++;

      dx = object[i].x - player.x;
      dy = object[i].y - player.y;
      object[i].angle = atan2_lookup(dy, dx); // angle = angle between player's x,y and sprite's x,y
      object[i].dist = (((dx^(dx>>31)) - (dx>>31)) > ((dy^(dy>>31)) - (dy>>31))) ? (dx<<16) / cos_lookup(object[i].angle) : (dy<<16) / sin_lookup(object[i].angle);
      // object[i].dist = (((dx<0)?0-dx:dx) > ((dy<0)?0-dy:dy))                     ? (dx<<16) / cos_lookup(angle) : (dy<<16) / sin_lookup(angle);
      // object[i].dist = (abs32(dx)>abs32(dy))                                     ? (dx<<16) / cos_lookup(angle) : (dy<<16) / sin_lookup(angle);
      object[i].angle -= player.facing;  // angle is now angle between center view column and object. <0=left of center, 0=center column, >0=right of center
    }
  }
  
  // Insertion Sort: Sort sprite_list in order from farthest to closest
  for(uint8_t i=1; i<number_of_objects; i++) {
    uint8_t temp = sprite_list[i];
    uint8_t j = i;
    while(j>0 && object[sprite_list[j-1]].dist<object[temp].dist) {
      sprite_list[j] = sprite_list[j-1];
      --j;
    }
    sprite_list[j] = temp;
  }
  
}










// ------------------------------------------------------------------------ //
//  Drawing to screen functions
// ------------------------------------------------------------------------ //
// void fill_window(GContext *ctx, uint8_t *data) {
//   for(uint16_t y=0, yaddr=0; y<168; y++, yaddr+=20)
//     for(uint16_t x=0; x<19; x++)
//       ((uint8_t*)(((GBitmap*)ctx)->addr))[yaddr+x] = data[y%8];
// }
//for(uint16_t y=0,; y<168*144; y+=144) for(uint16_t x=0; x<144; x++) screen[y+x] = 0b11000110;  // Fill with Blue background
//for(uint16_t i=0; i<168*144; i++) screen[i] = 0b11000110;  // Fill entire screen with Blue background

void draw_textbox(GContext *ctx, GRect textframe, char *text) {
  //graphics_context_set_fill_color(ctx, GColorBlack);   graphics_fill_rect(ctx, textframe, 0, GCornerNone);  //Black Solid Rectangle
  shadow_rect(ctx, textframe, MORE_SHADOW);
  graphics_context_set_stroke_color(ctx, GColorWhite); graphics_draw_rect(ctx, textframe);                //White Rectangle Border  
  graphics_context_set_text_color(ctx, GColorWhite);  // White Text
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14), textframe, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);  //Write Text
}


// 1-pixel-per-square map:
//   for (int16_t x = 0; x < MAP_SIZE; x++) for (int16_t y = 0; y < MAP_SIZE; y++) {graphics_context_set_stroke_color(ctx, map[y*MAP_SIZE+x]>0?1:0); graphics_draw_pixel(ctx, GPoint(x, y));}
void draw_map(GContext *ctx, GRect box, int32_t zoom) {
  // note: Currently doesn't handle drawing beyond screen boundaries
  // zoom = pixel size of each square
  shadow_rect(ctx, box, SOME_SHADOW);
  GBitmap* framebuffer = graphics_capture_frame_buffer(ctx);
  if(framebuffer) {   // if successfully captured the framebuffer
    uint8_t* screen = gbitmap_get_data(framebuffer);
    int32_t x, y, yaddr, xaddr, xonmap, yonmap, yonmapinit;

    xonmap = ((player.x*zoom)>>6) - (box.size.w/2);  // Divide by ZOOM to get map X coord, but rounds [-ZOOM to 0] to 0 and plots it, so divide by ZOOM after checking if <0
    yonmapinit = ((player.y*zoom)>>6) - (box.size.h/2);
    for(x=0; x<box.size.w; x++, xonmap++) {
      xaddr = x+box.origin.x;        // X memory address

      if(xonmap>=0 && xonmap<(MAP_SIZE*zoom)) {
        yonmap = yonmapinit;
        yaddr = box.origin.y * 144;           // Y memory address

        for(y=0; y<box.size.h; y++, yonmap++, yaddr+=144)
          if(yonmap>=0 && yonmap<(MAP_SIZE*zoom))             // If within Y bounds
            if(map[(((yonmap/zoom)*MAP_SIZE))+(xonmap/zoom)]>127) //   Map shows a wall >127
              screen[xaddr + yaddr] = 0b11111111;              //     White dot
      }
    }

    graphics_release_frame_buffer(ctx, framebuffer);
  }  // endif successfully captured framebuffer

  graphics_context_set_fill_color(ctx, (GColor){.argb=((time_ms(NULL, NULL) % 250)>125?0b11000000:0b11111111)});          // Flashing dot (250 is rate, 125 (1/2 of 250) means half the time black, half white)
  graphics_fill_rect(ctx, GRect((box.size.w/2)+box.origin.x - 1, (box.size.h/2)+box.origin.y - 1, 3, 3), 0, GCornerNone); // Square Cursor

  graphics_context_set_stroke_color(ctx, GColorWhite); graphics_draw_rect(ctx, GRect(box.origin.x-1, box.origin.y-1, box.size.w+2, box.size.h+2)); // White Border
}


int32_t Q1=0, Q2=0, Q3=0, Q4=0, Q5=0;
// implement more options: draw_3D_wireframe?  draw_3D_shaded?
void draw_3D(GContext *ctx, GRect box) { //, int32_t zoom) {
  //mid_y = (box.size.h/2) or maybe box.origin.y + (box.size.h/2) (middle in view or pixel on screen)
  //mid_x = (box.size.w/2) or maybe box.origin.x + (box.size.w/2)
  int32_t dx, dy;
  int16_t angle;
  int32_t farthest = 0; //colh, z;
  int32_t y, colheight, halfheight, bottom_half;
  uint32_t x, addr, addr2, xoffset, yoffset;
  uint8_t *target;
  uint8_t *palette;
  uint8_t txt;

  int32_t dist[144];                 // Array of non-cos adjusted distance for each vertical wall segment -- for sprite rendering
  halfheight = (box.size.h-1)>>1;         // Subtract one in case height is an even number
  bottom_half = (box.size.h&1) ? 0 : 144; // whether bottom-half column starts at the same center pixel or one below (due to odd/even box.size.h)

  // Draw background
  //graphics_context_set_fill_color(ctx, GColorBlack);  graphics_fill_rect(ctx, box, 0, GCornerNone); // Black background
  //graphics_context_set_fill_color(ctx, GColorCobaltBlue);  graphics_fill_rect(ctx, box, 0, GCornerNone); // Blue background
  // Draw Sky from horizion on up, rotate based upon player angle
  //graphics_context_set_fill_color(ctx, 1); graphics_fill_rect(ctx, GRect(box.origin.x, box.origin.y, box.size.w, box.size.h/2), 0, GCornerNone);    // White Sky  (Lightning?  Daytime?)
  //graphics_context_set_stroke_color(ctx, GColorOrange); graphics_draw_rect(ctx, GRect(box.origin.x-1, box.origin.y-1, box.size.w+2, box.size.h+2)); // Draw Box around view (not needed if fullscreen)

  GBitmap* framebuffer = graphics_capture_frame_buffer(ctx);
  if(framebuffer) {   // if successfully captured the framebuffer
    uint8_t* screen = gbitmap_get_data(framebuffer);
 
    for(addr=(box.origin.y*144)+box.origin.x, y=0; y<box.size.h; addr+=144-box.size.w,++y)
      for(uint16_t x=0; x<box.size.w; ++x, ++addr)
        screen[addr] = 0b11110011;  // Fill with Purple background for designing
    
    // Draw Box around view (not needed if fullscreen)
    #if view_border
    {
      uint32_t lft = (box.origin.x < 1) ? 0 : box.origin.x - 1;
      uint32_t rgt = (box.origin.x + box.size.w > 143) ? 143 : box.origin.x + box.size.w;
      uint32_t top = (box.origin.y < 1) ? 0 : (box.origin.y - 1)*144;
      uint32_t bot = (box.origin.y + box.size.h > 167) ? 167*144 : (box.origin.y + box.size.h)*144;

      for(x=lft; x<=rgt; ++x) {
        screen[top+x]=0b11101010;
        screen[bot+x]=0b11010101;
      }
      for(x=top; x<bot; x+=144) {
        screen[lft+x]=0b11101010;
        screen[rgt+x]=0b11010101;
      }
    }
    #endif

      
    x = box.origin.x;  // X screen coordinate
    for(int16_t col = 0; col < box.size.w; ++col, ++x) {        // Begin RayTracing Loop
      angle = atan2_lookup((64*col/box.size.w) - 32, 64);    // dx = (64*(col-(box.size.w/2)))/box.size.w; dy = 64; angle = atan2_lookup(dx, dy);

      shoot_ray(player.x, player.y, player.facing + angle);  //Shoot rays out of player's eyes.  pew pew.
      ray.hit &= 127;                                        // Whether ray hit a block (>127) or not (<128), set ray.hit to valid block type [0-127]
      if(ray.dist > (uint32_t)farthest) farthest = ray.dist; // farthest (furthest?) wall (for sprite rendering. only render sprites closer than farthest wall)
      dist[col] = (uint32_t)ray.dist;                        // save distance of this column for sprite rendering later
      ray.dist *= cos_lookup(angle);                         // multiply by cos to stop fisheye lens (should be >>16 to get actual dist, but that's all done below)
      //ray.dist <<= 16;                                     // use this if commenting out "ray.dist*=cos" line above, cause it >>16's a lot below
      colheight = (box.size.h << 21) /  ray.dist;    // half wall segment height = box.size.h * wallheight * 64(the "zoom factor") / (distance >> 16) // now /2 (<<21 instead of 22)
      if(colheight>halfheight) colheight=halfheight; // Make sure line isn't drawn beyond bounding box

      // Calculate amount of shade
      //z =  ray.dist >> 16; //z=(ray.dist*cos_lookup(angle))/TRIG_MAX_RATIO;  // z = distance
      //z -= 64; if(z<0) z=0;   // Make everything 1 block (64px) closer (solid white without having to be nearly touching)
      //z = sqrt_int(z,10) >> 1; // z was 0-RANGE(max dist visible), now z = 0 to 12: 0=close 10=distant.  Square Root makes it logarithmic
      //z -= 2; if(z<0) z=0;    // Closer still (zWas=zNow: 0-64=0, 65-128=2, 129-192=3, 256=4, 320=6, 384=6, 448=7, 512=8, 576=9, 640=10)
      // try a different shade calculation
      
      uint8_t alpha=0b11111111;  // Extra shading ANDed to texture's alpha (which is probably 0b11xxxxxx)
//       zzz=ray.dist>>16;
//       if(zzz>0b0000000010000000/*32*/)  alpha=0b10111111;
//       if(zzz>0b0000001000000000/*64*/)  alpha=0b01111111;
//       if(zzz>0b0000010000000000/*128*/) alpha=0b00111111;
      // end shade calculation

      // Draw Color Walls
      txt = squaretype[ray.hit].face[ray.face];
      palette = texture[txt].palette;
      target = texture[txt].data + (ray.offset<<texture[txt].bytes_per_row) + (1<<(texture[txt].bytes_per_row-1)); // put pointer in the middle of row texture.y=ray.offset (pointer = texture's upper left [0,0] + y*)
      
      addr = x + ((box.origin.y + halfheight) * 144); // address of screen pixel vertically centered at column X
      addr2 = addr + bottom_half;                         // If box.size.h is even, there's no center pixel (else top and bottom half_column_heights are different), so start bottom column one pixel lower (or not, if h is odd)
      y=0; yoffset=0;  // y is y +/- from vertical center, yoffset is the screen memory position of y (and is always = y*144)
      for(; y<=colheight; y++, yoffset+=144) {
        xoffset =  ((y * ray.dist / box.size.h) >> 16); // xoffset = which pixel of the texture is hit (0-31).  See Footnote 2
        screen[addr  - yoffset] = shadowtable[alpha & palette[(*(target - 1 - (xoffset>>texture[txt].pixels_per_byte)) >> ((                                 (xoffset&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel)&texture[txt].colormax)]];  // Draw Top Half
        screen[addr2 + yoffset] = shadowtable[alpha & palette[(*(target     + (xoffset>>texture[txt].pixels_per_byte)) >> (((7>>texture[txt].bits_per_pixel)-(xoffset&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel)&texture[txt].colormax)]];  // Draw Bottom Half (Texture is horizontally mirrored top half)
      }
      // End Draw Walls

//       {
//       colheight = ((box.size.h << 19) /  ray.dist);    // half wall segment height = box.size.h * wallheight * 64(the "zoom factor") / (distance >> 16) // now /2 (<<21 instead of 22)
//       if(colheight>halfheight) colheight=halfheight; // Make sure line isn't drawn beyond bounding box
//       y=colheight+1; yoffset = y*144;
      
//       // Draw Floor/Ceiling
//       int32_t temp_x = (((box.size.h << 3) * cos_lookup(player.facing + angle)) / cos_lookup(angle)); // Calculate now to save time later
//       int32_t temp_y = (((box.size.h << 3) * sin_lookup(player.facing + angle)) / cos_lookup(angle)); // Calculate now to save time later
//       for(; y<=halfheight; y++, yoffset+=144) {       // y and yoffset continue from wall top&bottom to view edge (unless wall is taller than view edge)
//         int32_t map_x = player.x + (temp_x / y);     // map_x & map_y = spot on map the screen pixel points to
//         int32_t map_y = player.y + (temp_y / y);     // map_y = player.y + dist_y, dist = (height/2 * 64 * (sin if y, cos if x) / i) (/cos to un-fisheye)
//         ray.hit = getmap(map_x, map_y) & 127;        // ceiling/ground of which cell is hit.  &127 shouldn't be needed since it *should* be hitting a spot without a wall

//         map_x&=63; map_y&=63; // Get position on texture
//         //txt=squaretype[ray.hit].ceiling; screen[addr  - yoffset] = texture[txt].palette[(((*(texture[txt].data + ((   map_x)<<texture[txt].bytes_per_row) + (map_y>>texture[txt].pixels_per_byte))) >> (((7>>texture[txt].bits_per_pixel)-(map_y&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel))&texture[txt].colormax)];
//         txt=squaretype[ray.hit].floor;   screen[addr2 + yoffset] = texture[txt].palette[(((*(texture[txt].data + ((63-map_x)<<texture[txt].bytes_per_row) + (map_y>>texture[txt].pixels_per_byte))) >> (((7>>texture[txt].bits_per_pixel)-(map_y&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel))&texture[txt].colormax)];
//       } // End Floor/Ceiling
//       }
      
//       {
//       colheight = ((box.size.h << 21) /  ray.dist);    // half wall segment height = box.size.h * wallheight * 64(the "zoom factor") / (distance >> 16) // now /2 (<<21 instead of 22)
//       if(colheight>halfheight) colheight=halfheight; // Make sure line isn't drawn beyond bounding box
//       y=colheight+1; yoffset=y*144;
      
//       // Draw Floor/Ceiling
//       int32_t temp_x = (((box.size.h << 5) * cos_lookup(player.facing + angle)) / cos_lookup(angle)); // Calculate now to save time later
//       int32_t temp_y = (((box.size.h << 5) * sin_lookup(player.facing + angle)) / cos_lookup(angle)); // Calculate now to save time later
//       for(; y<=halfheight; y++, yoffset+=144) {       // y and yoffset continue from wall top&bottom to view edge (unless wall is taller than view edge)
//         int32_t map_x = player.x + (temp_x / y);     // map_x & map_y = spot on map the screen pixel points to
//         int32_t map_y = player.y + (temp_y / y);     // map_y = player.y + dist_y, dist = (height/2 * 64 * (sin if y, cos if x) / i) (/cos to un-fisheye)
//         ray.hit = getmap(map_x, map_y) & 127;        // ceiling/ground of which cell is hit.  &127 shouldn't be needed since it *should* be hitting a spot without a wall

//         map_x&=63; map_y&=63; // Get position on texture
//         txt=squaretype[ray.hit].ceiling; 
//         if(texture[txt].colormax>0) screen[addr  - yoffset] = texture[txt].palette[(((*(texture[txt].data + ((   map_x)<<texture[txt].bytes_per_row) + (map_y>>texture[txt].pixels_per_byte))) >> (((7>>texture[txt].bits_per_pixel)-(map_y&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel))&texture[txt].colormax)];
//         //txt=squaretype[ray.hit].floor;   screen[addr2 + yoffset] = texture[txt].palette[(((*(texture[txt].data + ((63-map_x)<<texture[txt].bytes_per_row) + (map_y>>texture[txt].pixels_per_byte))) >> (((7>>texture[txt].bits_per_pixel)-(map_y&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel))&texture[txt].colormax)];
//       } // End Floor/Ceiling
//       }
      
      {
      colheight = ((box.size.h << 21) /  ray.dist);    // half wall segment height = box.size.h * wallheight * 64(the "zoom factor") / (distance >> 16) // now /2 (<<21 instead of 22)
      if(colheight>halfheight) colheight=halfheight; // Make sure line isn't drawn beyond bounding box
      y=colheight+1; yoffset=y*144;
        
      // Draw Floor/Ceiling
      int32_t temp_x = (((box.size.h << 5) * cos_lookup(player.facing + angle)) / cos_lookup(angle)); // Calculate now to save time later
      int32_t temp_y = (((box.size.h << 5) * sin_lookup(player.facing + angle)) / cos_lookup(angle)); // Calculate now to save time later
      for(; y<=halfheight; y++, yoffset+=144) {       // y and yoffset continue from wall top&bottom to view edge (unless wall is taller than view edge)
        int32_t map_x = player.x + (temp_x / y);     // map_x & map_y = spot on map the screen pixel points to
        int32_t map_y = player.y + (temp_y / y);     // map_y = player.y + dist_y, dist = (height/2 * 64 * (sin if y, cos if x) / i) (/cos to un-fisheye)
        ray.hit = getmap(map_x, map_y) & 127;        // ceiling/ground of which cell is hit.  &127 shouldn't be needed since it *should* be hitting a spot without a wall

        uint8_t alpha=0b11111111;
//         if(y<64) alpha=0b10111111;
//         if(y<16)  alpha=0b01111111;
//         if(y<4)  alpha=0b00111111;

        map_x&=63; map_y&=63; // Get position on texture
        txt=squaretype[ray.hit].ceiling; screen[addr  - yoffset] = shadowtable[alpha & texture[txt].palette[(((*(texture[txt].data + ((   map_x)<<texture[txt].bytes_per_row) + (map_y>>texture[txt].pixels_per_byte))) >> (((7>>texture[txt].bits_per_pixel)-(map_y&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel))&texture[txt].colormax)]];
        txt=squaretype[ray.hit].floor;   screen[addr2 + yoffset] = shadowtable[alpha & texture[txt].palette[(((*(texture[txt].data + ((63-map_x)<<texture[txt].bytes_per_row) + (map_y>>texture[txt].pixels_per_byte))) >> (((7>>texture[txt].bits_per_pixel)-(map_y&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel))&texture[txt].colormax)]];
      } // End Floor/Ceiling
      }
      ray.hit = dist[col]; // just to stop compiler from complaining if not rendering sprites
    } // End RayTracing Loop

    
    
// =======================================SPRITES====================================================
  // Draw Sprites!
  // Sort sprites by distance from player
  // draw sprites in order from farthest to closest
  // start from sprites closer than "farthest wall"
  // sprite:
  // x
  // y
  // angle
  // distance
  // type
  // d
//    note: Sprites can be semi-translucent!
    
    
    update_and_sort_sprites();
    
  //uint8_t numobjects=MAX_OBJECTS;
  int32_t spritecol, objectdist;  //, xoffset, yoffset;
//if(false)  // enable/disable drawing of sprites
  for(uint8_t j=0; j<number_of_objects; ++j) {
/*    
    dx = object[i].x - player.x;
    dy = object[i].y - player.y;
    angle = atan2_lookup(dy, dx); // angle = angle between player's x,y and sprite's x,y
    objectdist =  (((dx^(dx>>31)) - (dx>>31)) > ((dy^(dy>>31)) - (dy>>31))) ? (dx<<16) / cos_lookup(angle) : (dy<<16) / sin_lookup(angle);
    //objectdist =  (((dx<0)?0-dx:dx) > ((dy<0)?0-dy:dy)) ? (dx<<16) / cos_lookup(angle) : (dy<<16) / sin_lookup(angle);
//     objectdist = (abs32(dx)>abs32(dy))                                   ? (dx<<16) / cos_lookup(angle) : (dy<<16) / sin_lookup(angle);
    angle = angle - player.facing;  // angle is now angle between center view column and object. <0=left of center, 0=center column, >0=right of center
*/
    uint8_t i=sprite_list[j];
    angle = object[i].angle;
    objectdist = object[i].dist;
    i = object[i].sprite;
    
    if(cos_lookup(angle)>0) { // if object is in front of player.  note: if angle = 0 then object is straight right or left of the player
      if(farthest>=objectdist) { // if ANY wall is further (or =) than object distance, then display it
        spritecol = (box.size.w/2) + ((sin_lookup(angle)*box.size.w)>>16);  // column on screen of sprite center

        int32_t objectwidth = sprite[i].width;
        int32_t objectheight = sprite[i].height;
        //int32_t objectverticaloffset = 64-objectheight; // on the ground
        //int32_t objectverticaloffset = 0;               // floating in the middle
        int32_t objectverticaloffset = objectheight-64;   // on the ceiling

//         int32_t spritescale = box.size.h ;// * 20 / 10;

        //objectdist = (objectdist * cos_lookup(angle)) >> 16;

        int32_t spritescale = (box.size.h);// * 20 / 10;
        int32_t spritewidth  = (spritescale * objectwidth) / objectdist;   // should be box.size.w, but wanna keep sprite h/w proportionate
        int32_t spriteheight = (spritescale * objectheight)/ objectdist;  // note: make sure to use not-cosine adjusted distance!
//         int32_t halfspriteheight = spriteheight/2;
        int32_t spriteverticaloffset = (objectverticaloffset * spritescale) / objectdist; // fisheye adjustment
//         int32_t spriteverticaloffset = ((((objectverticaloffset * spritescale) + (32*box.size.h)) << 16) / (objectdist * cos_lookup(angle))); // floor it


        int16_t sprite_xmin = spritecol - (spritewidth/2);
        int16_t sprite_xmax = sprite_xmin + spritewidth;  // was =spritecol+(spritewidth/2);  Changed to display whole sprite cause /2 loses info
        if(sprite_xmax>=0 && sprite_xmin<box.size.w) {    // if any of the sprite is horizontally within view
          int16_t xmin = sprite_xmin<0 ? 0: sprite_xmin;
          int16_t xmax = sprite_xmax>box.size.w ? box.size.w : sprite_xmax;


// Half through floor
//int32_t objectheight = 16;          // 32 pixels tall
//int32_t objectverticaloffset = 64-objectheight;//+32;//16; // normally center dot is vertically centered, + or - how far to move it.

// perfectly puts 32x32 sprite on ceiling
//int32_t objectwidth = 32;
//int32_t objectheight = 64;
//int32_t objectverticaloffset = 0;
//int32_t spritescale = box.size.h;
//int32_t spritewidth  = (spritescale * objectwidth) / objectdist;
//int32_t spriteheight = (spritescale * objectheight) / objectdist;
//int32_t spriteverticaloffset = ((objectverticaloffset * spritescale) << 16) / (objectdist * cos_lookup(angle)); // fisheye adjustment
//int16_t sprite_ymax = spriteverticaloffset + ((box.size.h + spriteheight)/2);// + (((32*box.size.h) << 16) / (objectdist * cos_lookup(angle)));
//int16_t sprite_ymin = sprite_ymax - spriteheight; // note: sprite is not cos adjusted but offset is (to keep it in place)


          int16_t sprite_ymax = (box.size.h + spriteheight + spriteverticaloffset)/2;// + (((32*box.size.h) << 16) / (objectdist * cos_lookup(angle)));
          int16_t sprite_ymin = sprite_ymax - spriteheight; // note: sprite is not cos adjusted but offset is (to keep it in place)

//           int16_t sprite_ymin = halfheight + spriteverticaloffset - spriteheight; // note: sprite is not cos adjusted but offset is (to keep it in place)
//           int16_t sprite_ymax = halfheight + spriteverticaloffset;



          if(sprite_ymax>=0 && sprite_ymin<box.size.h) { // if any of the sprite is vertically within view
            int16_t ymin = sprite_ymin<0 ? 0 : sprite_ymin;
            int16_t ymax = sprite_ymax>box.size.h ? box.size.h : sprite_ymax;
///BEGIN DRAWING LOOPS
//       txt = squaretype[ray.hit].face[ray.face];
//       palette = texture[txt].palette;
//       target = texture[txt].data + (ray.offset<<texture[txt].bytes_per_row) + (1<<(texture[txt].bytes_per_row-1)); // put pointer in the middle of row texture.y=ray.offset (pointer = texture's upper left [0,0] + y*)
      
//       addr = x + ((box.origin.y + halfheight) * 144); // address of screen pixel vertically centered at column X
//       addr2 = addr + bottom_half;                         // If box.size.h is even, there's no center pixel (else top and bottom half_column_heights are different), so start bottom column one pixel lower (or not, if h is odd)
//       y=0; yoffset=0;  // y is y +/- from vertical center, yoffset is the screen memory position of y (and is always = y*144)
//       for(; y<=colheight; y++, yoffset+=144) {
//         xoffset =  ((y * ray.dist / box.size.h) >> 16); // xoffset = which pixel of the texture is hit (0-31).  See Footnote 2
//         screen[addr  - yoffset] = palette[(*(target - 1 - (xoffset>>texture[txt].pixels_per_byte)) >> ((                                 (xoffset&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel)&texture[txt].colormax)];  // Draw Top Half
//         screen[addr2 + yoffset] = palette[(*(target     + (xoffset>>texture[txt].pixels_per_byte)) >> (((7>>texture[txt].bits_per_pixel)-(xoffset&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel)&texture[txt].colormax)];  // Draw Bottom Half (Texture is horizontally mirrored top half)
//       }

//       case GBitmapFormat1BitPalette: {// IF 1bit texture. 8 means: 64px / 8Bytes/row = 8px/Byte = 1bit/px
//         target += ray.offset<<3;//*8;   // maybe use GBitmap's size veriables to store texture size?  // * 8 = 8 bytes per row
//         target += 4 - 1; //3         // 4 = half of 8 Bytes/row
//         for(; y<colheight; y++, yoffset+=144) {
//           xoffset =  ((y * ray.dist / box.size.h) >> 16); // xoffset = which pixel of the texture is hit (0-31).  See Footnote 2
//           screen[addr - yoffset] = palette[(*(target     - (xoffset>>3)) >> ((  (xoffset&7))<<0)&1)];  // Draw Top Half   <<0 (*1) is because 1 bit per pixel
//           screen[addr + yoffset] = palette[(*(target + 1 + (xoffset>>3)) >> ((7-(xoffset&7))<<0)&1)];  // Draw Bottom Half
//         }

//       case GBitmapFormat2BitPalette: { // Else: Draw 4bits/px (16 color) texture (note: Texture size is 4bits/px * 64*64px = 2048 Bytes)
//         target += ray.offset<<4;//*16;  // Puts pointer at row beginning // ray.offset is y position on texture = [0-63].  8 sets of 32bits = 1 row  // * 16 = 16 Bytes per row
//         target += 8 - 1; //7         // puts pointer at (mid) of row // 8 = half of 16 bytes/row
//         for(; y<=colheight; ++y, yoffset+=144) {
//           xoffset =  ((y * ray.dist / box.size.h) >> 16); // xoffset = which pixel of the texture is hit (0-31).  See Footnote 2
//           screen[addr - yoffset] = palette[(*(target     - (xoffset>>2)) >> ((  (xoffset&3))<<1) )&3];  // Draw Top Half  // &3 (0-3=4) cause 4 pixels inside byte.  <<1 (*2) is cause 2 bits per pixel
//           screen[addr + yoffset] = palette[(*(target + 1 + (xoffset>>2)) >> ((3-(xoffset&3))<<1) )&3];  // Draw Bottom Half
//         }
        
            uint32_t xaddr, yaddr;
            for(int16_t x = xmin; x < xmax; x++) {
              if(dist[x]>=objectdist) {  // if not behind wall
                //xoffset = (63-(((x - sprite_xmin) * objectdist) / spritescale)) << sprite[0].bytes_per_row; // x point hit on texture -- make sure to use the original object dist, not the cosine adjusted one
                xoffset = ((sprite[i].width-1)-(((x - sprite_xmin) * objectdist) / spritescale)) << sprite[i].bytes_per_row; // x point hit on texture -- make sure to use the original object dist, not the cosine adjusted one
                target = sprite[i].data + xoffset; // target = sprite
                xaddr = (box.origin.x + x);          // x location on screen
                yaddr = (box.origin.y + ymin) * 144; // y location on screen
                for(int16_t y=ymin; y<ymax; y++, yaddr+=144) {
                  yoffset = ((y - sprite_ymin) * objectdist) / spritescale; // y point hit on texture column (was = (objectheight*(y-sprite_ymin))/spriteheight) [0-31]
                  //screen[xaddr+yaddr] = 0b11001100;
                  //screen[xaddr+yaddr] = combine_colors(screen[xaddr+yaddr], 0b11001100);
                  screen[xaddr + yaddr] = combine_colors(screen[xaddr+yaddr], sprite[i].palette[((*(target + (yoffset>>sprite[i].pixels_per_byte))) >> (((7>>sprite[i].bits_per_pixel)-(yoffset&(7>>sprite[i].bits_per_pixel)))<<sprite[i].bits_per_pixel))&sprite[i].colormax]);  // make bit white or keep it black
                  //screen[xaddr + yaddr] = sprite[0].palette[((*(target + (yoffset>>sprite[0].pixels_per_byte))) >> (((7>>sprite[0].bits_per_pixel)-(yoffset&(7>>sprite[0].bits_per_pixel)))<<sprite[0].bits_per_pixel))&sprite[0].colormax];  // make bit white or keep it black
                     //texture[txt].palette[(((*(texture[txt].data + ((   map_x)<<texture[txt].bytes_per_row) + (map_y>>texture[txt].pixels_per_byte))) >> (((7>>texture[txt].bits_per_pixel)-(map_y&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel))&texture[txt].colormax)]];
                } // next y
              } // end display column if in front of wall
            } // next x

            
            
//                 for(int16_t y=ymin; y<ymax; y++, yaddr+=144) {
//                   //graphics_draw_pixel(ctx, GPoint(box.origin.x + x, box.origin.y + y));
//                   yoffset = ((y - sprite_ymin) * objectdist) / spritescale; // y point hit on texture column (was = (objectheight*(y-sprite_ymin))/spriteheight)
//                   //screen[xaddr+yaddr] = 0b11001100;
//                   screen[xaddr+yaddr] = combine_colors(screen[xaddr+yaddr], 0b11001100);
//                    //if(((*mask >> yoffset) & 1) == 1) {   // If mask point isn't clear, then draw point.  TODO: try removing == 1
//                    //ctx32[xaddr + yaddr] |= 1 << xbit;     // whiten bit
//                      screen[xaddr + yaddr] = sprite[0].palette[((*(target2) >> yoffset)&1)];  // make bit white or keep it black
//                      //texture[txt].palette[(((*(texture[txt].data + ((   map_x)<<texture[txt].bytes_per_row) + (map_y>>texture[txt].pixels_per_byte))) >> (((7>>texture[txt].bits_per_pixel)-(map_y&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel))&texture[txt].colormax)]];
//                    //ctx32[xaddr + yaddr] |= ((*((uint32_t*)sprite_image[0]->addr + xoffset) >> yoffset)&1) << xbit;  // make bit white or keep it black
//                    //}//endif mask
//                 } // next y
//               } // end display column if in front of wall
//             } // next x
//END DRAWING LOOPS      
          } // end display if within y bounds
        } // end display if within x bounds
      } // end display if within farthest
    } // end display if not behind you
  } // next obj
// =======================================END SPRITES====================================================
    graphics_release_frame_buffer(ctx, framebuffer);
  }  // endif successfully captured framebuffer

} // end draw 3D function
#endif