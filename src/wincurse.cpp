#pragma GCC diagnostic ignored "-Wunused-macros"
#if !defined(TILES) && defined(_WIN32)
#ifndef CMAKE
#define _UNICODE 1
#endif
#define UNICODE 1
#include "cursesport.h" // IWYU pragma: associated
#ifndef TUI
#include <cstdlib>
#include <fstream>

#include "cached_options.h"
#if defined(SDL_SOUND)
#include "sound_backend.h"
#endif
#include "cursesdef.h"
#include "options.h"
#include "output.h"
#include "color.h"
#include "catacharset.h"
#include "init.h"
#include "input.h"
#include "path_info.h"
#include "filesystem.h"
#include "debug.h"
#include "cata_scope_helpers.h"
#include "cata_utility.h"
#include "string_formatter.h"
#include "color_loader.h"
#include "font_loader.h"
#include "platform_win.h"
#include "mmsystem.h"
#include "ui_manager.h"
#include "wcwidth.h"


#if defined(SDL_SOUND)
#include "sdlsound.h"
#endif





static constexpr int ERR = -1;
const wchar_t *szWindowClass = L"CataCurseWindow";
HINSTANCE WindowINST;
HWND WindowHandle;
HDC WindowDC;
int WindowWidth;
int WindowHeight;
int lastchar;
int inputdelay;
HDC backbuffer;
HBITMAP backbit;
int fontwidth;
int fontheight;
int halfwidth;
int halfheight;
HFONT font;
std::array<RGBQUAD, color_loader<RGBQUAD>::COLOR_NAMES_COUNT> windowsPalette;
unsigned char *dcbits;
bool CursorVisible = true;
bool needs_resize = false;
bool initialized = false;

static int TERMINAL_WIDTH;
static int TERMINAL_HEIGHT;






LRESULT CALLBACK ProcessMessages( HWND__ *hWnd, std::uint32_t Msg, WPARAM wParam, LPARAM lParam );

static std::wstring widen( const std::string &s )
{
    if( s.empty() ) {

        return std::wstring();
    }
    std::vector<wchar_t> buffer( s.length() );
    const int newlen = MultiByteToWideChar( CP_UTF8, 0, s.c_str(), s.length(),
                                            buffer.data(), buffer.size() );

    return std::wstring( buffer.data(), newlen );
}

static constexpr uint32_t WndStyle = WS_CAPTION | WS_MINIMIZEBOX | WS_SIZEBOX |
                                     WS_MAXIMIZEBOX | WS_SYSMENU | WS_VISIBLE;


static bool WinCreate()
{

    WindowINST = GetModuleHandle( nullptr );


    WNDCLASSEXW WindowClassType   = WNDCLASSEXW();
    WindowClassType.cbSize        = sizeof( WNDCLASSEXW );

    WindowClassType.lpfnWndProc   = ProcessMessages;

    WindowClassType.hInstance     = WindowINST;

    WindowClassType.hIcon         = LoadIcon( WindowINST, MAKEINTRESOURCE( 0 ) );
    WindowClassType.hIconSm       = LoadIcon( WindowINST, MAKEINTRESOURCE( 0 ) );
    WindowClassType.hCursor       = LoadCursor( nullptr, IDC_ARROW );
    WindowClassType.lpszClassName = szWindowClass;
    if( !RegisterClassExW( &WindowClassType ) ) {
        return false;
    }



    RECT WndRect;
    WndRect.left   = WndRect.top = 0;
    WndRect.right  = WindowWidth;
    WndRect.bottom = WindowHeight;
    AdjustWindowRect( &WndRect, WndStyle, false );


    RECT WorkArea;
    SystemParametersInfo( SPI_GETWORKAREA, 0, &WorkArea, 0 );
    int WindowX = WorkArea.right / 2 - ( WndRect.right - WndRect.left ) / 2;
    int WindowY = WorkArea.bottom / 2 - ( WndRect.bottom - WndRect.top ) / 2;


    WindowHandle = CreateWindowExW( 0, szWindowClass, L"", WndStyle,
                                    WindowX, WindowY,
                                    WndRect.right - WndRect.left,
                                    WndRect.bottom - WndRect.top,
                                    nullptr, nullptr, WindowINST, nullptr );
    return WindowHandle != nullptr;

}


