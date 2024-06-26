#include "ulibc.h"
#include "efi.h"
#include "bmp.h"
#include "assets.h"

// -----------------
// Global macros
// -----------------
#define ARRAY_SIZE(x) (sizeof (x) / sizeof (x)[0])

// -----------------
// Global constants
// -----------------
#define SCANCODE_UP_ARROW   0x1
#define SCANCODE_DOWN_ARROW 0x2
#define SCANCODE_ESC        0x17


#define MAGENTA_MASK 0xFFFF00FF

// -----------------
// Global variables
// -----------------
extern EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cout;  // Console output
extern EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *cin; // Console input
extern EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cerr;  // Console output - stderr
EFI_BOOT_SERVICES    *bs;   // Boot services
EFI_RUNTIME_SERVICES *rs;   // Runtime services
EFI_HANDLE image = NULL;    // Image handle

extern Sprite sprites[];


// ====================
// Set global vars
// ====================
void init_global_variables(EFI_HANDLE handle, EFI_SYSTEM_TABLE *systable) {
    cout = systable->ConOut;
    cin = systable->ConIn;
    //cerr = systable->StdErr; // TODO: Research why this does not work in emulation, nothing prints
    cerr = cout;    // TEMP: Use stdout for error printing also
    bs = systable->BootServices;
    rs = systable->RuntimeServices;
    image = handle;
}

// ====================
// Get key from user
// ====================
EFI_INPUT_KEY get_key(void) {
    EFI_EVENT events[1];
    EFI_INPUT_KEY key;

    key.ScanCode = 0;
    key.UnicodeChar = u'\0';

    events[0] = cin->WaitForKey;
    UINTN index = 0;
    bs->WaitForEvent(1, events, &index);

    if (index == 0) cin->ReadKeyStroke(cin, &key);

    return key;
}

