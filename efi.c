#include "ulibc.h"
#include "efi.h"
#include "bmp.h"

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



// EFI_GRAPHICS_OUTPUT_BLT_PIXEL values, BGRr8888
#define px_LGRAY {0xEE,0xEE,0xEE,0x00}
#define px_BLACK {0x00,0x00,0x00,0x00}
#define px_BLUE  {0x98,0x00,0x00,0x00}  // EFI_BLUE

// -----------------
// Global variables
// -----------------
extern EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cout;  // Console output
extern EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *cin; // Console input
extern EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *cerr;  // Console output - stderr
EFI_BOOT_SERVICES    *bs;   // Boot services
EFI_RUNTIME_SERVICES *rs;   // Runtime services
EFI_HANDLE image = NULL;    // Image handle

// Mouse cursor buffer 8x8
EFI_GRAPHICS_OUTPUT_BLT_PIXEL cursor_buffer[] = {
    px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, // Line 1
    px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, // Line 2
    px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_BLUE, px_BLUE, px_BLUE, px_BLUE,     // Line 3
    px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_LGRAY, px_BLUE, px_BLUE, px_BLUE,    // Line 4
    px_LGRAY, px_LGRAY, px_BLUE, px_LGRAY, px_LGRAY, px_LGRAY, px_BLUE, px_BLUE,    // Line 5
    px_LGRAY, px_LGRAY, px_BLUE, px_BLUE, px_LGRAY, px_LGRAY, px_LGRAY, px_BLUE,    // Line 6
    px_LGRAY, px_LGRAY, px_BLUE, px_BLUE, px_BLUE, px_LGRAY, px_LGRAY, px_LGRAY,    // Line 7
    px_LGRAY, px_LGRAY, px_BLUE, px_BLUE, px_BLUE, px_BLUE, px_LGRAY, px_LGRAY,     // Line 8
};


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
// Set Text Mode
// ====================
EFI_STATUS set_text_mode(void) {
    // Overall screen loop
    while (true) {
        cout->ClearScreen(cout);

        // Write String
        cout->OutputString(cout, u"Text mode information:\r\n");
        UINTN max_cols = 0, max_rows = 0;

        // Get current text mode's column and row counts
        cout->QueryMode(cout, cout->Mode->Mode, &max_cols, &max_rows);

        printf(u"Max Mode: %d\r\n"
               u"Current Mode: %d\r\n"
               u"Attribute: %x\r\n" 
               u"CursorColumn: %d\r\n"
               u"CursorRow: %d\r\n"
               u"CursorVisible: %d\r\n"
               u"Columns: %d\r\n"
               u"Rows: %d\r\n\r\n",
               cout->Mode->MaxMode,
               cout->Mode->Mode,
               cout->Mode->Attribute,
               cout->Mode->CursorColumn,
               cout->Mode->CursorRow,
               cout->Mode->CursorVisible,
               max_cols,
               max_rows);

        cout->OutputString(cout, u"Available text modes:\r\n");

        // Print other text mode infos
        const INT32 max = cout->Mode->MaxMode;
        for (INT32 i = 0; i < max; i++) {
            cout->QueryMode(cout, i, &max_cols, &max_rows);
            printf(u"Mode #: %d, %dx%d\r\n", i, max_cols, max_rows);
        }

        // Get number from user
        while (true) {
            static UINTN current_mode = 0;
            current_mode = cout->Mode->Mode;

            for (UINTN i = 0; i < 79; i++) printf(u" ");
            printf(u"\rSelect Text Mode # (0-%d): %d", max, current_mode);

            // Move cursor left by 1, to overwrite the mode #
            cout->SetCursorPosition(cout, cout->Mode->CursorColumn-1, cout->Mode->CursorRow);

            EFI_INPUT_KEY key = get_key();

            // Get key info
            CHAR16 cbuf[2];
            cbuf[0] = key.UnicodeChar;
            cbuf[1] = u'\0';
            //printf(u"Scancode: %x, Unicode Char: %s\r", key.ScanCode, cbuf);

            // Process keystroke
            printf(u"%s ", cbuf);

            if (key.ScanCode == SCANCODE_ESC) {
                // Go back to main menu
                return EFI_SUCCESS;
            }

            // Choose text mode & redraw screen
            current_mode = key.UnicodeChar - u'0';
            EFI_STATUS status = cout->SetMode(cout, current_mode);
            if (EFI_ERROR(status)) {
                // Handle errors
                if (status == EFI_DEVICE_ERROR) { 
                    eprintf(u"ERROR: %x; Device Error", status);
                } else if (status == EFI_UNSUPPORTED) {
                    eprintf(u"ERROR: %x; Mode # is invalid", status);
                }
                eprintf(u"\r\nPress any key to select again", status);
                get_key();

            } 

            // Set new mode, redraw screen from outer loop
            break;
        }
    }

    return EFI_SUCCESS;
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
                        EFI_GRAPHICS_OUTPUT_BLT_PIXEL px = { 0x98, 0x00, 0x00, 0x00 };  // BGR_8888
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

VOID copy_buf_mask(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *source, EFI_GRAPHICS_OUTPUT_BLT_PIXEL *dest,
                  int source_x_offset, int source_y_offset, int dest_x_offset, int dest_y_offset,
                  int source_pitch, int dest_pitch,int dest_height, int width, int height, EFI_GRAPHICS_OUTPUT_BLT_PIXEL mask)
{
    for (int i = source_y_offset; i < source_y_offset + height; i++)
    {
        for (int j = source_x_offset; j < source_x_offset + width; j++)
        {
            EFI_GRAPHICS_OUTPUT_BLT_PIXEL val = source[source_pitch * i + j];
            if (val.Red == mask.Red && val.Blue == mask.Blue && val.Green == mask.Green)
            {
                continue;
            }
            else
            {
                UINT32 yidx = i - source_y_offset + dest_y_offset;
                UINT32 xidx = j - source_x_offset + dest_x_offset;

                if(xidx>= dest_pitch || yidx>=dest_height){
                    continue;
                }
                int index = yidx * dest_pitch + xidx;
                dest[index] = val;
            }
        }
    }
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
        }
        
    }

    free_bmp(img);
    

    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID; 
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    bs->LocateProtocol(&gop_guid, NULL, (VOID **)&gop);



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



    
    int tw = 576;
    int th = 512;
    int xoff = 0;
    int yoff = 0;


    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *framebuffer;
    status = bs->AllocatePool(EfiLoaderData, tw*th *4, &framebuffer);

    // copyarr_mask(pixelarr, framebuffer, )

    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *doublebuffer;
    status = bs->AllocatePool(EfiLoaderData, tw*th*4 *4, &doublebuffer);

    UINT32 counter = 0;
    int speed = 5;
    int birdyoff = 200;
    int pilxoff = 570;
    int pilspeed = 1;
    while (1)
    {
        bs->WaitForEvent(2, events, &index);
        if (index == 0)
        {
            cin->ReadKeyStroke(cin, &key);
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
            copy_buf_mask(asset_arr, framebuffer, xoff, yoff, 0, 0, width, tw,th, tw, th, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL){0xff, 0x00, 0xff, 0x00});
            
            int upw = 52;
            int uph = 242;
            int upxoff = 948;
            int upyoff = 0;

            copy_buf_mask(asset_arr, framebuffer, upxoff, upyoff, pilxoff-pilspeed, 350, width, tw,th, upw, uph, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL){0xff, 0x00, 0xff, 0x00});
        
            int dpw = 52;
            int dph = 270;
            int dpxoff = 892;
            int dpyoff = 0;

            copy_buf_mask(asset_arr, framebuffer, dpxoff, dpyoff, pilxoff-pilspeed,0, width, tw,th, dpw, dph, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL){0xff, 0x00, 0xff, 0x00});
            pilxoff -= pilspeed;
            if(pilxoff + dpw-6<0){
                pilxoff = 570;
            }
            int btw = 34;
            int bth = 32;
            int bxoff = 816;
            int byoff = 180;
            copy_buf_mask(asset_arr, framebuffer, bxoff, byoff, tw / 2 - btw / 2, birdyoff + speed, width, tw, th, btw, bth, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL){0xff, 0x00, 0xff, 0x00});
            birdyoff += speed;
        
            if (speed < 10 && counter++ == 4)
            {
                speed++;
                counter = 0;
            }

            for (int i = 0; i < th * 2; i++)
            {
                for (int j = 0; j < tw * 2; j++)
                {
                    doublebuffer[tw * 2 * i + j] = framebuffer[tw * (i / 2) + j / 2];
                }
            }

            gop->Blt(gop, doublebuffer, EfiBltBufferToVideo,
                     0, 0, // Origin BLT BUFFER X,Y
                     0, 0, // Destination screen X,Y
                     tw * 2, th * 2,
                     0);
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



