#include "assets.h"

extern EFI_BOOT_SERVICES *bs; // Boot services

#define DIG_W 14
#define DIG_H 20

Sprite sprites[] = {
		[ASSET_ZERO] = {.width = DIG_W, .height = DIG_H, .img_x_offset = 864, .img_y_offset = 200},
		[ASSET_ONE] = {.width = DIG_W, .height = DIG_H, .img_x_offset = 868, .img_y_offset = 236},
		[ASSET_TWO] = {.width = DIG_W, .height = DIG_H, .img_x_offset = 866, .img_y_offset = 268},
		[ASSET_THREE] = {.width = DIG_W, .height = DIG_H, .img_x_offset = 866, .img_y_offset = 300},
		[ASSET_FOUR] = {.width = DIG_W, .height = DIG_H, .img_x_offset = 862, .img_y_offset = 346},
		[ASSET_FIVE] = {.width = DIG_W, .height = DIG_H, .img_x_offset = 862, .img_y_offset = 370},
		[ASSET_SIX] = {.width = DIG_W, .height = DIG_H, .img_x_offset = 618, .img_y_offset = 490},
		[ASSET_SEVEN] = {.width = DIG_W, .height = DIG_H, .img_x_offset = 638, .img_y_offset = 490},
		[ASSET_EIGHT] = {.width = DIG_W, .height = DIG_H, .img_x_offset = 658, .img_y_offset = 490},
		[ASSET_NINE] = {.width = DIG_W, .height = DIG_H, .img_x_offset = 678, .img_y_offset = 490},
		[ASSET_BG] = {.width = 576, .height = 512, .img_x_offset = 0, .img_y_offset = 0},
		[ASSET_BIRD_1] = {.width = 34, .height = 32, .img_x_offset = 816, .img_y_offset = 180},
		[ASSET_BIRD_2] = {.width = 34, .height = 32, .img_x_offset = 816, .img_y_offset = 128},
		[ASSET_BIRD_3] = {.width = 34, .height = 32, .img_x_offset = 734, .img_y_offset = 248},
		[ASSET_PIPE_DOWN] = {.width = 52, .height = 270, .img_x_offset = 892, .img_y_offset = 0},
		[ASSET_PIPE_UP] = {.width = 52, .height = 242, .img_x_offset = 948, .img_y_offset = 0},
		[ASSET_LOGO] = {.width = 192, .height = 44, .img_x_offset = 580, .img_y_offset = 346},
		[ASSET_TAP] = {.width = 76, .height = 40, .img_x_offset = 636, .img_y_offset = 302},
		[ASSET_GAME_OVER] = {.width = 188, .height = 38, .img_x_offset = 580, .img_y_offset = 398},

};