// ====================
// Set Graphics Mode
// ====================
EFI_STATUS set_graphics_mode(void) {
    // Get GOP protocol via LocateProtocol()
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID; 
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mode_info = NULL;
    UINTN mode_info_size = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
    EFI_STATUS status = 0;
    UINTN mode_index = 0;   // Current mode within entire menu of GOP mode choices;

    // Store found GOP mode info
    typedef struct {
        UINT32 width;
        UINT32 height;
    } Gop_Mode_Info;

    Gop_Mode_Info gop_modes[50];

    status = bs->LocateProtocol(&gop_guid, NULL, (VOID **)&gop);
    if (EFI_ERROR(status)) {
        eprintf(u"\r\nERROR: %x; Could not locate GOP! :(\r\n", status);
        return status;
    }

    // Overall screen loop
    while (true) {
        cout->ClearScreen(cout);

        // Write String
        printf(u"Graphics mode information:\r\n");

        // Get current GOP mode information
        status = gop->QueryMode(gop, 
                                gop->Mode->Mode, 
                                &mode_info_size, 
                                &mode_info);

        if (EFI_ERROR(status)) {
            eprintf(u"\r\nERROR: %x; Could not Query GOP Mode %u\r\n", status, gop->Mode->Mode);
            return status;
        }

        // Print info
        printf(u"Max Mode: %d\r\n"
               u"Current Mode: %d\r\n"
               u"WidthxHeight: %ux%u\r\n"
               u"Framebuffer address: %x\r\n"
               u"Framebuffer size: %x\r\n"
               u"PixelFormat: %d\r\n"
               u"PixelsPerScanLine: %u\r\n",
               gop->Mode->MaxMode,
               gop->Mode->Mode,
               mode_info->HorizontalResolution, mode_info->VerticalResolution,
               gop->Mode->FrameBufferBase,
               gop->Mode->FrameBufferSize,
               mode_info->PixelFormat,
               mode_info->PixelsPerScanLine);

        cout->OutputString(cout, u"\r\nAvailable GOP modes:\r\n");

        // Get current text mode ColsxRows values
        UINTN menu_top = cout->Mode->CursorRow, menu_bottom = 0, max_cols;
        cout->QueryMode(cout, cout->Mode->Mode, &max_cols, &menu_bottom);

        // Print keybinds at bottom of screen
        cout->SetCursorPosition(cout, 0, menu_bottom-3);
        printf(u"Up/Down Arrow = Move Cursor\r\n"
               u"Enter = Select\r\n"
               u"Escape = Go Back");

        cout->SetCursorPosition(cout, 0, menu_top);
        menu_bottom -= 5;   // Bottom of menu will be 2 rows above keybinds
        UINTN menu_len = menu_bottom - menu_top;

        // Get all available GOP modes' info
        const UINT32 max = gop->Mode->MaxMode;
        if (max < menu_len) {
            // Bound menu by actual # of available modes
            menu_bottom = menu_top + max-1;
            menu_len = menu_bottom - menu_top;  // Limit # of modes in menu to max mode - 1
        }

        for (UINT32 i = 0; i < ARRAY_SIZE(gop_modes) && i < max; i++) {
            gop->QueryMode(gop, i, &mode_info_size, &mode_info);

            gop_modes[i].width = mode_info->HorizontalResolution;
            gop_modes[i].height = mode_info->VerticalResolution;
        }

        // Highlight top menu row to start off
        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
        printf(u"Mode %d: %dx%d", 0, gop_modes[0].width, gop_modes[0].height);

        // Print other text mode infos
        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
        for (UINT32 i = 1; i < menu_len + 1; i++) 
            printf(u"\r\nMode %d: %dx%d", i, gop_modes[i].width, gop_modes[i].height);

        // Get input from user 
        cout->SetCursorPosition(cout, 0, menu_top);
        bool getting_input = true;
        while (getting_input) {
            UINTN current_row = cout->Mode->CursorRow;

            EFI_INPUT_KEY key = get_key();
            switch (key.ScanCode) {
                case SCANCODE_ESC: return EFI_SUCCESS;  // ESC Key: Go back to main menu

                case SCANCODE_UP_ARROW:
                    if (current_row == menu_top && mode_index > 0) {
                        // Scroll menu up by decrementing all modes by 1
                        printf(u"                    \r");  // Blank out mode text first

                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        mode_index--;
                        printf(u"Mode %d: %dx%d", 
                               mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);

                        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                        UINTN temp_mode = mode_index + 1;
                        for (UINT32 i = 0; i < menu_len; i++, temp_mode++) {
                            printf(u"\r\n                    \r"  // Blank out mode text first
                                   u"Mode %d: %dx%d\r", 
                                   temp_mode, gop_modes[temp_mode].width, gop_modes[temp_mode].height);
                        }

                        // Reset cursor to top of menu
                        cout->SetCursorPosition(cout, 0, menu_top);

                    } else if (current_row-1 >= menu_top) {
                        // De-highlight current row, move up 1 row, highlight new row
                        printf(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r", 
                               mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);

                        mode_index--;
                        current_row--;
                        cout->SetCursorPosition(cout, 0, current_row);
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r", 
                               mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
                    }

                    // Reset colors
                    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    break;

                case SCANCODE_DOWN_ARROW:
                    // NOTE: Max valid GOP mode is ModeMax-1 per UEFI spec
                    if (current_row == menu_bottom && mode_index < max-1) {
                        // Scroll menu down by incrementing all modes by 1
                        mode_index -= menu_len - 1;

                        // Reset cursor to top of menu
                        cout->SetCursorPosition(cout, 0, menu_top);

                        // Print modes up until the last menu row
                        for (UINT32 i = 0; i < menu_len; i++, mode_index++) {
                            printf(u"                    \r"    // Blank out mode text first
                                   u"Mode %d: %dx%d\r\n", 
                                   mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
                        }

                        // Highlight last row
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r", 
                               mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);

                    } else if (current_row+1 <= menu_bottom) {
                        // De-highlight current row, move down 1 row, highlight new row
                        printf(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r\n", 
                               mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);

                        mode_index++;
                        current_row++;
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf(u"                    \r"    // Blank out mode text first
                               u"Mode %d: %dx%d\r", 
                               mode_index, gop_modes[mode_index].width, gop_modes[mode_index].height);
                    }

                    // Reset colors
                    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    break;

                default:
                    if (key.UnicodeChar == u'\r') {
                        // Enter key, set GOP mode
                        gop->SetMode(gop, mode_index);
                        gop->QueryMode(gop, mode_index, &mode_info_size, &mode_info);

                        // Clear GOP screen - EFI_BLUE seems to have a hex value of 0x98
                        EFI_GRAPHICS_OUTPUT_BLT_PIXEL px = { 0x00, 0x00, 0x00, 0x00 };  // BGR_8888
                        gop->Blt(gop, &px, EfiBltVideoFill, 
                                 0, 0,  // Origin BLT BUFFER X,Y
                                 0, 0,  // Destination screen X,Y
                                 mode_info->HorizontalResolution, mode_info->VerticalResolution,
                                 0);

                        getting_input = false;  // Will leave input loop and redraw screen
                        mode_index = 0;         // Reset last selected mode in menu
                    }
                    break;
            }
        }
    }

    return EFI_SUCCESS;
}


