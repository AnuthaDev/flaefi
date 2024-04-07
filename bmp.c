/*
 * A program to read, write, and crop BMP image files.
 * 
 */
#include "bmp.h"

// Correct values for the header
#define MAGIC_VALUE         0x4D42 
#define NUM_PLANE           1
#define COMPRESSION         0
#define NUM_COLORS          0
#define IMPORTANT_COLORS    0
#define BITS_PER_PIXEL      24
#define BITS_PER_BYTE       8

extern EFI_BOOT_SERVICES    *bs;   // Boot services

VOID free_bmp(BMPImage *image);
INT32 _get_image_size_bytes(BMPHeader *bmp_header);
INT32 _get_image_row_size_bytes(BMPHeader *bmp_header);
INT32 _get_bytes_per_pixel(BMPHeader  *bmp_header);
INT32 _get_padding(BMPHeader *bmp_header);
INT32 _get_position_x_row(INT32 x, BMPHeader *bmp_header);

/*
 * Read a BMP image from an already open file.
 * 
 * - Postcondition: it is the caller's responsibility to free the memory
 *   for the error message and the returned image.
 * 
 * - Return: the image as a BMPImage on the heap.
 */
BMPImage *read_bmp(EFI_FILE_PROTOCOL *efp)
{    
    BMPImage *image = NULL;
		bs->AllocatePool(EfiLoaderData, sizeof(*image), (VOID*)&image);
    // if (!_check(image != NULL, error, "Not enough memory"))
    // {
    //     return NULL;
    // }
    // Read header
    //rewind(fp);
		EFI_STATUS status = efp->SetPosition(efp, 0);
    
		//int num_read = fread(&image->header, sizeof(image->header), 1, fp);
		UINTN buf_size = sizeof(image->header);
		status = efp->Read(efp, &buf_size, &(image->header));
    // if(!_check(num_read == 1, error, "Cannot read header"))
    // {
    //     return NULL;
    // }
    // Check header
    //bool is_valid_header = check_bmp_header(&image->header, fp);
		check_bmp_header(&image->header, efp);
    // if(!_check(is_valid_header, error, "Invalid BMP file"))
    // {
    //     return NULL;
    // }
    // Allocate memory for image data
    //image->data = malloc(sizeof(*image->data) * image->header.image_size_bytes);
		bs->AllocatePool(EfiLoaderData, sizeof(*image->data) * image->header.image_size_bytes, (VOID*)&image->data);
    // if (!_check(image->data != NULL, error, "Not enough memory"))
    // {
    //     return NULL;
    // }
    // Read image data
    //num_read = fread(image->data, image->header.image_size_bytes, 1, efp);
		buf_size = image->header.image_size_bytes;
		efp->Read(efp, &buf_size, image->data);
    // if (!_check(num_read == 1, error, "Cannot read image"))
    // {
    //     return NULL;
    // }

    return image;
}


/*
 * Test if the BMPHeader is consistent with itself and the already open image file.
 * 
 * Return: true if and only if the given BMPHeader is valid.
 */
BOOLEAN check_bmp_header(BMPHeader* bmp_header, EFI_FILE_PROTOCOL *efp)
{
    /*
    A header is valid if:
    1. its magic number is 0x4d42,
    2. image data begins immediately after the header data (header->offset == BMP HEADER SIZE),
    3. the DIB header is the correct size (DIB_HEADER_SIZE),
    4. there is only one image plane,
    5. there is no compression (header->compression == 0),
    6. num_colors and  important_colors are both 0,
    7. the image has 24 bits per pixel,
    8. the size and image_size_bytes fields are correct in relation to the bits,
       width, and height fields and in relation to the file size.
    */
    return
        bmp_header->type == MAGIC_VALUE
        && bmp_header->offset == BMP_HEADER_SIZE
        && bmp_header->dib_header_size == DIB_HEADER_SIZE
        && bmp_header->num_planes == NUM_PLANE
        && bmp_header->compression == COMPRESSION
        && bmp_header->num_colors == NUM_COLORS && bmp_header->important_colors == IMPORTANT_COLORS
        && bmp_header->bits_per_pixel == BITS_PER_PIXEL;
        // && bmp_header->size == _get_file_size(efp) && bmp_header->image_size_bytes == _get_image_size_bytes(bmp_header);
}

/*
 * Free all memory referred to by the given BMPImage.
 */
VOID free_bmp(BMPImage *image)
{
    bs->FreePool(image->data);
    bs->FreePool(image);
}

/* 
 * Return the size of the file.
 */
// UINT64 _get_file_size(EFI_FILE_PROTOCOL *efp)
// {   
//     // Get current file position (now at the end)
//     UINT64 file_size = 0;
// 		EFI_FILE_INFO file_info;

// 		efp->Read(efp, sizeof(file_info), &file_info );


//     return file_info.FileSize;
// }

/* 
 * Return the size of the image in bytes.
 */
INT32 _get_image_size_bytes(BMPHeader *bmp_header)
{
    return _get_image_row_size_bytes(bmp_header) * bmp_header->height_px;
}

/* 
 * Return the size of an image row in bytes.
 *  
 * - Precondition: the header must have the width of the image in pixels.
 */
INT32 _get_image_row_size_bytes(BMPHeader *bmp_header)
{
    INT32 bytes_per_row_without_padding = bmp_header->width_px * _get_bytes_per_pixel(bmp_header);
    return bytes_per_row_without_padding + _get_padding(bmp_header);
}

/*
 * Return size of padding in bytes.
 */ 
INT32 _get_padding(BMPHeader *bmp_header)
{
    return (4 - (bmp_header->width_px * _get_bytes_per_pixel(bmp_header)) % 4) % 4;
}

/* 
 * Return the number of bytes per pixel.
 *  
 * Precondition:
 *     - the header must have the number of bits per pixel.
 */
INT32 _get_bytes_per_pixel(BMPHeader  *bmp_header)
{
    return bmp_header->bits_per_pixel / BITS_PER_BYTE;
}

/*
 * Return the position of the pixel x from the beginning of a row.
 */ 
INT32 _get_position_x_row(INT32 x, BMPHeader *bmp_header)
{
    return x * _get_bytes_per_pixel(bmp_header);
}