#pragma once
#ifndef CATA_SRC_SDL_WRAPPERS_H
#define CATA_SRC_SDL_WRAPPERS_H

// IWYU pragma: begin_exports
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
// IWYU pragma: end_exports

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct point;


using CataFlipMode = SDL_FlipMode;

inline constexpr SDL_Keymod KMOD_CTRL  = SDL_KMOD_CTRL;
inline constexpr SDL_Keymod KMOD_SHIFT = SDL_KMOD_SHIFT;
inline constexpr SDL_Keymod KMOD_ALT   = SDL_KMOD_ALT;

struct SDL_Renderer_deleter {
    void operator()( SDL_Renderer *const renderer ) {
        SDL_DestroyRenderer( renderer );
    }
};
using SDL_Renderer_Ptr = std::unique_ptr<SDL_Renderer, SDL_Renderer_deleter>;

struct SDL_Window_deleter {
    void operator()( SDL_Window *const window ) {
        SDL_DestroyWindow( window );
    }
};
using SDL_Window_Ptr = std::unique_ptr<SDL_Window, SDL_Window_deleter>;

struct SDL_Texture_deleter {
    void operator()( SDL_Texture *const ptr ) {
        SDL_DestroyTexture( ptr );
    }
};
using SDL_Texture_Ptr = std::unique_ptr<SDL_Texture, SDL_Texture_deleter>;

struct SDL_Surface_deleter {
    void operator()( SDL_Surface *const ptr ) {
        SDL_DestroySurface( ptr );
    }
};
using SDL_Surface_Ptr = std::unique_ptr<SDL_Surface, SDL_Surface_deleter>;

struct TTF_Font_deleter {
    void operator()( TTF_Font *const font ) {
        TTF_CloseFont( font );
    }
};
using TTF_Font_Ptr = std::unique_ptr<TTF_Font, TTF_Font_deleter>;





bool printErrorIf( bool condition, const char *message );




void throwErrorIf( bool condition, const char *message );










void RenderCopy( const SDL_Renderer_Ptr &renderer, const SDL_Texture_Ptr &texture,
                 const SDL_Rect *srcrect, const SDL_Rect *dstrect );
SDL_Texture_Ptr CreateTexture( const SDL_Renderer_Ptr &renderer, Uint32 format, int access,
                               int w, int h );
SDL_Texture_Ptr CreateTextureFromSurface( const SDL_Renderer_Ptr &renderer,
        const SDL_Surface_Ptr &surface );
void SetRenderDrawColor( const SDL_Renderer_Ptr &renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a );
void RenderDrawPoint( const SDL_Renderer_Ptr &renderer, const point &p );
void RenderFillRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect );
int FillRect( const SDL_Surface_Ptr &surface, const SDL_Rect *rect, Uint32 color );
void SetTextureBlendMode( const SDL_Texture_Ptr &texture, SDL_BlendMode blendMode );
void SetTextureBlendMode( const std::shared_ptr<SDL_Texture> &texture, SDL_BlendMode blendMode );
bool SetTextureColorMod( const SDL_Texture_Ptr &texture, Uint32 r, Uint32 g, Uint32 b );
bool SetTextureColorMod( const std::shared_ptr<SDL_Texture> &texture, Uint32 r, Uint32 g,
                         Uint32 b );


bool UpdateTexture( const SDL_Texture_Ptr &texture, const SDL_Rect *rect, const void *pixels,
                    int pitch );
void SetRenderDrawBlendMode( const SDL_Renderer_Ptr &renderer, SDL_BlendMode blendMode );
void GetRenderDrawBlendMode( const SDL_Renderer_Ptr &renderer, SDL_BlendMode &blend_mode );
SDL_Surface_Ptr load_image( const char *path );







class gpu_handle_graveyard
{
    public:
        using gate = std::shared_ptr<std::atomic<bool>>;

        gpu_handle_graveyard();
        ~gpu_handle_graveyard();
        gpu_handle_graveyard( const gpu_handle_graveyard & ) = delete;
        gpu_handle_graveyard &operator=( const gpu_handle_graveyard & ) = delete;
        gpu_handle_graveyard( gpu_handle_graveyard && ) = default;
        gpu_handle_graveyard &operator=( gpu_handle_graveyard && ) = default;



        void add( SDL_Texture *raw );

        void adopt( std::shared_ptr<SDL_Texture> handle );
        bool empty() const {
            return handles_.empty();
        }


        const gate &current_gate() const {
            return gate_;
        }


        void drain_live_renderer();


        void abandon();

    private:
        std::vector<std::shared_ptr<SDL_Texture>> handles_;
        gate gate_;
};