VOID copy_sprite_mask(Sprite *sprite, EFI_GRAPHICS_OUTPUT_BLT_PIXEL *dest, INT16 dest_pitch, INT16 dest_height, UINT32 mask)
{
    INT16 dst_x1 = sprite->x_pos;
    INT16 dst_y1 = sprite->y_pos;
    INT16 dst_x2 = sprite->x_pos + sprite->width - 1;
    INT16 dst_y2 = sprite->y_pos + sprite->height - 1;
    INT16 src_x1 = 0;
    INT16 src_y1 = 0;

    if (dst_x1 >= dest_pitch)
        return;
    if (dst_x2 < 0)
        return;
    if (dst_y1 >= dest_height)
        return;
    if (dst_y2 < 0)
        return;

    if (dst_x1 < 0)
    {
        src_x1 -= dst_x1;
        dst_x1 = 0;
    }
    if (dst_y1 < 0)
    {
        src_y1 -= dst_y1;
        dst_y1 = 0;
    }
    if (dst_x2 >= dest_pitch)
        dst_x2 = dest_pitch - 1;
    if (dst_y2 >= dest_height)
        dst_y2 = dest_height - 1;

    INT16 clipped_width = dst_x2 - dst_x1 + 1;
    INT16 dst_next_row = dest_pitch - clipped_width;
    INT16 src_next_row = sprite->width - clipped_width;

    UINT32 *dst_pixel = dest + dst_y1 * dest_pitch + dst_x1;
    UINT32 *src_pixel = sprite->image + src_y1 * sprite->width + src_x1;

    for (INT16 y = dst_y1; y <= dst_y2; y++)
    {
        for(INT16 i = 0; i<clipped_width; i++)
        {
            UINT32 src_color = *src_pixel;
            UINT32 dst_color = *dst_pixel;
            *dst_pixel = src_color != mask ? src_color : dst_color;
            src_pixel++;
            dst_pixel++;
        }
        dst_pixel += dst_next_row;
        src_pixel += src_next_row;
    }

    // for (UINT32 i = 0; i < sprite->height; i++)
    // {
    //     for (UINT32 j = 0; j < sprite->width; j++)
    //     {
    //         EFI_GRAPHICS_OUTPUT_BLT_PIXEL val = sprite->image[sprite->width * i + j];
    //         if (*((UINT32 *)(&val)) ^ mask)
    //         {
    //             UINT32 yidx = i + sprite->y_pos;
    //             UINT32 xidx = j + sprite->x_pos;

    //             if (xidx < dest_pitch && yidx < dest_height)
    //             {
    //                 UINT32 index = yidx * dest_pitch + xidx;
    //                 dest[index] = val;
    //             }
    //         }
    //     }
    // }
}