static void WinDestroy()
{
    if( ( WindowDC != nullptr ) && ( ReleaseDC( WindowHandle, WindowDC ) == 0 ) ) {
        WindowDC = nullptr;
    }
    if( WindowHandle != nullptr && ( !( DestroyWindow( WindowHandle ) ) ) ) {
        WindowHandle = nullptr;
    }
    if( !( UnregisterClassW( szWindowClass, WindowINST ) ) ) {
        WindowINST = nullptr;
    }
}


static void create_backbuffer()
{
    if( WindowDC != nullptr ) {
        ReleaseDC( WindowHandle, WindowDC );
    }
    if( backbuffer != nullptr ) {
        ReleaseDC( WindowHandle, backbuffer );
    }
    WindowDC   = GetDC( WindowHandle );
    backbuffer = CreateCompatibleDC( WindowDC );

    BITMAPINFO bmi = BITMAPINFO();
    bmi.bmiHeader.biSize         = sizeof( BITMAPINFOHEADER );
    bmi.bmiHeader.biWidth        = WindowWidth;
    bmi.bmiHeader.biHeight       = -WindowHeight;
    bmi.bmiHeader.biPlanes       = 1;
    bmi.bmiHeader.biBitCount     = 8;
    bmi.bmiHeader.biCompression  = BI_RGB;
    bmi.bmiHeader.biSizeImage    = WindowWidth * WindowHeight * 1;
    bmi.bmiHeader.biClrUsed      = color_loader<RGBQUAD>::COLOR_NAMES_COUNT;
    bmi.bmiHeader.biClrImportant = color_loader<RGBQUAD>::COLOR_NAMES_COUNT;
    backbit = CreateDIBSection( nullptr, &bmi, DIB_RGB_COLORS, reinterpret_cast<void **>( &dcbits ),
                                nullptr,
                                0 );
    DeleteObject( SelectObject( backbuffer, backbit ) );
}

bool handle_resize( int, int )
{
    if( !initialized ) {
        return false;
    }
    needs_resize = false;
    RECT WndRect;
    if( GetClientRect( WindowHandle, &WndRect ) ) {
        TERMINAL_WIDTH = WndRect.right / fontwidth;
        TERMINAL_HEIGHT = WndRect.bottom / fontheight;
        WindowWidth = TERMINAL_WIDTH * fontwidth;
        WindowHeight = TERMINAL_HEIGHT * fontheight;
        catacurses::stdscr = catacurses::newwin( TERMINAL_HEIGHT, TERMINAL_WIDTH, point::zero );
        catacurses::resizeterm();
        create_backbuffer();
        SetBkMode( backbuffer, TRANSPARENT );
        SelectObject( backbuffer, font );
        color_loader<RGBQUAD>().load( windowsPalette );
        if( SetDIBColorTable( backbuffer, 0, windowsPalette.size(), windowsPalette.data() ) == 0 ) {
            throw std::runtime_error( "SetDIBColorTable failed" );
        }
        ui_manager::screen_resized();
    }

    return true;
}

void resize_term( const int cell_w, const int cell_h )
{
    RECT WndRect;
    WndRect.left = WndRect.top = 0;
    WndRect.right = cell_w * fontwidth * get_scaling_factor();
    WndRect.bottom = cell_h * fontheight * get_scaling_factor();
    if( !AdjustWindowRect( &WndRect, WndStyle, false ) ) {
        return;
    }
    if( !SetWindowPos( WindowHandle, nullptr, 0, 0,
                       WndRect.right - WndRect.left, WndRect.bottom - WndRect.top,
                       SWP_NOMOVE | SWP_NOZORDER ) ) {
        return;
    }
    GetClientRect( WindowHandle, &WndRect );
    handle_resize( WndRect.right - WndRect.left, WndRect.bottom - WndRect.top );
}


#define ALT_BUFFER_SIZE 8
static char alt_buffer[ALT_BUFFER_SIZE] = {};
static int alt_buffer_len = 0;
static bool alt_down = false;
static bool shift_down = false;

