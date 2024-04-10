#include "assets.h"

extern EFI_BOOT_SERVICES *bs; // Boot services

Sprite sprites[] = {
		[ASSET_BG] = {.width = 576, .height = 512, .img_x_offset = 0, .img_y_offset = 0},
		[ASSET_BIRD] = {.width = 34, .height = 32, .img_x_offset = 816, .img_y_offset = 180},
		[ASSET_PIPE_DOWN] = {.width = 52, .height = 270, .img_x_offset = 892, .img_y_offset = 0},
		[ASSET_PIPE_UP] = {.width = 52, .height = 242, .img_x_offset = 948, .img_y_offset = 0}
};