VOID load_asset_image(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *source, UINT32 src_pitch, Sprite *sprite)
{
    bs->AllocatePool(EfiLoaderData, sprite->width * sprite->height * 4, &sprite->image);

    for (UINT32 i = 0; i < sprite->height; i++) {
        for (UINT32 j = 0; j < sprite->width; j++)
        {
            sprite->image[sprite->width * i + j] = source[src_pitch * (i + sprite->img_y_offset) + j + sprite->img_x_offset];
        }
    }
}

BOOLEAN is_overlap(Sprite *bird, Sprite *sp2){
    INT16 sp1left = bird->x_pos + 4;
    INT16 sp1right = bird->x_pos + bird->width - 4;
    INT16 sp1top = bird->y_pos + 4;
    INT16 sp1bottom = bird->y_pos+bird->height - 12;

    // INT32 sp1left = sp1->x_pos;
    // INT32 sp1right = sp1->x_pos + sp1->width;
    // INT32 sp1top = sp1->y_pos;
    // INT32 sp1bottom = sp1->y_pos+sp1->height;

    INT16 sp2left = sp2->x_pos;
    INT16 sp2right = sp2->x_pos + sp2->width;
    INT16 sp2top = sp2->y_pos;
    INT16 sp2bottom = sp2->y_pos+sp2->height;


    if(sp1left < sp2right && sp1right > sp2left && sp1top < sp2bottom && sp1bottom > sp2top){
        return TRUE;
    }else{
        return FALSE;
    }
}

BOOLEAN is_collide(Sprite *bird, Obstacle *obs){
    obs->pipe_down.x_pos = obs->x_pos;
    obs->pipe_down.y_pos = obs->y_offset;
    obs->pipe_up.y_pos = obs->y_offset + 380;
    obs->pipe_up.x_pos = obs->x_pos;

    return is_overlap(bird, &obs->pipe_down) || is_overlap(bird, &obs->pipe_up);
}

BOOLEAN is_out_of_bounds(Sprite * bird, INT16 height){
    if(bird->y_pos + bird->height<0 || bird->y_pos > height){
        return TRUE;
    }else{
        return FALSE;
    }
}

VOID render_obstacle(Obstacle *obs, EFI_GRAPHICS_OUTPUT_BLT_PIXEL *fb, INT16 dest_pitch, INT16 dest_height, UINT32 mask){
    obs->pipe_up.y_pos = obs->y_offset + 380;
    obs->pipe_up.x_pos = obs->x_pos;
    obs->pipe_down.x_pos = obs->x_pos;
    obs->pipe_down.y_pos = obs->y_offset;
    copy_sprite_mask(&obs->pipe_down, fb, dest_pitch, dest_height, mask);
    copy_sprite_mask(&obs->pipe_up, fb, dest_pitch, dest_height, mask);
}