// ===========================================================
// Timer function to print current date/time every 1 second
// ===========================================================
VOID EFIAPI print_datetime(IN EFI_EVENT event, IN VOID *Context) {
    (VOID)event; // Suppress compiler warning

    // Timer context will be the text mode screen bounds
    typedef struct {
        UINT32 rows; 
        UINT32 cols;
    } Timer_Context;

    Timer_Context context = *(Timer_Context *)Context;

    // Save current cursor position before printing date/time
    UINT32 save_col = cout->Mode->CursorColumn, save_row = cout->Mode->CursorRow;

    // Get current date/time
    EFI_TIME time;
    EFI_TIME_CAPABILITIES capabilities;
    rs->GetTime(&time, &capabilities);

    // Move cursor to print in lower right corner
    cout->SetCursorPosition(cout, context.cols-20, context.rows-1);

    // Print current date/time
    printf(u"%u-%c%u-%c%u %c%u:%c%u:%c%u",
            time.Year, 
            time.Month  < 10 ? u'0' : u'\0', time.Month,
            time.Day    < 10 ? u'0' : u'\0', time.Day,
            time.Hour   < 10 ? u'0' : u'\0', time.Hour,
            time.Minute < 10 ? u'0' : u'\0', time.Minute,
            time.Second < 10 ? u'0' : u'\0', time.Second);

    // Restore cursor position
    cout->SetCursorPosition(cout, save_col, save_row);
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
            u"Set Text Mode",
            u"Set Graphics Mode",
            u"Test flaEFI"
        };

        // Functions to call for each menu option
        EFI_STATUS (*menu_funcs[])(void) = {
            set_text_mode,
            set_graphics_mode,
            test_flaefi
        };

        // Clear console output; clear screen to background color and
        //   set cursor to 0,0
        cout->ClearScreen(cout);

        // Get current text mode ColsxRows values
        UINTN cols = 0, rows = 0;
        cout->QueryMode(cout, cout->Mode->Mode, &cols, &rows);

        // Timer context will be the text mode screen bounds
        typedef struct {
            UINT32 rows; 
            UINT32 cols;
        } Timer_Context;

        Timer_Context context;
        context.rows = rows;
        context.cols = cols;

        EFI_EVENT timer_event;

        // // Create timer event, to print date/time on screen every ~1second
        // bs->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL,
        //                 TPL_CALLBACK, 
        //                 print_datetime,
        //                 (VOID *)&context,
        //                 &timer_event);

        // // Set Timer for the timer event to run every 1 second (in 100ns units)
        // bs->SetTimer(timer_event, TimerPeriodic, 10000000);

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
                    // Close Timer Event for cleanup
                    bs->CloseEvent(timer_event);

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