static void begin_alt_code()
{
    alt_buffer[0] = '\0';
    alt_down = true;
    alt_buffer_len = 0;
}

static void add_alt_code( char c )
{

    if( c >= '0' && c <= '9' ) {
        if( alt_buffer_len < ALT_BUFFER_SIZE - 1 ) {
            alt_buffer[alt_buffer_len] = c;
            alt_buffer[++alt_buffer_len] = '\0';
        }
    }
}

static int end_alt_code()
{
    alt_down = false;
    return atoi( alt_buffer );
}


LRESULT CALLBACK ProcessMessages( HWND__ *hWnd, unsigned int Msg,
                                  WPARAM wParam, LPARAM lParam )
{
    uint16_t MouseOver;
    switch( Msg ) {
        case WM_DEADCHAR:
        case WM_CHAR:
            lastchar = static_cast<int>( wParam );
            switch( lastchar ) {
                case VK_TAB:
                    lastchar = ( shift_down ) ? KEY_BTAB : '\t';
                    break;
                case VK_RETURN:

                    lastchar = 10;
                    break;
                case VK_BACK:

                    lastchar = 127;
                    break;
            }
            return 0;

        case WM_KEYDOWN:

            switch( wParam ) {
                case VK_SHIFT:
                    shift_down = true;
                    break;
                case VK_LEFT:
                    lastchar = KEY_LEFT;
                    break;
                case VK_RIGHT:
                    lastchar = KEY_RIGHT;
                    break;
                case VK_UP:
                    lastchar = KEY_UP;
                    break;
                case VK_DOWN:
                    lastchar = KEY_DOWN;
                    break;
                case VK_NEXT:
                    lastchar = KEY_NPAGE;
                    break;
                case VK_PRIOR:
                    lastchar = KEY_PPAGE;
                    break;
                case VK_HOME:
                    lastchar = KEY_HOME;
                    break;
                case VK_END:
                    lastchar = KEY_END;
                    break;
                case VK_F1:
                    lastchar = KEY_F( 1 );
                    break;
                case VK_F2:
                    lastchar = KEY_F( 2 );
                    break;
                case VK_F3:
                    lastchar = KEY_F( 3 );
                    break;
                case VK_F4:
                    lastchar = KEY_F( 4 );
                    break;
                case VK_F5:
                    lastchar = KEY_F( 5 );
                    break;
                case VK_F6:
                    lastchar = KEY_F( 6 );
                    break;
                case VK_F7:
                    lastchar = KEY_F( 7 );
                    break;
                case VK_F8:
                    lastchar = KEY_F( 8 );
                    break;
                case VK_F9:
                    lastchar = KEY_F( 9 );
                    break;
                case VK_F10:
                    lastchar = KEY_F( 10 );
                    break;
                case VK_F11:
                    lastchar = KEY_F( 11 );
                    break;
                case VK_F12:
                    lastchar = KEY_F( 12 );
                    break;
                default:
                    break;
            }
            return 0;

        case WM_KEYUP:
            if( wParam == VK_SHIFT ) {
                shift_down = false;
                return 0;
            }


            if( !GetAsyncKeyState( VK_LMENU ) && alt_down ) {
                if( int code = end_alt_code() ) {
                    lastchar = code;
                }
            }
            return 0;

        case WM_SIZE:
            WINDOWPLACEMENT win_placement;
            win_placement.length = sizeof( win_placement );
            if( GetWindowPlacement( hWnd, &win_placement ) && win_placement.showCmd != SW_SHOWMINIMIZED ) {
                needs_resize = true;
            }
            return 0;

        case WM_SYSCHAR:
            add_alt_code( static_cast<char>( wParam ) );
            return 0;

        case WM_SYSKEYDOWN:

            if( GetAsyncKeyState( VK_LMENU ) && !alt_down ) {
                begin_alt_code();
            }
            break;

        case WM_SETCURSOR:
            MouseOver = LOWORD( lParam );
            if( get_option<std::string>( "HIDE_CURSOR" ) == "hide" ) {
                if( MouseOver == HTCLIENT && CursorVisible ) {
                    CursorVisible = false;
                    ShowCursor( false );
                } else if( MouseOver != HTCLIENT && !CursorVisible ) {
                    CursorVisible = true;
                    ShowCursor( true );
                }
            } else if( !CursorVisible ) {
                CursorVisible = true;
                ShowCursor( true );
            }
            break;

        case WM_ERASEBKGND:

            return 1;

        case WM_PAINT:
            BitBlt( WindowDC, 0, 0, WindowWidth, WindowHeight, backbuffer, 0, 0, SRCCOPY );
            ValidateRect( WindowHandle, nullptr );
            return 0;

        case WM_DESTROY:

            exit( 0 );
    }

    return DefWindowProcW( hWnd, Msg, wParam, lParam );
}


