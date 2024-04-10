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


enum ASSETS {
	ASSET_BG,
	ASSET_BIRD,
	ASSET_PIPE_UP,
	ASSET_PIPE_DOWN
};

#endif