#ifndef _ASSETS_H
#define _ASSETS_H

#include "efi.h"


typedef struct Sprite {
	UINT16 width;
	UINT16 height;
	INT16 x_pos;
	INT16 y_pos;
	UINT16 img_x_offset;
	UINT16 img_y_offset;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL *image;
} Sprite;

typedef struct Obstacle {
	Sprite pipe_up;
	Sprite pipe_down;
	INT16 x_pos;
	INT16 y_offset;
	BOOLEAN can_score;
} Obstacle;

enum ASSETS {
	ASSET_ZERO,
	ASSET_ONE,
	ASSET_TWO,
	ASSET_THREE,
	ASSET_FOUR,
	ASSET_FIVE,
	ASSET_SIX,
	ASSET_SEVEN,
	ASSET_EIGHT,
	ASSET_NINE,
	ASSET_BG,
	ASSET_BIRD_1,
	ASSET_BIRD_2,
	ASSET_BIRD_3,
	ASSET_PIPE_UP,
	ASSET_PIPE_DOWN,
	ASSET_LOGO,
	ASSET_TAP,
	ASSET_GAME_OVER
};

#endif