inline void VertLineDIB( int x, int y, int y2, int thickness, unsigned char color )
{
    int j;
    for( j = y; j < y2; j++ ) {
        memset( &dcbits[x + j * WindowWidth], color, thickness );
    }
}
inline void HorzLineDIB( int x, int y, int x2, int thickness, unsigned char color )
{
    int j;
    for( j = y; j < y + thickness; j++ ) {
        memset( &dcbits[x + j * WindowWidth], color, x2 - x );
    }
}
inline void FillRectDIB( int x, int y, int width, int height, unsigned char color )
{
    int j;
    for( j = y; j < y + height; j++ )

    {
        memset( &dcbits[x + j * WindowWidth], color, width );
    }
}

void cata_cursesport::curses_drawwindow( const catacurses::window &w )
{

    WINDOW *const win = w.get<WINDOW>();
    int i = 0;
    int j = 0;
    int drawx = 0;
    int drawy = 0;
    wchar_t tmp;

    for( j = 0; j < win->height; j++ ) {
        if( win->line[j].touched ) {
            win->line[j].touched = false;

            for( i = 0; i < win->width; i++ ) {
                const cursecell &cell = win->line[j].chars[i];
                if( cell.ch.empty() ) {

                    continue;
                }
                drawx = ( ( win->pos.x + i ) * fontwidth );
                drawy = ( ( win->pos.y + j ) * fontheight );
                if( drawx + fontwidth > WindowWidth || drawy + fontheight > WindowHeight ) {

                    continue;
                }

                int FG = cell.FG;
                int BG = cell.BG;
                FillRectDIB( drawx, drawy, fontwidth, fontheight, BG );
                static const std::string space_string = " ";

                if( cell.ch == space_string ) {
                    continue;
                }

                tmp = UTF8_getch( cell.ch );
                if( tmp != UNKNOWN_UNICODE ) {

                    int color = RGB( windowsPalette[FG].rgbRed, windowsPalette[FG].rgbGreen,
                                     windowsPalette[FG].rgbBlue );
                    SetTextColor( backbuffer, color );

                    int cw = mk_wcwidth( tmp );
                    if( cw > 1 ) {
                        FillRectDIB( drawx + fontwidth * ( cw - 1 ), drawy, fontwidth, fontheight, BG );
                        i += cw - 1;
                    }
                    if( tmp ) {
                        const std::wstring utf16 = widen( cell.ch );
                        ExtTextOutW( backbuffer, drawx, drawy, 0, nullptr, utf16.c_str(), utf16.length(), nullptr );
                    }
                } else {
                    switch( static_cast<unsigned char>( win->line[j].chars[i].ch[0] ) ) {

                        case LINE_OXOX_C:
                            HorzLineDIB( drawx, drawy + halfheight, drawx + fontwidth, 1, FG );
                            break;

                        case LINE_XOXO_C:
                            VertLineDIB( drawx + halfwidth, drawy, drawy + fontheight, 2, FG );
                            break;

                        case LINE_OXXO_C:
                            HorzLineDIB( drawx + halfwidth, drawy + halfheight, drawx + fontwidth, 1, FG );
                            VertLineDIB( drawx + halfwidth, drawy + halfheight, drawy + fontheight, 2, FG );
                            break;

                        case LINE_OOXX_C:
                            HorzLineDIB( drawx, drawy + halfheight, drawx + halfwidth, 1, FG );
                            VertLineDIB( drawx + halfwidth, drawy + halfheight, drawy + fontheight, 2, FG );
                            break;

                        case LINE_XOOX_C:
                            HorzLineDIB( drawx, drawy + halfheight, drawx + halfwidth, 1, FG );
                            VertLineDIB( drawx + halfwidth, drawy, drawy + halfheight + 1, 2, FG );
                            break;

                        case LINE_XXOO_C:
                            HorzLineDIB( drawx + halfwidth, drawy + halfheight, drawx + fontwidth, 1, FG );
                            VertLineDIB( drawx + halfwidth, drawy, drawy + halfheight + 1, 2, FG );
                            break;

                        case LINE_XXOX_C:
                            HorzLineDIB( drawx, drawy + halfheight, drawx + fontwidth, 1, FG );
                            VertLineDIB( drawx + halfwidth, drawy, drawy + halfheight, 2, FG );
                            break;

                        case LINE_XXXO_C:
                            VertLineDIB( drawx + halfwidth, drawy, drawy + fontheight, 2, FG );
                            HorzLineDIB( drawx + halfwidth, drawy + halfheight, drawx + fontwidth, 1, FG );
                            break;

                        case LINE_OXXX_C:
                            HorzLineDIB( drawx, drawy + halfheight, drawx + fontwidth, 1, FG );
                            VertLineDIB( drawx + halfwidth, drawy + halfheight, drawy + fontheight, 2, FG );
                            break;

                        case LINE_XXXX_C:
                            HorzLineDIB( drawx, drawy + halfheight, drawx + fontwidth, 1, FG );
                            VertLineDIB( drawx + halfwidth, drawy, drawy + fontheight, 2, FG );
                            break;

                        case LINE_XOXX_C:
                            VertLineDIB( drawx + halfwidth, drawy, drawy + fontheight, 2, FG );
                            HorzLineDIB( drawx, drawy + halfheight, drawx + halfwidth, 1, FG );
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }

    win->draw = false;
}


static void CheckMessages()
{
    MSG msg;
    while( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) ) {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
    if( needs_resize ) {
        restore_on_out_of_scope prev_lastchar( lastchar );
        handle_resize( 0, 0 );
        refresh_display();
    }
}


int projected_window_width()
{
    return get_option<int>( "TERMINAL_X" ) * fontwidth;
}


int projected_window_height()
{
    return get_option<int>( "TERMINAL_Y" ) * fontheight;
}

int get_terminal_width()
{
    return TERMINAL_WIDTH;
}

int get_terminal_height()
{
    return TERMINAL_HEIGHT;
}






void catacurses::init_interface()
{
    lastchar = -1;
    inputdelay = -1;

    font_loader fl;
    fl.load();
    ::fontwidth = fl.fontwidth;
    ::fontheight = fl.fontheight;
    halfwidth = fontwidth / 2;
    halfheight = fontheight / 2;
    TERMINAL_WIDTH = get_option<int>( "TERMINAL_X" );
    TERMINAL_HEIGHT = get_option<int>( "TERMINAL_Y" );
    WindowWidth = TERMINAL_WIDTH * fontwidth;
    WindowHeight = TERMINAL_HEIGHT * fontheight;


    WinCreate();

    timeBeginPeriod( 1 );

    CheckMessages();

    create_backbuffer();


    if( SetCurrentDirectoryW( L"data\\font" ) ) {
        WIN32_FIND_DATAW findData;
        for( HANDLE findFont = FindFirstFileW( L".\\*", &findData ); findFont != INVALID_HANDLE_VALUE; ) {

            if( !( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) {
                AddFontResourceExW( findData.cFileName, FR_PRIVATE, nullptr );
            }
            if( !FindNextFileW( findFont, &findData ) ) {
                FindClose( findFont );
                break;
            }
        }
        SetCurrentDirectoryW( L"..\\.." );
    }


    cata_assert( !fl.typeface.empty() );
    font = CreateFontW( fontheight, fontwidth, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        PROOF_QUALITY, FF_MODERN, widen( fl.typeface.front() ).c_str() );


    SetBkMode( backbuffer, TRANSPARENT );

    SelectObject( backbuffer, font );

    color_loader<RGBQUAD>().load( windowsPalette );
    if( SetDIBColorTable( backbuffer, 0, windowsPalette.size(), windowsPalette.data() ) == 0 ) {
        throw std::runtime_error( "SetDIBColorTable failed" );
    }
    init_colors();

    stdscr = newwin( get_option<int>( "TERMINAL_Y" ), get_option<int>( "TERMINAL_X" ), point::zero );


    initialized = true;

#if defined(SDL_SOUND)
    initSDLAudioOnly();
    init_sound();
    if( sound_init_success ) {
        load_soundset();
    }
#endif
}

bool catacurses::supports_256_colors()
{
    return COLORS >= 256;
}


static uint64_t GetPerfCount()
{
    uint64_t Count;
    QueryPerformanceCounter( reinterpret_cast<PLARGE_INTEGER>( &Count ) );
    return Count;
}

void input_manager::pump_events()
{
    if( test_mode ) {
        return;
    }


    CheckMessages();

    lastchar = ERR;
    previously_pressed_key = 0;
}



input_event input_manager::get_input_event( const keyboard_mode )
{
    if( test_mode ) {

        throw std::runtime_error( "input_manager::get_input_event called in test mode" );
    }




    uint64_t Frequency;
    QueryPerformanceFrequency( reinterpret_cast<PLARGE_INTEGER>( &Frequency ) );
    wrefresh( catacurses::stdscr );
    InvalidateRect( WindowHandle, nullptr, true );
    lastchar = ERR;
    if( inputdelay < 0 ) {
        for( ; lastchar == ERR; Sleep( 1 ) ) {
            CheckMessages();
        }
    } else if( inputdelay > 0 ) {
        for( uint64_t t0 = GetPerfCount(), t1 = 0; t1 < ( t0 + inputdelay * Frequency / 1000 );
             t1 = GetPerfCount() ) {
            CheckMessages();
            if( lastchar != ERR ) {
                break;
            }
            Sleep( 1 );
        }
    } else {
        CheckMessages();
    }

    if( lastchar != ERR && get_option<std::string>( "HIDE_CURSOR" ) == "hidekb" && CursorVisible ) {
        CursorVisible = false;
        ShowCursor( false );
    }

    previously_pressed_key = 0;
    input_event rval;
    if( lastchar == ERR ) {
        if( input_timeout > 0 ) {
            rval.type = input_event_t::timeout;
        } else {
            rval.type = input_event_t::error;
        }
    } else {

        if( lastchar == 127 ) {
            previously_pressed_key = KEY_BACKSPACE;
            return input_event( KEY_BACKSPACE, input_event_t::keyboard_char );
        }
        rval.type = input_event_t::keyboard_char;
        rval.text = utf32_to_utf8( lastchar );
        previously_pressed_key = lastchar;


        rval.add_input( lastchar );
    }

    return rval;
}

bool gamepad_available()
{
    return false;
}

std::optional<tripoint_bub_ms> input_context::get_coordinates( const catacurses::window &,
        const point &, bool center_cursor ) const
{

    return std::nullopt;
}


void catacurses::endwin()
{
    ui_manager::reset();
    DeleteObject( font );
    WinDestroy();

    RemoveFontResourceExA( "data\\font", FR_PRIVATE, nullptr );
}

template<>
RGBQUAD color_loader<RGBQUAD>::from_rgb( const int r, const int g, const int b )
{
    RGBQUAD result;

    result.rgbBlue = b;

    result.rgbGreen = g;

    result.rgbRed = r;

    result.rgbReserved = 0;
    return result;
}

void input_manager::set_timeout( const int t )
{
    input_timeout = t;
    inputdelay = t;
}

int get_scaling_factor()
{
    return 1;
}

HWND getWindowHandle()
{
    return WindowHandle;
}

void refresh_display()
{
#if defined(SDL_SOUND)
    sound_backend::poll();
#endif
    RedrawWindow( WindowHandle, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW );
}

void drain_renderer_recovery()
{
}

void refresh_mouse_config()
{
}

void set_title( const std::string &title )
{
    if( WindowHandle != nullptr ) {
        SetWindowTextW( WindowHandle, widen( title ).c_str() );
    }
}
#endif
#endif