std::shared_ptr<SDL_Texture> make_gated_texture( SDL_Texture *raw, gpu_handle_graveyard::gate g );

namespace cata_shader
{
class variant_pass;
}







class scoped_render_target
{
    public:
        scoped_render_target( const SDL_Renderer_Ptr &renderer, SDL_Texture *target,
                              cata_shader::variant_pass *vp = nullptr );
        ~scoped_render_target();

        scoped_render_target( const scoped_render_target & ) = delete;
        scoped_render_target &operator=( const scoped_render_target & ) = delete;
        scoped_render_target( scoped_render_target && ) = delete;
        scoped_render_target &operator=( scoped_render_target && ) = delete;

        bool is_valid() const {
            return valid_;
        }




        bool boundary_intact() const {
            return boundary_intact_;
        }




        bool restore();

    private:


        void mark_boundary_lost();

        SDL_Renderer *renderer_ = nullptr;
        SDL_Texture *prior_target_ = nullptr;
        cata_shader::variant_pass *vp_ = nullptr;
        bool valid_ = false;
        bool restored_ = false;
        bool restore_attempted_ = false;
        bool last_restore_ok_ = false;
        bool boundary_intact_ = true;
};










enum class bind_result {
    ok,
    refused_pre_switch,
    failed_in_switch,
};



bool renderer_boundary_recovery_pending();

void renderer_boundary_signal_recovery_required();




bind_result permanent_render_target_bind( const SDL_Renderer_Ptr &renderer, SDL_Texture *target,
        cata_shader::variant_pass *vp = nullptr );
void RenderClear( const SDL_Renderer_Ptr &renderer );
SDL_Surface_Ptr CreateRGBSurface( Uint32 flags, int width, int height, int depth, Uint32 Rmask,
                                  Uint32 Gmask, Uint32 Bmask, Uint32 Amask );
void SetTextureAlphaMod( const SDL_Texture_Ptr &texture, Uint8 alpha );
void SetTextureAlphaMod( const std::shared_ptr<SDL_Texture> &texture, Uint8 alpha );
void RenderCopyEx( const SDL_Renderer_Ptr &renderer, SDL_Texture *texture,
                   const SDL_Rect *srcrect, const SDL_Rect *dstrect,
                   double angle, const SDL_Point *center, CataFlipMode flip );
void RenderSetClipRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect );




void RenderGeometry( const SDL_Renderer_Ptr &renderer, SDL_Texture *texture,
                     const SDL_Vertex *vertices, int num_vertices,
                     const int *indices, int num_indices );


bool UpdateTexture( const SDL_Texture_Ptr &texture, const SDL_Rect &rect,
                    const SDL_Surface_Ptr &surface );
void RenderGetClipRect( const SDL_Renderer_Ptr &renderer, SDL_Rect *rect );

bool GetRectIntersection( const SDL_Rect &a, const SDL_Rect &b, SDL_Rect &result );
bool RenderIsClipEnabled( const SDL_Renderer_Ptr &renderer );
int BlitSurface( const SDL_Surface_Ptr &src, const SDL_Rect *srcrect,
                 const SDL_Surface_Ptr &dst, SDL_Rect *dstrect );
Uint32 MapRGB( const SDL_Surface_Ptr &surface, Uint8 r, Uint8 g, Uint8 b );
Uint32 MapRGBA( const SDL_Surface_Ptr &surface, Uint8 r, Uint8 g, Uint8 b, Uint8 a );
void GetRGBA( Uint32 pixel, const SDL_Surface_Ptr &surface, Uint8 &r, Uint8 &g, Uint8 &b,
              Uint8 &a );
int SetColorKey( const SDL_Surface_Ptr &surface, int flag, Uint32 key );
int SetSurfaceRLE( const SDL_Surface_Ptr &surface, int flag );
int SetSurfaceBlendMode( const SDL_Surface_Ptr &surface, SDL_BlendMode blendMode );
SDL_Surface_Ptr ConvertSurfaceFormat( const SDL_Surface_Ptr &surface, Uint32 pixel_format );
int LockSurface( const SDL_Surface_Ptr &surface );
void UnlockSurface( const SDL_Surface_Ptr &surface );

Uint32 GetSurfacePixelFormat( const SDL_Surface_Ptr &surface );
TTF_Font_Ptr OpenFontIndex( const char *file, int ptsize, int64_t index );
const char *FontFaceStyleName( const TTF_Font_Ptr &font );
int FontFaces( const TTF_Font_Ptr &font );
int FontHeight( const TTF_Font_Ptr &font );
void SetFontStyle( const TTF_Font_Ptr &font, int style );
SDL_Surface_Ptr RenderUTF8_Solid( const TTF_Font_Ptr &font, const char *text, SDL_Color fg );
SDL_Surface_Ptr RenderUTF8_Blended( const TTF_Font_Ptr &font, const char *text, SDL_Color fg );



