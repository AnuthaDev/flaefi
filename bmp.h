#ifndef _BMP_H_  // prevent recursive inclusion
#define _BMP_H_

#include "efi.h"

#define BMP_HEADER_SIZE 54
#define DIB_HEADER_SIZE 40

#pragma pack(push)  // save the original data alignment
#pragma pack(1)     // Set data alignment to 1 byte boundary


typedef struct {
    UINT16 type;              // Magic identifier: 0x4d42
    UINT32 size;              // File size in bytes
    UINT16 reserved1;         // Not used
    UINT16 reserved2;         // Not used
    UINT32 offset;            // Offset to image data in bytes from beginning of file
    UINT32 dib_header_size;   // DIB Header size in bytes
    INT32  width_px;          // Width of the image
    INT32  height_px;         // Height of image
    UINT16 num_planes;        // Number of color planes
    UINT16 bits_per_pixel;    // Bits per pixel
    UINT32 compression;       // Compression type
    UINT32 image_size_bytes;  // Image size in bytes
    INT32  x_resolution_ppm;  // Pixels per meter
    INT32  y_resolution_ppm;  // Pixels per meter
    UINT32 num_colors;        // Number of colors
    UINT32 important_colors;  // Important colors
} BMPHeader;

#pragma pack(pop)  // restore the previous pack setting

typedef struct {
    BMPHeader header;
    UINT8     *data;
} BMPImage;

BMPImage* read_bmp(EFI_FILE_PROTOCOL *efp);
BOOLEAN   check_bmp_header(BMPHeader* bmp_header, EFI_FILE_PROTOCOL *efp);
VOID      free_bmp(BMPImage* image);

#endif  /* bmp.h */