// ================================================
// Test the game
// ================================================
EFI_STATUS test_flaefi(void) {
    // Get the Loaded Image protocol for this EFI image/application itself,
    //   in order to get the device handle to use for the Simple File System Protocol
    EFI_STATUS status = EFI_SUCCESS;
    EFI_GUID lip_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *lip = NULL;
    status = bs->OpenProtocol(image,
                              &lip_guid,
                              (VOID **)&lip,
                              image,
                              NULL,
                              EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status)) {
        eprintf(u"Error %x; Could not open Loaded Image Protocol\r\n", status);
        get_key();
        return status;
    }

    // Get Simple File System Protocol for device handle for this loaded
    //   image, to open the root directory for the ESP
    EFI_GUID sfsp_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp = NULL;
    status = bs->OpenProtocol(lip->DeviceHandle,
                              &sfsp_guid,
                              (VOID **)&sfsp,
                              image,
                              NULL,
                              EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status)) {
        eprintf(u"Error %x; Could not open Simple File System Protocol\r\n", status);
        get_key();
        return status;
    }

    // Open root directory via OpenVolume()
    EFI_FILE_PROTOCOL *dirp = NULL;
    status = sfsp->OpenVolume(sfsp, &dirp);
    if (EFI_ERROR(status)) {
        eprintf(u"Error %x; Could not Open Volume for root directory\r\n", status);
        get_key();
        goto cleanup;
    }

    EFI_FILE_PROTOCOL *efp;
    status = dirp->Open(dirp, 
                    &efp, 
                    u"EFI\\BOOT\\sprites.bmp", 
                    EFI_FILE_MODE_READ,
                    0);

    if (EFI_ERROR(status)) {
        eprintf(u"Error %x; Could not open file %s\r\n", 
                status, u"bleckceckh");
        get_key();
        goto cleanup;
    }
    BMPImage *img = read_bmp(efp);

    const int width = img->header.width_px;
    const int height = img->header.height_px;

    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *asset_arr;
    status = bs->AllocatePool(EfiLoaderData, height * width *4, &asset_arr);
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            asset_arr[width*i + j].Blue = img->data[(width*3+2)*(height-i-1) + 3*j];
            asset_arr[width*i + j].Green = img->data[(width*3+2)*(height-i-1) + 3*j + 1];
            asset_arr[width*i + j].Red = img->data[(width*3+2)*(height-i-1) + 3*j + 2];
            asset_arr[width*i + j].Reserved = 0xFF;
        }
        
    }

    free_bmp(img);
    

    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID; 
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    bs->LocateProtocol(&gop_guid, NULL, (VOID **)&gop);

    EFI_EVENT start_timer;
    bs->CreateEvent(EVT_TIMER, 0, NULL, NULL, &start_timer);
    bs->SetTimer(start_timer, TimerPeriodic, 10000000/2);

    EFI_EVENT start_events[2];
    start_events[1] = start_timer;
    UINTN startindex = 1;


    EFI_INPUT_KEY start_key;

    start_events[0] = cin->WaitForKey;

    UINT8 score = 0;
    
    INT16 fb_width = 576;
    INT16 fb_height = 512;


    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *framebuffer;
    status = bs->AllocatePool(EfiLoaderData, fb_width*fb_height *4, &framebuffer);


    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *doublebuffer;
    status = bs->AllocatePool(EfiLoaderData, fb_width*fb_height*4 *4, &doublebuffer);

    Sprite digits[10];
    for (int i = 0; i <10; i++)
    {
        digits[i] = sprites[i + ASSET_ZERO];
        load_asset_image(asset_arr, width, &digits[i]);
        digits[i].x_pos = fb_width/2 - digits[i].width/2;
        digits[i].y_pos = 50;
    }

    Sprite bird_1 = sprites[ASSET_BIRD_1];
    load_asset_image(asset_arr,width, &bird_1);

    Sprite bird_2 = sprites[ASSET_BIRD_2];
    load_asset_image(asset_arr,width, &bird_2);

    Sprite bird_3 = sprites[ASSET_BIRD_3];
    load_asset_image(asset_arr,width, &bird_3);

    Sprite bird = sprites[ASSET_BIRD_1];
    bird.image = bird_1.image;

    Sprite background = sprites[ASSET_BG];
    load_asset_image(asset_arr,width, &background);

    Sprite pipe_down = sprites[ASSET_PIPE_DOWN];
    load_asset_image(asset_arr,width, &pipe_down);

    Sprite pipe_up = sprites[ASSET_PIPE_UP];
    load_asset_image(asset_arr,width, &pipe_up);

    Sprite logo = sprites[ASSET_LOGO];
    load_asset_image(asset_arr, width, &logo);

    Sprite tap = sprites[ASSET_TAP];
    load_asset_image(asset_arr, width, &tap);

    Sprite game_over = sprites[ASSET_GAME_OVER];
    load_asset_image(asset_arr, width, &game_over);

    Sprite digit_1 = sprites[ASSET_ONE];
    load_asset_image(asset_arr, width, &digit_1);

    Obstacle obs1 = {.pipe_down = pipe_down, .pipe_up = pipe_up, .x_pos = 570, .y_offset = -100, .can_score = TRUE};

    Obstacle obs2 = {.pipe_down = pipe_down, .pipe_up = pipe_up, .x_pos = 770, .y_offset = 0, .can_score = TRUE};

    Obstacle obs3 = {.pipe_down = pipe_down, .pipe_up = pipe_up, .x_pos = 970, .y_offset = -50, .can_score = TRUE};


    bs->FreePool(asset_arr);

    bird.x_pos = fb_width/2 - bird.width/2;
    bird.y_pos = fb_height/2 - bird.height/2;

    logo.x_pos = fb_width/2 - logo.width/2;
    logo.y_pos = 50;

    tap.x_pos = fb_width/2 - tap.width/2;
    tap.y_pos = fb_height-100;

    game_over.x_pos = fb_width/2 - game_over.width/2;
    game_over.y_pos = 50;

    int parity = 1;


    while (1)
    {
        if (startindex == 0)
        {
            cin->ReadKeyStroke(cin, &start_key);
            bs->SetTimer(start_timer, TimerCancel, 0);
            bs->CloseEvent(start_timer);
            break;
        }
        else
        {
            parity = !parity;
            copy_sprite_mask(&background, framebuffer, fb_width, fb_height, MAGENTA_MASK);
            copy_sprite_mask(&bird, framebuffer, fb_width, fb_height, MAGENTA_MASK);
            copy_sprite_mask(&logo, framebuffer, fb_width, fb_height, MAGENTA_MASK);

            if (parity & 1)
            {
                copy_sprite_mask(&tap, framebuffer, fb_width, fb_height, MAGENTA_MASK);
            }

            for (int i = 0; i < fb_height * 2; i++)
            {
                for (int j = 0; j < fb_width * 2; j++)
                {
                    doublebuffer[fb_width * 2 * i + j] = framebuffer[fb_width * (i / 2) + j / 2];
                }
            }

            gop->Blt(gop, doublebuffer, EfiBltBufferToVideo,
                     0, 0, // Origin BLT BUFFER X,Y
                     0, 0, // Destination screen X,Y
                     fb_width * 2, fb_height * 2,
                     0);
        }
        startindex = 0;
        bs->WaitForEvent(2, start_events, &startindex);

    }
   


    EFI_EVENT timer_event;

    // Create timer event, to print date/time on screen every ~1second
    bs->CreateEvent(EVT_TIMER,
                    0,
                    NULL,
                    NULL,
                    &timer_event);

    // Set Timer for the timer event to run every 1/60 second (in 100ns units)
    bs->SetTimer(timer_event, TimerPeriodic, 10000000/60);

    EFI_EVENT events[2];

    events[1] = timer_event;
    UINTN index = 0;

    EFI_INPUT_KEY key;

    events[0] = cin->WaitForKey;




    
    UINT32 counter = 0;
    int speed = -3;
    int pilspeed = 1;

    int bird_selector = 0;
    while (1)
    {
        bs->WaitForEvent(2, events, &index);
        if (index == 0)
        {
            cin->ReadKeyStroke(cin, &key);

            if(key.ScanCode == SCANCODE_ESC){
                return EFI_SUCCESS;
            }
            
            if (speed >= 0)
            {
                speed = -3;
            }
            else if (speed > -10)
            {
                speed -= 3;
            }
        }
        else
        {
  

            copy_sprite_mask(&background, framebuffer, fb_width, fb_height, MAGENTA_MASK);
            // copy_sprite_mask(&pipe_up, framebuffer, fb_width, fb_height, MAGENTA_MASK);
            // copy_sprite_mask(&pipe_down, framebuffer, fb_width, fb_height, MAGENTA_MASK);
            render_obstacle(&obs1, framebuffer,fb_width, fb_height, MAGENTA_MASK );
            render_obstacle(&obs2, framebuffer, fb_width, fb_height, MAGENTA_MASK);
            render_obstacle(&obs3, framebuffer, fb_width, fb_height, MAGENTA_MASK);

            copy_sprite_mask(&bird, framebuffer, fb_width, fb_height, MAGENTA_MASK);
            copy_sprite_mask(&digits[score], framebuffer, fb_width, fb_height, MAGENTA_MASK);

            for (int i = 0; i < fb_height * 2; i++)
            {
                for (int j = 0; j < fb_width * 2; j++)
                {
                    doublebuffer[fb_width * 2 * i + j] = framebuffer[fb_width * (i / 2) + j / 2];
                }
            }

            gop->Blt(gop, doublebuffer, EfiBltBufferToVideo,
                     0, 0, // Origin BLT BUFFER X,Y
                     0, 0, // Destination screen X,Y
                     fb_width * 2, fb_height * 2,
                     0);
                     
            if (is_collide(&bird, &obs1) || is_collide(&bird, &obs2) || is_collide(&bird, &obs3) || is_out_of_bounds(&bird, fb_height))
            {
                copy_sprite_mask(&game_over, framebuffer, fb_width, fb_height, MAGENTA_MASK);

                for (int i = 0; i < fb_height * 2; i++)
                {
                    for (int j = 0; j < fb_width * 2; j++)
                    {
                        doublebuffer[fb_width * 2 * i + j] = framebuffer[fb_width * (i / 2) + j / 2];
                    }
                }

                gop->Blt(gop, doublebuffer, EfiBltBufferToVideo,
                         0, 0, // Origin BLT BUFFER X,Y
                         0, 0, // Destination screen X,Y
                         fb_width * 2, fb_height * 2,
                         0);
                get_key();
                return EFI_SUCCESS;

            }

            obs1.x_pos -= pilspeed;
            obs2.x_pos -= pilspeed;
            obs3.x_pos -= pilspeed;

            if (obs1.x_pos + obs1.pipe_down.width - 6 < 0)
            {
                obs1.x_pos = 570;
                obs1.can_score = TRUE;
            }
            else if (obs2.x_pos + obs2.pipe_down.width - 6 < 0)
            {
                obs2.x_pos = 570;
                obs2.can_score = TRUE;
            }
            else if (obs3.x_pos + obs3.pipe_down.width - 6 < 0)
            {
                obs3.x_pos = 570;
                obs3.can_score = TRUE;
            }

            
            if(obs1.x_pos < fb_width/2 && obs1.can_score){
                score++;
                obs1.can_score = FALSE;
            }
            if(obs2.x_pos < fb_width/2 && obs2.can_score){
                score++;
                obs2.can_score = FALSE;
            }
            if(obs3.x_pos < fb_width/2 && obs3.can_score){
                score++;
                obs3.can_score = FALSE;
            }

            // printf(u"\r%u", score);



            bird.y_pos += speed;
            if (speed < 10 && counter++ == 4)
            {
                speed++;
                counter = 0;
            }


            if (bird_selector < 20)
            {
                bird.image = bird_2.image;
            }
            else if (bird_selector < 40)
            {
                bird.image = bird_1.image;
            }
            else if (bird_selector < 60)
            {
                bird.image = bird_3.image;
            }else{
                bird_selector = 0;
            }
            
            bird_selector++;


        }
    }