bool CanRenderGlyph( const TTF_Font_Ptr &font, Uint32 ch );



int GetNumVideoDisplays();
const char *GetDisplayName( int displayIndex );
bool GetDesktopDisplayMode( int displayIndex, SDL_DisplayMode *mode );


int GetNumRenderDrivers();
const char *GetRenderDriverName( int index );

int GetNumGPUDrivers();
const char *GetGPUDriverName( int index );


const char *GetRendererName( const SDL_Renderer_Ptr &renderer );


const char *GetGPUBackendName( const SDL_Renderer_Ptr &renderer );
bool IsRendererSoftware( const SDL_Renderer_Ptr &renderer );


bool WaitForEvent( int timeout_ms );

void PushWakeEvent();

bool HasEvent( Uint32 type );

inline constexpr Uint32 CATA_WAKE_EVENT = SDL_EVENT_USER + 2;
bool GetRendererMaxTextureSize( const SDL_Renderer_Ptr &renderer, int *max_w, int *max_h );

void RenderPresent( const SDL_Renderer_Ptr &renderer );
void RenderDrawRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect );
void RenderGetViewport( const SDL_Renderer_Ptr &renderer, SDL_Rect *rect );



void RenderSetLogicalSize( const SDL_Renderer_Ptr &renderer, int w, int h );
void RenderSetScale( const SDL_Renderer_Ptr &renderer, float scaleX, float scaleY );


bool RenderReadPixels( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect,
                       Uint32 format, void *pixels, int pitch );
void GetRendererOutputSize( const SDL_Renderer_Ptr &renderer, int *w, int *h );


SDL_Texture *GetRenderTarget( const SDL_Renderer_Ptr &renderer );


uint32_t GetTicks();

bool IsCursorVisible();
void ShowCursor();
void HideCursor();


std::string GetClipboardText();
bool SetClipboardText( const std::string &text );




Uint32 GetMouseState( int *x, int *y );



bool IsScancodePressed( SDL_Scancode scancode );


void GetWindowSize( SDL_Window *window, int *w, int *h );
void GetWindowSizeInPixels( SDL_Window *window, int *w, int *h );



void SetTextureScaleQuality( const SDL_Texture_Ptr &texture, const std::string &quality );

void SetDefaultTextureScaleQuality( const std::string &quality );

SDL_ScaleMode GetTextureScaleMode( const std::shared_ptr<SDL_Texture> &texture );


void StartTextInput( SDL_Window *window );
void StopTextInput( SDL_Window *window );
bool IsTextInputActive( SDL_Window *window );



inline constexpr Uint32 CATA_WINDOW_HIDDEN    = SDL_WINDOW_HIDDEN;
inline constexpr Uint32 CATA_WINDOW_RESIZABLE = SDL_WINDOW_RESIZABLE;
inline constexpr Uint32 CATA_WINDOW_MAXIMIZED = SDL_WINDOW_MAXIMIZED;
inline constexpr Uint32 CATA_WINDOW_HIGH_DPI  = SDL_WINDOW_HIGH_PIXEL_DENSITY;




SDL_Window_Ptr CreateGameWindow( const char *title, int display, int w, int h, Uint32 flags );

enum class FullscreenMode { windowed, fullscreen_desktop, fullscreen_exclusive };


bool SetWindowFullscreen( SDL_Window *window, FullscreenMode mode );


void RestoreWindow( SDL_Window *window );
void SetWindowSize( SDL_Window *window, int w, int h );
void SetWindowMinimumSize( SDL_Window *window, int w, int h );
void SetWindowTitle( SDL_Window *window, const char *title );



SDL_Renderer_Ptr CreateRenderer( const SDL_Window_Ptr &window, const char *driver_name,
                                 bool software, bool vsync );




float GetFingerX( const SDL_Event &ev, int windowWidth );
float GetFingerY( const SDL_Event &ev, int windowHeight );






bool IsWindowEvent( const SDL_Event &ev );

Uint32 GetWindowEventID( const SDL_Event &ev );


