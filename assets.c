#include "assets.h"

extern EFI_BOOT_SERVICES *bs; // Boot services

Sprite sprites[] = {
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