cleanup:
    // Close open protocols
    bs->CloseProtocol(lip->DeviceHandle,
                      &sfsp_guid,
                      image,
                      NULL);

    bs->CloseProtocol(image,
                      &lip_guid,
                      image,
                      NULL);


    return status;
}



// ====================
// Entry Point
// ====================
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // Initialize global variables
    init_global_variables(ImageHandle, SystemTable);

    // Reset Console Inputs/Outputs
    cin->Reset(cin, FALSE);
    cout->Reset(cout, FALSE);
    cout->Reset(cerr, FALSE);

    // Set text colors - foreground, background
    cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));

    // Disable Watchdog Timer
    bs->SetWatchdogTimer(0, 0x10000, 0, NULL);

    // Screen loop
    bool running = true;
    while (running) {
        EFI_GUID lip_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
        EFI_LOADED_IMAGE_PROTOCOL *lip = NULL;
        bs->OpenProtocol(image,
                         &lip_guid,
                         (VOID **)&lip,
                         image,
                         NULL,
                         EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
        
        printf(u"%x\r\n", lip->ImageBase);
        get_key();
        // Menu text on screen
        const CHAR16 *menu_choices[] = {
            u"Set Graphics Mode",
            u"Test flaEFI"
        };

        // Functions to call for each menu option
        EFI_STATUS (*menu_funcs[])(void) = {
            set_graphics_mode,
            test_flaefi
        };

        // Clear console output; clear screen to background color and
        //   set cursor to 0,0
        cout->ClearScreen(cout);

        // Get current text mode ColsxRows values
        UINTN cols = 0, rows = 0;
        cout->QueryMode(cout, cout->Mode->Mode, &cols, &rows);

        // Print keybinds at bottom of screen
        cout->SetCursorPosition(cout, 0, rows-3);
        printf(u"Up/Down Arrow = Move cursor\r\n"
               u"Enter = Select\r\n"
               u"Escape = Shutdown");

        // Print menu choices
        // Highlight first choice as initial choice
        cout->SetCursorPosition(cout, 0, 0);
        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
        printf(u"%s", menu_choices[0]);

        // Print rest of choices
        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
        for (UINTN i = 1; i < ARRAY_SIZE(menu_choices); i++)
            printf(u"\r\n%s", menu_choices[i]);

        // Get cursor row boundaries
        INTN min_row = 0, max_row = cout->Mode->CursorRow;

        // Input loop
        cout->SetCursorPosition(cout, 0, 0);
        bool getting_input = true;
        while (getting_input) {
            INTN current_row = cout->Mode->CursorRow;
            EFI_INPUT_KEY key = get_key();

            // Process input
            switch (key.ScanCode) {
                case SCANCODE_UP_ARROW:
                    if (current_row-1 >= min_row) {
                        // De-highlight current row, move up 1 row, highlight new row
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                        printf(u"%s\r", menu_choices[current_row]);

                        current_row--;
                        cout->SetCursorPosition(cout, 0, current_row);
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf(u"%s\r", menu_choices[current_row]);

                        // Reset colors
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    }
                    break;

                case SCANCODE_DOWN_ARROW:
                    if (current_row+1 <= max_row) {
                        // De-highlight current row, move down 1 row, highlight new row
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                        printf(u"%s\r", menu_choices[current_row]);

                        current_row++;
                        cout->SetCursorPosition(cout, 0, current_row);
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(HIGHLIGHT_FG_COLOR, HIGHLIGHT_BG_COLOR));
                        printf(u"%s\r", menu_choices[current_row]);

                        // Reset colors
                        cout->SetAttribute(cout, EFI_TEXT_ATTR(DEFAULT_FG_COLOR, DEFAULT_BG_COLOR));
                    }
                    break;

                case SCANCODE_ESC:
                    // Escape key: power off
                    rs->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

                    // !NOTE!: This should not return, system should power off
                    break;

                default:
                    if (key.UnicodeChar == u'\r') {
                        // Enter key, select choice
                        EFI_STATUS return_status = menu_funcs[current_row]();
                        if (EFI_ERROR(return_status)) {
                            eprintf(u"ERROR %x\r\n; Press any key to go back...", return_status);
                            get_key();
                        }

                        // Will leave input loop and reprint main menu
                        getting_input = false; 
                    }
                    break;
            }
        }
    }

    // End program
    return EFI_SUCCESS;
}