inline constexpr Uint32 CATA_WINDOWEVENT_SHOWN        = SDL_EVENT_WINDOW_SHOWN;
inline constexpr Uint32 CATA_WINDOWEVENT_EXPOSED      = SDL_EVENT_WINDOW_EXPOSED;
inline constexpr Uint32 CATA_WINDOWEVENT_MINIMIZED    = SDL_EVENT_WINDOW_MINIMIZED;
inline constexpr Uint32 CATA_WINDOWEVENT_RESTORED     = SDL_EVENT_WINDOW_RESTORED;
inline constexpr Uint32 CATA_WINDOWEVENT_RESIZED      = SDL_EVENT_WINDOW_RESIZED;
inline constexpr Uint32 CATA_WINDOWEVENT_FOCUS_LOST   = SDL_EVENT_WINDOW_FOCUS_LOST;
inline constexpr Uint32 CATA_WINDOWEVENT_FOCUS_GAINED = SDL_EVENT_WINDOW_FOCUS_GAINED;
inline constexpr Uint32 CATA_WINDOWEVENT_SAFE_AREA_CHANGED = SDL_EVENT_WINDOW_SAFE_AREA_CHANGED;

inline constexpr Uint32 CATA_RENDER_TARGETS_RESET = SDL_EVENT_RENDER_TARGETS_RESET;


inline constexpr Uint32 CATA_RENDER_DEVICE_RESET = SDL_EVENT_RENDER_DEVICE_RESET;
inline constexpr Uint32 CATA_RENDER_DEVICE_LOST = SDL_EVENT_RENDER_DEVICE_LOST;
inline constexpr Uint32 CATA_APP_DIDENTERFOREGROUND = SDL_EVENT_DID_ENTER_FOREGROUND;
inline constexpr Uint32 CATA_APP_WILLENTERBACKGROUND = SDL_EVENT_WILL_ENTER_BACKGROUND;
inline constexpr Uint32 CATA_APP_DIDENTERBACKGROUND = SDL_EVENT_DID_ENTER_BACKGROUND;
inline constexpr Uint32 CATA_WINDOWEVENT_PIXEL_SIZE_CHANGED = SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED;


inline SDL_FingerID GetFingerID( const SDL_Event &ev )
{
    return ev.tfinger.fingerID;
}

inline constexpr Uint32 CATA_FINGERMOTION = SDL_EVENT_FINGER_MOTION;
inline constexpr Uint32 CATA_FINGERDOWN   = SDL_EVENT_FINGER_DOWN;
inline constexpr Uint32 CATA_FINGERUP     = SDL_EVENT_FINGER_UP;
inline constexpr Uint32 CATA_FINGERCANCELED = SDL_EVENT_FINGER_CANCELED;

inline constexpr Uint32 CATA_KEYDOWN         = SDL_EVENT_KEY_DOWN;
inline constexpr Uint32 CATA_KEYUP           = SDL_EVENT_KEY_UP;
inline constexpr Uint32 CATA_TEXTINPUT       = SDL_EVENT_TEXT_INPUT;
inline constexpr Uint32 CATA_TEXTEDITING     = SDL_EVENT_TEXT_EDITING;
inline constexpr Uint32 CATA_MOUSEMOTION     = SDL_EVENT_MOUSE_MOTION;
inline constexpr Uint32 CATA_MOUSEBUTTONDOWN = SDL_EVENT_MOUSE_BUTTON_DOWN;
inline constexpr Uint32 CATA_MOUSEBUTTONUP   = SDL_EVENT_MOUSE_BUTTON_UP;
inline constexpr Uint32 CATA_MOUSEWHEEL      = SDL_EVENT_MOUSE_WHEEL;
inline constexpr Uint32 CATA_QUIT            = SDL_EVENT_QUIT;



struct CataKeysym {
    SDL_Keycode sym;
    Uint16 mod;
    SDL_Scancode scancode;
};
CataKeysym GetKeysym( const SDL_Event &ev );






inline bool operator==( const SDL_Color &lhs, const SDL_Color &rhs )
{
    return
        lhs.r == rhs.r &&
        lhs.g == rhs.g &&
        lhs.b == rhs.b &&
        lhs.a == rhs.a;
}

inline bool operator!=( const SDL_Color &lhs, const SDL_Color &rhs )
{
    return !operator==( lhs, rhs );
}

inline bool operator==( const SDL_Rect &lhs, const SDL_Rect &rhs )
{
    return
        lhs.x == rhs.x &&
        lhs.y == rhs.y &&
        lhs.w == rhs.w &&
        lhs.h == rhs.h;
}

inline bool operator!=( const SDL_Rect &lhs, const SDL_Rect &rhs )
{
    return !operator==( lhs, rhs );
}





#if defined(__ANDROID__)
inline void *GetAndroidJNIEnv()
{
    return SDL_GetAndroidJNIEnv();
}

inline void *GetAndroidActivity()
{
    return SDL_GetAndroidActivity();
}

inline const char *GetAndroidExternalStoragePath()
{
    return SDL_GetAndroidExternalStoragePath();
}
#endif

#endif
