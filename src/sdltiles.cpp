#include "cursesdef.h" // IWYU pragma: associated
#include "sdltiles.h" // IWYU pragma: associated
#include "a11y_runtime.h"
#include "a11y_android.h"

#include "cata_shader.h"
#include "cuboid_rectangle.h"
#include "point.h"

#if defined(TILES)

#include <algorithm>
#include <array>
#include <atomic>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stack>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "atlas_bake_plan.h"
#include "avatar.h"
#include "cached_options.h"
#include "cata_assert.h"
#include "cata_scope_helpers.h"
#include "cata_tiles.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "color_loader.h"
#include "cursesport.h"
#include "debug.h"
#include "filesystem.h"
#include "flag.h"
#include "font_loader.h"
#include "game.h"
#include "game_constants.h"
#include "game_ui.h"
#include "hash_utils.h"
#include "horde_entity.h"
#include "input.h"
#include "input_context.h"
#include "input_wait.h"
#include "json.h"
#include "line.h"
#include "loading_ui.h"
#include "map.h"
#include "map_extras.h"
#include "mapbuffer.h"
#include "mission.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "path_info.h"
#include "sdl_geometry.h"
#include "sdl_renderer_recovery.h"
#include "sdl_wrappers.h"
#include "sdl_font.h"
#include "sdl_quad_batch.h"
#include "tileset_loader.h"
#include "sdl_gamepad.h"
#if defined(SDL_SOUND)
#include "sound_backend.h"
#endif
#include "sdlsound.h"
#include "string_formatter.h"
#include "terminal_layout.h" // IWYU pragma: keep
#include "touch_joystick.h" // IWYU pragma: keep
#include "uistate.h"
#include "ui_manager.h"
#include "wcwidth.h"
#include "cata_imgui.h"

std::unique_ptr<cataimgui::client> imclient;

#if defined(_WIN32)
#   if 1 // HACK: Hack to prevent reordering of #include "platform_win.h" by IWYU
#       include "platform_win.h"
#   endif
#   include <shlwapi.h>
#endif

#if defined(__ANDROID__)
#include <jni.h>

#include "action.h"
#include "inventory.h"
#include "map.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "worldfactory.h"
#endif

#if defined(EMSCRIPTEN)
#include <emscripten.h>
#endif

#define dbg(x) DebugLog((x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "

static const oter_type_str_id oter_type_forest_trail( "forest_trail" );

static const trait_id trait_DEBUG_CLAIRVOYANCE( "DEBUG_CLAIRVOYANCE" );
static const trait_id trait_DEBUG_NIGHTVISION( "DEBUG_NIGHTVISION" );





static tileset_cache ts_cache;
std::shared_ptr<cata_tiles> tilecontext;
std::shared_ptr<cata_tiles> closetilecontext;
std::shared_ptr<cata_tiles> fartilecontext;
std::unique_ptr<cata_tiles> overmap_tilecontext;
std::shared_ptr<cata_tiles> portrait_tilecontext;
static uint32_t lastupdate = 0;
static uint32_t interval = 25;
static bool needupdate = false;
palette_array windowsPalette;

static Font_Ptr font;
static Font_Ptr gui_font;
static Font_Ptr map_font;
static Font_Ptr overmap_font;

static Font_Ptr fixture_font;
static bool test_fixture_acquired_ttf = false;

static bool test_fixture_fonts_saved = false;
static Font_Ptr test_fixture_saved_font;
static Font_Ptr test_fixture_saved_gui_font;
static bool test_fixture_saved_glyphs_changed = false;

static SDL_Window_Ptr window;
static SDL_Renderer_Ptr renderer;
static Uint32 pixel_format = SDL_PIXELFORMAT_UNKNOWN;
static SDL_Texture_Ptr display_buffer;
static GeometryRenderer_Ptr geometry;

static text_batch curses_text_batch;
static std::unique_ptr<cata_shader::variant_pass> shared_variant_pass;


static std::unique_ptr<smooth_lightmap> shared_lightmap;
static void reset_shared_lightmap();
static int WindowWidth;
static int WindowHeight;


static int DrawableWidth;
static int DrawableHeight;



input_event last_input;

static int inputdelay;
int fontwidth;
int fontheight;
static int TERMINAL_WIDTH;
static int TERMINAL_HEIGHT;
static bool fullscreen;
static int scaling_factor;

using cata_cursesport::cursecell;





static void ClearScreen()
{
    SetRenderDrawColor( renderer, 0, 0, 0, 255 );
    RenderClear( renderer );
}

bool clear_sdl_window()
{
    display_buffer_draw_scope draw_scope;
    if( !draw_scope.should_draw() ) {


        return false;
    }
    ClearScreen();
    return true;
}

static void InitSDL()
{
    int init_flags = SDL_INIT_VIDEO;
#if defined(SDL_SOUND)
    init_flags |= SDL_INIT_AUDIO;
#endif
#if defined(SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING)


    SDL_SetHint( SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING, "1" );
#endif

#if defined(_WIN32) && defined(SDL_HINT_IME_SHOW_UI)


    SDL_SetHint( SDL_HINT_IME_SHOW_UI, "1" );
#endif

#if defined(SDL_HINT_IME_SUPPORT_EXTENDED_TEXT)

    SDL_SetHint( SDL_HINT_IME_SUPPORT_EXTENDED_TEXT, "1" );
#endif

#if defined(SDL_HINT_WINDOWS_DPI_AWARENESS)
    SDL_SetHint( SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2" );
#endif

#if defined(SDL_HINT_APP_NAME)



    SDL_SetHint( SDL_HINT_APP_NAME, _( "Cataclysm: Dark Days Ahead" ) );
#endif

    throwErrorIf( !SDL_Init( init_flags ), "SDL_Init failed" );
    throwErrorIf( !TTF_Init(), "TTF_Init failed" );





    if( atexit( SDL_Quit ) ) {
        debugmsg( "atexit failed to register SDL_Quit" );
    }
}

namespace
{



gpu_handle_graveyard &setup_target_quarantine()
{
    static gpu_handle_graveyard q;
    return q;
}
}



static point compute_display_buffer_dims()
{


    return point{ TERMINAL_WIDTH * fontwidth, TERMINAL_HEIGHT * fontheight };
}



static point compute_drawable_dims()
{
    int pw = 0;
    int ph = 0;
    GetWindowSizeInPixels( ::window.get(), &pw, &ph );
    return point{ pw, ph };
}

#if !defined(__ANDROID__)



static SDL_Rect get_display_buffer_render_rect()
{
    const point b = compute_display_buffer_dims();
    const point d = compute_drawable_dims();
    if( b.x <= 0 || b.y <= 0 ) {
        return SDL_Rect{ 0, 0, 0, 0 };
    }
    const int scale = std::max( 1, std::min( d.x / b.x, d.y / b.y ) );
    return SDL_Rect{ 0, 0, b.x * scale, b.y * scale };
}
#endif




static int test_drawable_override_w = -1;
static int test_drawable_override_h = -1;


static void refresh_drawable_dims()
{
    if( test_drawable_override_w >= 0 && test_drawable_override_h >= 0 ) {
        DrawableWidth = test_drawable_override_w;
        DrawableHeight = test_drawable_override_h;
        return;
    }
    const point drawable = compute_drawable_dims();
    DrawableWidth = drawable.x;
    DrawableHeight = drawable.y;
}

static bool apply_resize_layout( int w, int h );


static bool terminal_glyphs_changed = false;
static bool rebuild_terminal_fonts( const point &cell, int point_size,
                                    const std::vector<font_config> &typefaces,
                                    const std::vector<font_config> &gui_typefaces, bool blending );
#if defined(__ANDROID__)
static SDL_Rect android_layout_bounds();
static float quick_shortcut_height();


static font_loader android_fonts;

static int android_buffer_scale = 1;

struct android_text_layout {
    point cell;
    point grid;
    int scale = 1;
};
static android_text_layout android_desired_layout();
#endif

static bool SetupRenderTarget()
{
    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );


    const point buffer_dims = compute_display_buffer_dims();
    SDL_Texture_Ptr staged = CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888,
                                            SDL_TEXTUREACCESS_TARGET, buffer_dims.x,
                                            buffer_dims.y );
    if( printErrorIf( !staged, "Failed to create window buffer" ) ) {
        return false;
    }
    cata_shader::variant_pass *vp = get_shared_variant_pass();
    {
        const bind_result r = permanent_render_target_bind( renderer, staged.get(), vp );
        if( r == bind_result::failed_in_switch ) {


            setup_target_quarantine().add( staged.release() );
            return false;
        }
        if( r != bind_result::ok ) {

            return false;
        }
    }
    ClearScreen();
    {
        const bind_result r = permanent_render_target_bind( renderer, nullptr, vp );
        if( r == bind_result::failed_in_switch ) {


            setup_target_quarantine().add( staged.release() );
            return false;
        }
        if( r != bind_result::ok ) {

            setup_target_quarantine().add( staged.release() );
            return false;
        }
    }
    display_buffer = std::move( staged );
    return true;
}



void refresh_mouse_config()
{
    using cata::options::mouse;
    ImGuiIO &io = ImGui::GetIO();
    if( !mouse.enabled ) {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        if( IsCursorVisible() ) {
            HideCursor();
        }
        return;
    }

    io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    const std::string hide_mode = get_option<std::string>( "HIDE_CURSOR" );
    if( hide_mode == "show" ) {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
        if( !IsCursorVisible() ) {
            ShowCursor();
        }
    } else {


        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        if( hide_mode == "hide" && IsCursorVisible() ) {
            HideCursor();
        }
    }
}

std::optional<std::string> gpu_backend_hint( const std::string &option_value,
        const bool env_override )
{
    if( env_override || option_value.empty() || option_value == "auto" ) {
        return std::nullopt;
    }
    return option_value;
}

#if defined(_WIN32)




static bool only_spirv_shader_artifacts_present()
{
    namespace fs = std::filesystem;
    const fs::path shaders_dir = fs::path( PATH_INFO::datadir() ) / "shaders";
    std::error_code ec;
    if( !fs::is_directory( shaders_dir, ec ) ) {
        return false;
    }
    bool any_spv = false;
    for( const fs::directory_entry &entry :
         fs::directory_iterator( shaders_dir, ec ) ) {
        const std::string ext = entry.path().extension().string();
        if( ext == ".dxil" ) {
            return false;
        }
        if( ext == ".spv" ) {
            any_spv = true;
        }
    }
    return any_spv;
}
#endif




static SDL_Renderer_Ptr create_game_renderer( const std::string &renderer_name, bool software )
{
    if( software ) {
        return CreateRenderer( ::window, nullptr, true, false );
    }
    return CreateRenderer( ::window, renderer_name.c_str(), false, true );
}




static void detect_renderer_backend()
{
    direct3d_mode = false;
    const SDL_PropertiesID props = SDL_GetRendererProperties( renderer.get() );
    const char *actual_name = props != 0
                              ? SDL_GetStringProperty( props, SDL_PROP_RENDERER_NAME_STRING, "" )
                              : "";
    dbg( D_INFO ) << "SDL renderer in use: " << ( actual_name ? actual_name : "unknown" );
    if( actual_name && std::string( actual_name ).find( "direct3d" ) != std::string::npos ) {
        direct3d_mode = true;
    }
}




static void rebuild_geometry_strategy( bool software_renderer )
{
    DebugLog( D_INFO, DC_ALL ) << "USE_COLOR_MODULATED_TEXTURES is set to " <<
                               get_option<bool>( "USE_COLOR_MODULATED_TEXTURES" );
    ( void )software_renderer;



    geometry = std::make_unique<DefaultGeometryRenderer>();
}





static Uint32 renderer_watch_window_id = 0;

static bool SDLCALL renderer_event_watch( void *userdata, SDL_Event *event )
{
    renderer_resource_coordinator *coord =
        static_cast<renderer_resource_coordinator *>( userdata );
    switch( event->type ) {
        case CATA_RENDER_TARGETS_RESET:
            if( event->render.windowID == renderer_watch_window_id ) {
                coord->request_recovery( renderer_recovery_severity::targets_reset );
            }
            break;
        case CATA_RENDER_DEVICE_RESET:
            if( event->render.windowID == renderer_watch_window_id ) {
#if defined(__ANDROID__)



                coord->request_recovery( renderer_recovery_severity::device_lost );
#else
                coord->request_recovery( renderer_recovery_severity::device_reset );
#endif
            }
            break;
        case CATA_RENDER_DEVICE_LOST:
            if( event->render.windowID == renderer_watch_window_id ) {
                coord->request_recovery( renderer_recovery_severity::device_lost );
            }
            break;
#if defined(__ANDROID__)
        case CATA_APP_DIDENTERFOREGROUND:


            coord->request_recovery( renderer_recovery_severity::device_reset );
            coord->notify_lifecycle( lifecycle_state::resumed_pending_rebuild );
            break;
        case CATA_APP_WILLENTERBACKGROUND:
        case CATA_APP_DIDENTERBACKGROUND:

            coord->notify_lifecycle( lifecycle_state::paused );
            break;
#endif
        case CATA_WINDOWEVENT_RESIZED:
        case CATA_WINDOWEVENT_PIXEL_SIZE_CHANGED:
            if( event->window.windowID == renderer_watch_window_id ) {
                coord->notify_resize();
            }
            break;
        default:
            break;
    }
    return true;
}

static tile_atlas_config applied_atlas_config;
static std::optional<bool> test_shader_variants_override;

static void select_applied_memory_preset()
{
    if( cata_shader::variant_pass *vp = get_shared_variant_pass() ) {
        vp->select_memory_preset(
            cata_shader::memory_preset_from_option_value( applied_atlas_config.mode ) );
    }
}

void apply_tile_atlas_options()
{



    SetDefaultTextureScaleQuality( get_option<std::string>( "SCALING_MODE" ) );
    applied_atlas_config.mode = get_option<std::string>( "MEMORY_MAP_MODE" );
    applied_atlas_config.fingerprint =
        compute_tileset_filter_fingerprint( applied_atlas_config.mode );
    select_applied_memory_preset();
}

const tile_atlas_config &applied_tile_atlas_config()
{
    return applied_atlas_config;
}

std::optional<atlas_bake_plan> resolve_atlas_bake_plan( const std::string &memory_map_mode )
{
    if( std::getenv( "CATA_FORCE_ATLAS_VARIANTS" ) ) {
        return atlas_bake_plan{};
    }
    cata_shader::variant_pass *vp = get_shared_variant_pass();
    bool shader_variants = false;
    if( vp ) {
        switch( vp->ensure_probed() ) {
            case cata_shader::probe_state::unsafe:
                display_buffer_scope_signal_recovery_required();
                return std::nullopt;
            case cata_shader::probe_state::available:
                shader_variants = true;
                break;
            case cata_shader::probe_state::unavailable:
                break;
        }
        if( vp->shader_fault() ) {

            return atlas_bake_plan{};
        }
    }
    if( test_shader_variants_override ) {
        shader_variants = *test_shader_variants_override;
    }


    const bool tint_shader = vp && vp->tint_available();
    return compute_atlas_bake_plan( shader_variants,
                                    cata_shader::memory_preset_from_option_value( memory_map_mode ),
                                    tint_shader );
}


static void WinCreate()
{

    Uint32 window_flags = CATA_WINDOW_RESIZABLE | CATA_WINDOW_HIGH_DPI;
    WindowWidth = TERMINAL_WIDTH * fontwidth * scaling_factor;
    WindowHeight = TERMINAL_HEIGHT * fontheight * scaling_factor;



    apply_tile_atlas_options();


    FullscreenMode desired_fullscreen = FullscreenMode::windowed;

#if !defined(__ANDROID__) && !defined(EMSCRIPTEN)
    if( get_option<std::string>( "FULLSCREEN" ) == "fullscreen" ) {
        desired_fullscreen = FullscreenMode::fullscreen_exclusive;
        fullscreen = true;


        SDL_SetHint( SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0" );
    } else if( get_option<std::string>( "FULLSCREEN" ) == "windowedbl" ) {
        desired_fullscreen = FullscreenMode::fullscreen_desktop;
        fullscreen = true;

        SDL_SetHint( SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "1" );
    } else if( get_option<std::string>( "FULLSCREEN" ) == "maximized" ) {
        window_flags |= CATA_WINDOW_MAXIMIZED;
    }
#endif
#if defined(EMSCRIPTEN)

    window_flags &= ~CATA_WINDOW_HIGH_DPI;
#endif
#if defined(__ANDROID__)

    window_flags = CATA_WINDOW_HIGH_DPI | CATA_WINDOW_MAXIMIZED;
#endif

    int display = std::stoi( get_option<std::string>( "DISPLAY" ) );
    if( display < 0 || display >= GetNumVideoDisplays() ) {
        display = 0;
    }

#if defined(__ANDROID__)


    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 6 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );


#if defined(SDL_HINT_ANDROID_TRAP_BACK_BUTTON )
    const bool trap_back_button = get_option<bool>( "ANDROID_TRAP_BACK_BUTTON" );
    SDL_SetHint( SDL_HINT_ANDROID_TRAP_BACK_BUTTON, trap_back_button ? "1" : "0" );
#endif


#if defined(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH)
    SDL_SetHint( SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1" );
#else
    SDL_SetHint( SDL_HINT_MOUSE_TOUCH_EVENTS, "0" );
    SDL_SetHint( SDL_HINT_TOUCH_MOUSE_EVENTS, "0" );
#endif
#endif

    ::window = CreateGameWindow( "", display, WindowWidth, WindowHeight, window_flags );
    throwErrorIf( !::window, "CreateWindow failed" );

#if defined(__ANDROID__)



    GetWindowSize( ::window.get(), &WindowWidth, &WindowHeight );
    const android_text_layout start = android_desired_layout();
    ::fontwidth = start.cell.x;
    ::fontheight = start.cell.y;
    android_buffer_scale = start.scale;
    TERMINAL_WIDTH = start.grid.x;
    TERMINAL_HEIGHT = start.grid.y;
#endif


    if( desired_fullscreen != FullscreenMode::windowed ) {
        SetWindowFullscreen( ::window.get(), desired_fullscreen );
    }

#if !defined(__ANDROID__) && !defined(EMSCRIPTEN)



    if( desired_fullscreen != FullscreenMode::windowed
        || ( window_flags & CATA_WINDOW_MAXIMIZED ) ) {
        GetWindowSize( ::window.get(), &WindowWidth, &WindowHeight );

        TERMINAL_WIDTH = WindowWidth / fontwidth / scaling_factor;
        TERMINAL_HEIGHT = WindowHeight / fontheight / scaling_factor;
    }
#endif
    pixel_format = SDL_GetWindowPixelFormat( ::window.get() );
    throwErrorIf( pixel_format == SDL_PIXELFORMAT_UNKNOWN, "SDL_GetWindowPixelFormat failed" );

#if !defined(__ANDROID__)






    bool software_renderer = false;
    std::string renderer_name;
#else
    bool software_renderer = get_option<bool>( "SOFTWARE_RENDERING" );
    std::string renderer_name = software_renderer ? "software" : "";
#endif

#if !defined(__ANDROID__)
    const bool gpu_driver_from_env = std::getenv( "SDL_GPU_DRIVER" ) != nullptr;
    const std::string gpu_backend = get_option<std::string>( "GPU_BACKEND" );
    if( const std::optional<std::string> hint = gpu_backend_hint( gpu_backend, gpu_driver_from_env ) ) {
        SDL_SetHint( SDL_HINT_GPU_DRIVER, hint->c_str() );
    }
    DebugLog( D_INFO, DC_ALL ) << "GPU backend option: " << gpu_backend << " (env override: "
                               << ( gpu_driver_from_env ? "yes" : "no" ) << ")";
#endif

#  if defined(_WIN32)
    SDL_SetHint( SDL_HINT_RENDER_DRIVER, "gpu,direct3d12,direct3d11,opengl" );





    if( !std::getenv( "SDL_GPU_DRIVER" )
        && !SDL_GetHint( SDL_HINT_GPU_DRIVER )
        && only_spirv_shader_artifacts_present() ) {
        SDL_SetHint( SDL_HINT_GPU_DRIVER, "vulkan" );
        dbg( D_INFO ) << "Only SPIR-V shader artifacts present; biasing the "
                      "GPU device toward Vulkan.";
    }
#  else
    SDL_SetHint( SDL_HINT_RENDER_DRIVER, "gpu,opengl" );
#  endif
    if( !software_renderer ) {
        dbg( D_INFO ) << "Attempting to initialize accelerated SDL renderer.";

        renderer = create_game_renderer( renderer_name, false );
        if( printErrorIf( !renderer,
                          "Failed to initialize accelerated renderer, falling back to software rendering" ) ) {
            software_renderer = true;
        } else if( !SetupRenderTarget() ) {
            dbg( D_ERROR ) <<
                           "Failed to initialize display buffer under accelerated rendering, falling back to software rendering.";
            software_renderer = true;
            display_buffer.reset();
            renderer.reset();


            display_buffer_scope_clear_recovery_required();
        }
    }

    if( software_renderer ) {
        renderer = create_game_renderer( {}, true );
        throwErrorIf( !renderer, "Failed to initialize software renderer" );
        throwErrorIf( !SetupRenderTarget(),
                      "Failed to initialize display buffer under software rendering, unable to continue." );
    }

    detect_renderer_backend();


    refresh_drawable_dims();

    SetWindowMinimumSize( ::window.get(), fontwidth * EVEN_MINIMUM_TERM_WIDTH * scaling_factor,
                          fontheight * EVEN_MINIMUM_TERM_HEIGHT * scaling_factor );

#if defined(__ANDROID__)

    if( desired_fullscreen != FullscreenMode::windowed
        || ( window_flags & CATA_WINDOW_MAXIMIZED ) ) {
        GetWindowSize( ::window.get(), &WindowWidth, &WindowHeight );
    }
#endif

    ClearScreen();

    if( get_option<bool>( "ENABLE_JOYSTICK" ) ) {
        gamepad::init();
    }


    init_sound();

    rebuild_geometry_strategy( software_renderer );

    shared_variant_pass = std::make_unique<cata_shader::variant_pass>( renderer.get() );
    shared_lightmap = std::make_unique<smooth_lightmap>();
    select_applied_memory_preset();

    imclient = std::make_unique<cataimgui::client>( renderer, window, geometry );

    renderer_coordinator.seed_renderer_policy( renderer_name );



    renderer_watch_window_id = SDL_GetWindowID( ::window.get() );
    if( !SDL_AddEventWatch( renderer_event_watch, &renderer_coordinator ) ) {
        DebugLog( D_ERROR, DC_ALL ) << "Failed to register renderer event watch: " << SDL_GetError();
    }
}

static void WinDestroy()
{


    SDL_RemoveEventWatch( renderer_event_watch, &renderer_coordinator );
    imclient.reset();
    shutdown_sound();
    tilecontext.reset();
    gamepad::quit();
    geometry.reset();
    reset_shared_lightmap();
    shared_lightmap.reset();
    shared_variant_pass.reset();
    display_buffer.reset();
    renderer.reset();
    ::window.reset();
}


static const SDL_Color &color_as_sdl( const unsigned char color )
{
    return windowsPalette[color];
}

#if defined(__ANDROID__)
void draw_quick_shortcuts();
void draw_virtual_joystick();

static bool quick_shortcuts_enabled = true;





struct android_visible_frame_inbox {
    std::atomic<uint32_t> seq{ 0 };
    std::atomic<int> x{ 0 };
    std::atomic<int> y{ 0 };
    std::atomic<int> w{ 0 };
    std::atomic<int> h{ 0 };
    std::atomic<bool> valid{ false };
    std::atomic<bool> visible{ false };

    void publish( int left, int top, int right, int bottom, bool frame_visible ) {
        const uint32_t s = seq.load( std::memory_order_relaxed );
        seq.store( s + 1, std::memory_order_relaxed );
        std::atomic_thread_fence( std::memory_order_release );
        x.store( left, std::memory_order_relaxed );
        y.store( top, std::memory_order_relaxed );
        w.store( std::max( 0, right - left ), std::memory_order_relaxed );
        h.store( std::max( 0, bottom - top ), std::memory_order_relaxed );
        valid.store( true, std::memory_order_relaxed );
        visible.store( frame_visible, std::memory_order_relaxed );
        std::atomic_thread_fence( std::memory_order_release );
        seq.store( s + 2, std::memory_order_relaxed );
    }

    uint32_t read_frame( SDL_Rect &out, bool &has_frame, bool &frame_visible ) const {
        uint32_t s1 = 0;
        uint32_t s2 = 0;
        int rx = 0;
        int ry = 0;
        int rw = 0;
        int rh = 0;
        bool rv = false;
        bool rvisible = false;
        do {
            s1 = seq.load( std::memory_order_acquire );
            rx = x.load( std::memory_order_relaxed );
            ry = y.load( std::memory_order_relaxed );
            rw = w.load( std::memory_order_relaxed );
            rh = h.load( std::memory_order_relaxed );
            rv = valid.load( std::memory_order_relaxed );
            rvisible = visible.load( std::memory_order_relaxed );
            std::atomic_thread_fence( std::memory_order_acquire );
            s2 = seq.load( std::memory_order_relaxed );
        } while( s1 != s2 || ( s1 & 1u ) );
        out.x = rx;
        out.y = ry;
        out.w = rw;
        out.h = rh;
        has_frame = rv;
        frame_visible = rvisible;
        return s1;
    }

    uint32_t even_sequence() const {
        return seq.load( std::memory_order_acquire ) & ~uint32_t( 1 );
    }
};

static_assert( std::atomic<int>::is_always_lock_free,
               "visible-frame inbox needs lock-free int atomics on every shipped ABI" );
static_assert( std::atomic<bool>::is_always_lock_free,
               "visible-frame inbox needs lock-free bool atomics on every shipped ABI" );
static_assert( std::atomic<uint32_t>::is_always_lock_free,
               "visible-frame inbox needs lock-free uint32 atomics on every shipped ABI" );




static constexpr int surface_rect_scale = 10000;
static android_visible_frame_inbox visible_frame_inbox;
static android_visible_frame_inbox safe_area_inbox;

static android_visible_frame_inbox outer_size_inbox;

static SDL_Rect surface_rect_to_window( const SDL_Rect &r )
{
    SDL_Rect out;
    out.x = r.x * WindowWidth / surface_rect_scale;
    out.y = r.y * WindowHeight / surface_rect_scale;
    out.w = r.w * WindowWidth / surface_rect_scale;
    out.h = r.h * WindowHeight / surface_rect_scale;
    return out;
}

extern "C" {

    JNIEXPORT void JNICALL Java_com_cleverraven_cataclysmdda_CataclysmDDA_onNativeImeInsetsChanged(
        JNIEnv *env, jclass jcls, jint left, jint top, jint right, jint bottom, jboolean visible )
    {
        ( void )env;
        ( void )jcls;
        visible_frame_inbox.publish( left, top, right, bottom, visible == JNI_TRUE );

        PushWakeEvent();
    }

    JNIEXPORT void JNICALL Java_com_cleverraven_cataclysmdda_CataclysmDDA_onNativeSafeAreaChanged(
        JNIEnv *env, jclass jcls, jint left, jint top, jint right, jint bottom )
    {
        ( void )env;
        ( void )jcls;
        safe_area_inbox.publish( left, top, right, bottom, true );
        PushWakeEvent();
    }

    JNIEXPORT void JNICALL Java_com_cleverraven_cataclysmdda_CataclysmDDA_onNativeOuterSizeChanged(
        JNIEnv *env, jclass jcls, jint width, jint height )
    {
        ( void )env;
        ( void )jcls;
        outer_size_inbox.publish( 0, 0, width, height, true );
        PushWakeEvent();
    }

}


static point android_outer_size()
{
    SDL_Rect r;
    bool has = false;
    bool unused_visible = false;
    outer_size_inbox.read_frame( r, has, unused_visible );
    return has ? point( r.w, r.h ) : point::zero;
}


static std::optional<SDL_Rect> android_visible_ime_frame()
{
    SDL_Rect frame;
    bool has_frame = false;
    bool visible = false;
    visible_frame_inbox.read_frame( frame, has_frame, visible );
    if( !has_frame || !visible || frame.w <= 0 || frame.h <= 0 ) {
        return std::nullopt;
    }
    return surface_rect_to_window( frame );
}


static bool android_shortcuts_above_keyboard()
{
    return quick_shortcuts_enabled &&
           terminal_layout::is_portrait( point( WindowWidth, WindowHeight ) ) &&
           android_visible_ime_frame().has_value();
}




static SDL_Rect android_layout_bounds()
{
    SDL_Rect bounds{ 0, 0, WindowWidth, WindowHeight };
    if( get_option<bool>( "ANDROID_RENDER_SAFE_AREA" ) ) {


        SDL_Rect safe;
        bool has_safe = false;
        bool unused_visible = false;
        safe_area_inbox.read_frame( safe, has_safe, unused_visible );
        if( has_safe && safe.w > 0 && safe.h > 0 ) {
            bounds = surface_rect_to_window( safe );
        } else if( SDL_GetWindowSafeArea( ::window.get(), &safe ) && safe.w > 0 && safe.h > 0 ) {
            bounds = safe;
        }
    }
    const bool overlap = get_option<bool>( "ANDROID_SHORTCUT_OVERLAP" );
    int reserved = 0;
    if( terminal_layout::is_portrait( point( WindowWidth, WindowHeight ) ) ) {


        reserved = terminal_layout::portrait_reserved_bottom( bounds.h,
                   get_option<int>( "ANDROID_PORTRAIT_RESERVE" ),
                   overlap ? 0 : static_cast<int>( quick_shortcut_height() ) );
    } else if( !overlap && quick_shortcuts_enabled ) {
        reserved = static_cast<int>( quick_shortcut_height() );
    }

    bounds.h = std::max( 1, bounds.h - reserved );
    return bounds;
}


static bool android_bitmap_typeface()
{
    return !android_fonts.typeface.empty() &&
           is_bitmap_typeface( android_fonts.typeface.front().path );
}



static int android_font_point_size( const point &cell )
{
    return android_bitmap_typeface() ? get_option<int>( "FONT_SIZE" ) : cell.y;
}

static point android_grid_for( const point &cell, const int scale )
{
    const SDL_Rect bounds = android_layout_bounds();
    const point caps( get_option<int>( "ANDROID_MAX_COLUMNS" ), get_option<int>( "ANDROID_MAX_ROWS" ) );
    return terminal_layout::fit_grid( point( bounds.w / scale, bounds.h / scale ), cell, caps );
}

static android_text_layout android_desired_layout()
{
    const int reference = terminal_layout::sizing_reference( point( WindowWidth, WindowHeight ),
                          android_outer_size() );
    android_text_layout out;
    if( android_bitmap_typeface() ) {
        out.cell = point( get_option<int>( "FONT_WIDTH" ), get_option<int>( "FONT_HEIGHT" ) );
        out.scale = terminal_layout::bitmap_scale( reference, out.cell.x );
    } else {
        out.cell = terminal_layout::resolve_text_size( get_option<std::string>( "ANDROID_TEXT_SIZE" ),
                   get_option<int>( "ANDROID_TEXT_SIZE_CUSTOM" ), reference );
    }
    out.grid = android_grid_for( out.cell, out.scale );
    return out;
}

static SDL_Rect get_android_render_rect( const int buffer_w, const int buffer_h )
{
    const SDL_Rect bounds = android_layout_bounds();
    const point size = terminal_layout::present_size( point( buffer_w, buffer_h ),
                       point( bounds.w, bounds.h ), android_buffer_scale );

    SDL_Rect dstrect;
    dstrect.w = size.x;
    dstrect.h = size.y;
    dstrect.x = bounds.x + ( bounds.w - size.x ) / 2;
    dstrect.y = bounds.y;

    const std::optional<SDL_Rect> ime_visible_frame = android_visible_ime_frame();
    if( get_option<bool>( "ANDROID_KEYBOARD_SCREEN_SCALE" ) && ime_visible_frame ) {
        const int ime_right = ime_visible_frame->x + ime_visible_frame->w;
        int ime_bottom = ime_visible_frame->y + ime_visible_frame->h;
        if( android_shortcuts_above_keyboard() ) {
            ime_bottom -= static_cast<int>( quick_shortcut_height() );
        }
        if( ime_right < dstrect.x + dstrect.w ) {
            dstrect.w = std::max( 1, ime_right - dstrect.x );
        }
        if( ime_bottom < dstrect.y + dstrect.h ) {
            dstrect.h = std::max( 1, ime_bottom - dstrect.y );
        }
    }
    return dstrect;
}

#endif

static void draw_gamepad_radial_menu();



static bool shader_variants_available_now()
{
    const cata_shader::variant_pass *vp = get_shared_variant_pass();
    if( vp && vp->shader_fault() ) {
        return false;
    }
    if( test_shader_variants_override ) {
        return *test_shader_variants_override;
    }
    return vp && vp->available();
}



static bool request_tile_repair_if_needed()
{
    if( !ts_cache.any_live_bundle_needs_repair( applied_atlas_config.mode,
            applied_atlas_config.fingerprint, shader_variants_available_now() ) ) {
        return false;
    }
    renderer_coordinator.request_recovery( renderer_recovery_severity::device_reset );
    return true;
}





static bool present_gate()
{
    if( renderer_coordinator.should_abort_frame() ) {
        needupdate = true;
        return false;
    }
    if( request_tile_repair_if_needed() ) {
        needupdate = true;
        return false;
    }
    return true;
}

void refresh_display()
{
    needupdate = false;
    lastupdate = GetTicks();

    if( test_mode ) {
        return;
    }

    if( !present_gate() ) {

        return;
    }

#if defined(SDL_SOUND)
    sound_backend::poll();
#endif




    cata_shader::variant_pass *present_vp = get_shared_variant_pass();
    {
        const bind_result r = permanent_render_target_bind( renderer, nullptr, present_vp );
        if( r == bind_result::failed_in_switch ) {

            display_buffer_scope_signal_recovery_required();
            return;
        }
        if( r != bind_result::ok ) {

            return;
        }
    }
    ClearScreen();
#if defined(__ANDROID__)
    SDL_Rect dstrect = get_android_render_rect( TERMINAL_WIDTH * fontwidth,
                       TERMINAL_HEIGHT * fontheight );
    RenderCopy( renderer, display_buffer, NULL, &dstrect );
#else


    const SDL_Rect dstrect = get_display_buffer_render_rect();
    if( dstrect.w > 0 && dstrect.h > 0 ) {
        RenderCopy( renderer, display_buffer, nullptr, &dstrect );
    } else {
        RenderCopy( renderer, display_buffer, nullptr, nullptr );
    }
#endif

#if defined(__ANDROID__)
    if( g ) {
        draw_quick_shortcuts();
    }
    draw_virtual_joystick();
#endif
    draw_gamepad_radial_menu();
    RenderPresent( renderer );
}


static void try_sdl_update()
{
    uint32_t now = GetTicks();
    if( now - lastupdate >= interval ) {
        refresh_display();
    } else {
        needupdate = true;
    }
}

void drain_renderer_recovery()
{
    renderer_coordinator.drain_pending();
}

void pump_until_renderer_foreground()
{
    while( renderer_coordinator.lifecycle_paused() ) {
        inp_mngr.pump_events();
    }
}

bool renderer_should_abort_frame()
{
    return renderer_coordinator.should_abort_frame();
}

uint64_t renderer_resource_generation()
{
    return renderer_coordinator.resource_generation();
}

void get_display_buffer_dims( int *w, int *h )
{
    int buf_w = 0;
    int buf_h = 0;
    if( display_buffer ) {
        SDL_PropertiesID props = SDL_GetTextureProperties( display_buffer.get() );
        if( props ) {
            buf_w = static_cast<int>( SDL_GetNumberProperty( props,
                                      SDL_PROP_TEXTURE_WIDTH_NUMBER, 0 ) );
            buf_h = static_cast<int>( SDL_GetNumberProperty( props,
                                      SDL_PROP_TEXTURE_HEIGHT_NUMBER, 0 ) );
        }
    }
    if( w ) {
        *w = buf_w;
    }
    if( h ) {
        *h = buf_h;
    }
}

SDL_Point window_to_display_buffer_coords( SDL_Point window_pt )
{
    if( !window ) {
        return window_pt;
    }
    int buf_w = 0;
    int buf_h = 0;
    get_display_buffer_dims( &buf_w, &buf_h );
    if( buf_w <= 0 || buf_h <= 0 ) {
        return window_pt;
    }
#if defined(__ANDROID__)



    const SDL_Rect dstrect = get_android_render_rect( TERMINAL_WIDTH * fontwidth,
                             TERMINAL_HEIGHT * fontheight );
    if( dstrect.w <= 0 || dstrect.h <= 0 ) {
        return window_pt;
    }
    return SDL_Point{
        static_cast<int>( static_cast<int64_t>( window_pt.x - dstrect.x ) * buf_w / dstrect.w ),
        static_cast<int>( static_cast<int64_t>( window_pt.y - dstrect.y ) * buf_h / dstrect.h )
    };
#else

    if( renderer ) {
        float rx = 0.0f;
        float ry = 0.0f;
        if( SDL_RenderCoordinatesFromWindow( renderer.get(),
                                             static_cast<float>( window_pt.x ), static_cast<float>( window_pt.y ), &rx, &ry ) ) {
            return SDL_Point{ static_cast<int>( rx ), static_cast<int>( ry ) };
        }
    }
    return window_pt;
#endif
}

void convert_event_to_display_buffer_coords( SDL_Event *event )
{
    if( !event ) {
        return;
    }
    switch( event->type ) {
        case CATA_MOUSEMOTION: {
            SDL_Point pt { static_cast<int>( event->motion.x ), static_cast<int>( event->motion.y ) };
            const SDL_Point conv = window_to_display_buffer_coords( pt );
            event->motion.x = static_cast<float>( conv.x );
            event->motion.y = static_cast<float>( conv.y );
            break;
        }
        case CATA_MOUSEBUTTONDOWN:
        case CATA_MOUSEBUTTONUP: {
            SDL_Point pt { static_cast<int>( event->button.x ), static_cast<int>( event->button.y ) };
            const SDL_Point conv = window_to_display_buffer_coords( pt );
            event->button.x = static_cast<float>( conv.x );
            event->button.y = static_cast<float>( conv.y );
            break;
        }
        case CATA_MOUSEWHEEL: {
            SDL_Point pt { static_cast<int>( event->wheel.mouse_x ), static_cast<int>( event->wheel.mouse_y ) };
            const SDL_Point conv = window_to_display_buffer_coords( pt );
            event->wheel.mouse_x = static_cast<float>( conv.x );
            event->wheel.mouse_y = static_cast<float>( conv.y );
            break;
        }
        default:
            break;
    }
}

cata_shader::variant_pass *get_shared_variant_pass()
{
    return shared_variant_pass.get();
}

smooth_lightmap *get_shared_lightmap()
{
    return shared_lightmap.get();
}

static void reset_shared_lightmap()
{
    if( !shared_lightmap ) {
        return;
    }
    if( shared_variant_pass ) {
        shared_variant_pass->drop_lit();
    }
    shared_lightmap->reset();
}

bool unbind_sprite_shader()
{
    cata_shader::variant_pass *vp = get_shared_variant_pass();
    if( vp && !vp->flush() ) {
        display_buffer_scope_signal_recovery_required();
        return false;
    }
    return true;
}

namespace
{





int display_buffer_scope_depth = 0;
bool display_buffer_scope_aborted = false;
bool display_buffer_scope_invalid = false;
bool display_buffer_scope_recovery_required = false;
}

bool display_buffer_scope_is_invalid()
{
    return display_buffer_scope_invalid;
}

bool display_buffer_scope_recovery_pending()
{
    return display_buffer_scope_recovery_required;
}

void display_buffer_scope_signal_recovery_required()
{
    display_buffer_scope_recovery_required = true;
    display_buffer_scope_invalid = true;
}




bool renderer_boundary_recovery_pending()
{
    return display_buffer_scope_recovery_pending();
}

void renderer_boundary_signal_recovery_required()
{
    display_buffer_scope_signal_recovery_required();
}

void display_buffer_scope_clear_recovery_required()
{
    display_buffer_scope_recovery_required = false;
}

display_buffer_draw_scope::display_buffer_draw_scope( const bool allow_during_recovery )
{
    if( display_buffer_scope_depth++ != 0 ) {

        return;
    }
    outermost_ = true;
    display_buffer_scope_aborted = false;


    display_buffer_scope_invalid = display_buffer_scope_recovery_required;
    if( !allow_during_recovery && renderer_should_abort_frame() ) {


        display_buffer_scope_invalid = true;
        return;
    }
    if( !renderer || !display_buffer ) {
        display_buffer_scope_invalid = true;
        return;
    }
    if( display_buffer_scope_recovery_required ) {


        return;
    }
    cata_shader::variant_pass *vp = get_shared_variant_pass();
    const bind_result r = permanent_render_target_bind( renderer, display_buffer.get(), vp );
    if( r == bind_result::failed_in_switch ) {

        display_buffer_scope_recovery_required = true;
        display_buffer_scope_invalid = true;
        return;
    }
    if( r != bind_result::ok ) {

        display_buffer_scope_invalid = true;
        return;
    }
    active_ = true;
}

void display_buffer_draw_scope::abort_unbind()
{
    display_buffer_scope_aborted = true;
}

display_buffer_draw_scope::~display_buffer_draw_scope()
{
    --display_buffer_scope_depth;
    if( !outermost_ ) {
        return;
    }



    const bool was_active = active_;
    const bool was_aborted = display_buffer_scope_aborted;
    display_buffer_scope_invalid = display_buffer_scope_recovery_required;
    if( !was_active || was_aborted || !renderer
        || display_buffer_scope_recovery_required ) {



        return;
    }
    cata_shader::variant_pass *vp = get_shared_variant_pass();
    if( permanent_render_target_bind( renderer, nullptr, vp )
        == bind_result::failed_in_switch ) {

        display_buffer_scope_recovery_required = true;
    }
}

bool display_buffer_draw_scope::should_draw() const
{
    return !display_buffer_scope_invalid && !renderer_should_abort_frame();
}

void clear_window_area( const catacurses::window &win_ )
{
    display_buffer_draw_scope draw_scope;
    if( !draw_scope.should_draw() ) {
        return;
    }
    cata_cursesport::WINDOW *const win = win_.get<cata_cursesport::WINDOW>();
    geometry->rect( renderer, point( win->pos.x * fontwidth, win->pos.y * fontheight ),
                    win->width * fontwidth, win->height * fontheight, color_as_sdl( catacurses::black ) );
}



static cata_shader::variant_pass *shared_variant_pass_or_null()
{
    return get_shared_variant_pass();
}







static constexpr uint64_t make_lifecycle_epoch( uint64_t seq, lifecycle_state s )
{
    return recovery_drain_planner::make_lifecycle_epoch( seq, s );
}
static constexpr uint64_t lifecycle_seq_of( uint64_t epoch )
{
    return recovery_drain_planner::lifecycle_seq_of( epoch );
}
static constexpr lifecycle_state lifecycle_state_of( uint64_t epoch )
{
    return recovery_drain_planner::lifecycle_state_of( epoch );
}

static_assert( std::atomic<uint64_t>::is_always_lock_free );
static_assert( std::atomic<uint32_t>::is_always_lock_free );
static_assert( std::atomic<renderer_recovery_severity>::is_always_lock_free );



renderer_resource_coordinator renderer_coordinator;

void renderer_resource_coordinator::request_recovery( renderer_recovery_severity sev )
{
    renderer_recovery_severity cur = pending_severity_.load();
    while( static_cast<int>( sev ) > static_cast<int>( cur ) ) {
        if( pending_severity_.compare_exchange_weak( cur, sev ) ) {
            break;
        }
    }
}

void renderer_resource_coordinator::notify_resize()
{
    pending_resize_epoch_.fetch_add( 1 );
}

void renderer_resource_coordinator::notify_lifecycle( lifecycle_state ev )
{
    uint64_t cur = lifecycle_epoch_.load();
    uint64_t desired = 0;
    do {
        desired = make_lifecycle_epoch( lifecycle_seq_of( cur ) + 1, ev );
    } while( !lifecycle_epoch_.compare_exchange_weak( cur, desired ) );
}

bool renderer_resource_coordinator::is_render_allowed() const
{
    return planner_.state() == renderer_recovery_state::ready
           && pending_severity_.load() == renderer_recovery_severity::none
           && pending_resize_epoch_.load() == planner_.observed_resize_epoch()
           && lifecycle_state_of( lifecycle_epoch_.load() ) == lifecycle_state::active
           && !display_buffer_scope_recovery_pending();
}

bool renderer_resource_coordinator::external_inbox_blocks_render() const
{
    return pending_severity_.load() != renderer_recovery_severity::none
           || lifecycle_state_of( lifecycle_epoch_.load() ) != lifecycle_state::active
           || pending_resize_epoch_.load() != planner_.observed_resize_epoch()
           || display_buffer_scope_recovery_pending();
}

bool renderer_resource_coordinator::recovery_blank_blocked() const
{
    const uint64_t epoch = lifecycle_epoch_.load();
    const lifecycle_state ls = lifecycle_state_of( epoch );
    if( ls == lifecycle_state::paused ) {
        return true;
    }

    if( ls == lifecycle_state::resumed_pending_rebuild
        && lifecycle_seq_of( epoch ) != planner_.serviced_lifecycle_seq() ) {
        return true;
    }
    return pending_severity_.load() != renderer_recovery_severity::none
           || pending_resize_epoch_.load() != planner_.observed_resize_epoch()
           || display_buffer_scope_recovery_pending();
}

bool renderer_resource_coordinator::should_abort_frame() const
{
    return planner_.abort_latch() || external_inbox_blocks_render();
}



static void for_each_unique_tile_context( const std::function<void( cata_tiles & )> &fn )
{
    cata_tiles *ctxs[] = { tilecontext.get(), closetilecontext.get(),
                           fartilecontext.get(), overmap_tilecontext.get(),
                           portrait_tilecontext.get()
                         };
    cata_tiles *seen[5] = {};
    size_t n = 0;
    for( cata_tiles *c : ctxs ) {
        if( !c ) {
            continue;
        }
        bool dup = false;
        for( size_t i = 0; i < n; ++i ) {
            if( seen[i] == c ) {
                dup = true;
                break;
            }
        }
        if( dup ) {
            continue;
        }
        seen[n++] = c;
        fn( *c );
    }
}

void on_tiles_options_changed()
{
    apply_tile_atlas_options();
    for_each_unique_tile_context( []( cata_tiles & ctx ) {
        ctx.on_options_changed();
    } );


    request_tile_repair_if_needed();
}

static void reset_context_minimaps()
{
    for_each_unique_tile_context( []( cata_tiles & c ) {
        c.reset_minimap();
    } );
}



static void reset_context_tint_masks()
{
    for_each_unique_tile_context( []( cata_tiles & c ) {
        c.reset_tint_mask();
    } );
    reset_shared_lightmap();
}



static void release_font_roots()
{
    for( Font_Ptr *f : {
             &font, &gui_font, &map_font, &overmap_font
         } ) {
        if( *f ) {
            ( *f )->release_gpu_resources();
        }
    }
}



static void rebuild_font_roots()
{
    for( Font_Ptr *f : {
             &font, &gui_font, &map_font, &overmap_font
         } ) {
        if( *f ) {
            ( *f )->rebuild_for_renderer( renderer );
        }
    }
}

recipe_result renderer_resource_coordinator::run_recipe_for( renderer_recovery_severity sev )
{
    const uint64_t resource_gen_before = renderer_resource_generation_;
    const renderer_texture_generations gens_before{
        renderer_instance_generation_, gpu_textures_generation_ };
    recipe_result r{ recipe_outcome::success };
    switch( sev ) {
        case renderer_recovery_severity::targets_reset:
            r = recipe_targets_reset();
            break;
        case renderer_recovery_severity::device_reset:
            r = recipe_device_reset();
            break;
        case renderer_recovery_severity::device_lost:
            r = recipe_device_lost();
            break;
        case renderer_recovery_severity::none:
            return r;
    }
    if( r.outcome == recipe_outcome::success ) {
        assert_recipe_postcondition( resource_gen_before, gens_before );
    }
    return r;
}

void renderer_resource_coordinator::assert_recipe_postcondition(
    [[maybe_unused]] const uint64_t resource_gen_before,
    [[maybe_unused]] const renderer_texture_generations gens_before ) const
{


    cata_assert( renderer );
    cata_assert( display_buffer );
    cata_assert( !display_buffer_scope_recovery_pending() );
    cata_assert( renderer_resource_generation_ >= resource_gen_before );
    cata_assert( renderer_instance_generation_ >= gens_before.instance );
    cata_assert( gpu_textures_generation_ >= gens_before.textures );
}

bool renderer_resource_coordinator::check_pause_abort()
{
    if( test_phase_action_ != test_phase_action::none && test_phase_countdown_ > 0 ) {
        --test_phase_countdown_;
        if( test_phase_countdown_ == 0 ) {
            const test_phase_action action = test_phase_action_;
            test_phase_action_ = test_phase_action::none;
            test_phase_fault_fired_ = true;
            if( action == test_phase_action::fail_retry ) {
                planner_.note_pause_abort();
                return true;
            }


            request_recovery( renderer_recovery_severity::device_reset );
        }
    }
    if( lifecycle_state_of( lifecycle_epoch_.load() ) == lifecycle_state::paused ) {
        planner_.note_pause_abort();
        return true;
    }
    return false;
}





bool renderer_resource_coordinator::safe_pre_teardown_unbind( recipe_result &out )
{
    const bind_result r = permanent_render_target_bind( renderer, nullptr,
                          shared_variant_pass_or_null() );
    if( r == bind_result::failed_in_switch ) {
        out = { recipe_outcome::restart_required, renderer_recovery_severity::device_lost };
        return false;
    }
    return true;
}

void renderer_resource_coordinator::run_post_success_full_invalidation()
{



    if( !recovery_blank_blocked() ) {


        display_buffer_draw_scope draw_scope( true );
        if( !display_buffer_scope_is_invalid() ) {
            ClearScreen();
        }
    }
    ui_manager::invalidate_all_ui_adaptors();


    cata_cursesport::bump_curses_render_epoch();
    needupdate = true;
    repaint_requested_ = true;
}

recipe_result renderer_resource_coordinator::recipe_targets_reset()
{
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    if( cata_shader::variant_pass *vp = get_shared_variant_pass() ) {
        if( !vp->flush() ) {


            return { recipe_outcome::restart_required, renderer_recovery_severity::device_lost };
        }
    }
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    recipe_result restart { recipe_outcome::failure };
    if( !safe_pre_teardown_unbind( restart ) ) {
        return restart;
    }
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    reset_context_minimaps();
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    display_buffer.reset();
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    const recipe_result setup = setup_target_phase();
    if( setup.outcome != recipe_outcome::success ) {
        return setup;
    }
    ++renderer_resource_generation_;
    display_buffer_scope_clear_recovery_required();
    run_post_success_full_invalidation();
    return { recipe_outcome::success };
}

recipe_result renderer_resource_coordinator::recipe_device_reset()
{
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    if( cata_shader::variant_pass *vp = get_shared_variant_pass() ) {
        if( !vp->flush() ) {
            return { recipe_outcome::restart_required, renderer_recovery_severity::device_lost };
        }
    }
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    recipe_result restart { recipe_outcome::failure };
    if( !safe_pre_teardown_unbind( restart ) ) {
        return restart;
    }
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    reset_context_minimaps();
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    reset_context_tint_masks();
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }


    release_live_atlases();
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }


    replay_quarantine_.drain_live_renderer();
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    release_font_roots();
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    if( geometry ) {
        geometry->release_gpu_resources();
    }
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    loading_ui::release_gpu_resources();
    display_buffer.reset();
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    if( cata_shader::variant_pass *vp = get_shared_variant_pass() ) {
        vp->release_gpu_resources();
    }
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    if( imclient ) {
        imclient->destroy_backend_device_objects();
    }



    ++gpu_textures_generation_;
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    if( cata_shader::variant_pass *vp = get_shared_variant_pass() ) {
        vp->rebind_renderer( renderer.get() );
    }
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    const recipe_result setup = setup_target_phase();
    if( setup.outcome != recipe_outcome::success ) {
        return setup;
    }
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    if( geometry ) {
        geometry->rebuild_for_renderer( renderer );
    }
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    rebuild_font_roots();
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    const atlas_upload_interrupt replay = replay_live_atlases();
    if( replay != atlas_upload_interrupt::none ) {
        return map_replay_interrupt( replay );
    }
    ++renderer_resource_generation_;
    display_buffer_scope_clear_recovery_required();
    run_post_success_full_invalidation();
    return { recipe_outcome::success };
}





bool renderer_resource_coordinator::install_renderer_with_fallback()
{
    for( const bool software : {
             software_renderer_, true
         } ) {
        if( check_pause_abort() ) {
            return false;
        }
        if( imclient ) {
            imclient->shutdown_renderer_backend();
            if( check_pause_abort() ) {
                return false;
            }
            imclient->shutdown_platform_backend();
        }
        if( check_pause_abort() ) {
            return false;
        }
        renderer.reset();
        if( check_pause_abort() ) {
            return false;
        }
        renderer = create_game_renderer( renderer_name_, software );
        if( !renderer ) {
            continue;
        }
        if( check_pause_abort() ) {
            return false;
        }
        detect_renderer_backend();
        if( check_pause_abort() ) {
            return false;
        }
        if( imclient ) {
            imclient->init_platform_backend();
            if( check_pause_abort() ) {
                return false;
            }
            imclient->init_renderer_backend();
        }
        if( check_pause_abort() ) {
            return false;
        }
        if( cata_shader::variant_pass *vp = get_shared_variant_pass() ) {
            vp->rebind_renderer( renderer.get() );
        }
        if( check_pause_abort() ) {
            return false;
        }



        display_buffer_scope_clear_recovery_required();
        if( !SetupRenderTarget() ) {
            continue;
        }
        if( check_pause_abort() ) {
            return false;
        }
        record_display_buffer_dims();
        software_renderer_ = IsRendererSoftware( renderer );
        ++renderer_instance_generation_;
        ++gpu_textures_generation_;
        return true;
    }
    return false;
}

recipe_result renderer_resource_coordinator::recipe_device_lost()
{



    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    if( cata_shader::variant_pass *vp = get_shared_variant_pass() ) {
        if( !vp->flush() ) {
            vp->force_abandon_gpu_resources();
        }
    }
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }


    if( renderer ) {
        permanent_render_target_bind( renderer, nullptr, shared_variant_pass_or_null() );
    }
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    reset_context_minimaps();
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    reset_context_tint_masks();
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }


    release_live_atlases();



    replay_quarantine_.abandon_pre_lost_renderer();
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    release_font_roots();
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    if( geometry ) {
        geometry->release_gpu_resources();
    }
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    loading_ui::release_gpu_resources();
    display_buffer.reset();
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    if( cata_shader::variant_pass *vp = get_shared_variant_pass() ) {
        vp->release_gpu_resources();
    }
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    if( !install_renderer_with_fallback() ) {
        return { recipe_outcome::failure };
    }
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }


    rebuild_geometry_strategy( software_renderer_ );
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    rebuild_font_roots();
    if( check_pause_abort() ) {
        return { recipe_outcome::failure };
    }
    const atlas_upload_interrupt replay = replay_live_atlases();
    if( replay != atlas_upload_interrupt::none ) {
        return map_replay_interrupt( replay );
    }
    ++renderer_resource_generation_;
    display_buffer_scope_clear_recovery_required();
    run_post_success_full_invalidation();
    return { recipe_outcome::success };
}

void renderer_resource_coordinator::record_display_buffer_dims()
{
    const point d = compute_display_buffer_dims();
    display_buffer_w_ = d.x;
    display_buffer_h_ = d.y;
}

void renderer_resource_coordinator::release_live_atlases() const
{
    ts_cache.release_live_atlases();
}

atlas_upload_interrupt renderer_resource_coordinator::replay_poll()
{
    if( test_replay_pause_countdown_ > 0 ) {
        --test_replay_pause_countdown_;
        if( test_replay_pause_countdown_ == 0 ) {


            notify_lifecycle( lifecycle_state::paused );
            return atlas_upload_interrupt::paused;
        }
    }
    const uint64_t epoch = lifecycle_epoch_.load();
    const lifecycle_state ls = lifecycle_state_of( epoch );
    if( ls == lifecycle_state::paused ) {
        return atlas_upload_interrupt::paused;
    }
    if( ls == lifecycle_state::resumed_pending_rebuild
        && lifecycle_seq_of( epoch ) != planner_.serviced_lifecycle_seq() ) {



        return atlas_upload_interrupt::texture_resources_invalidated;
    }
    const renderer_recovery_severity sev = pending_severity_.load();
    if( sev == renderer_recovery_severity::device_lost
        || display_buffer_scope_recovery_pending() ) {
        return atlas_upload_interrupt::renderer_invalidated;
    }
    if( sev == renderer_recovery_severity::device_reset ) {
        return atlas_upload_interrupt::texture_resources_invalidated;
    }

    return atlas_upload_interrupt::none;
}

atlas_upload_interrupt renderer_resource_coordinator::mode2_upload_poll()
{
    if( test_mode2_interrupt_countdown_ > 0 ) {
        --test_mode2_interrupt_countdown_;
        if( test_mode2_interrupt_countdown_ == 0 ) {
            const atlas_upload_interrupt injected = test_mode2_interrupt_;
            test_mode2_interrupt_ = atlas_upload_interrupt::none;


            switch( injected ) {
                case atlas_upload_interrupt::paused:
                    notify_lifecycle( lifecycle_state::paused );
                    break;
                case atlas_upload_interrupt::texture_resources_invalidated:
                    request_recovery( renderer_recovery_severity::device_reset );
                    break;
                case atlas_upload_interrupt::renderer_invalidated:
                case atlas_upload_interrupt::shader_boundary_lost:
                    request_recovery( renderer_recovery_severity::device_lost );
                    break;
                case atlas_upload_interrupt::none:
                    break;
            }
            return injected;
        }
    }
    const uint64_t epoch = lifecycle_epoch_.load();
    const lifecycle_state ls = lifecycle_state_of( epoch );
    if( ls == lifecycle_state::paused ) {
        return atlas_upload_interrupt::paused;
    }
    if( ls == lifecycle_state::resumed_pending_rebuild ) {
        return atlas_upload_interrupt::texture_resources_invalidated;
    }
    const renderer_recovery_severity sev = pending_severity_.load();
    if( sev == renderer_recovery_severity::device_lost
        || display_buffer_scope_recovery_pending() ) {
        return atlas_upload_interrupt::renderer_invalidated;
    }
    if( sev == renderer_recovery_severity::device_reset ) {
        return atlas_upload_interrupt::texture_resources_invalidated;
    }
    return atlas_upload_interrupt::none;
}

atlas_upload_interrupt renderer_resource_coordinator::replay_live_atlases()
{
    return ts_cache.replay_live_atlases( renderer, renderer_instance_generation_,
                                         gpu_textures_generation_,
    [this]() {
        return replay_poll();
    },
    replay_quarantine_ );
}

recipe_result renderer_resource_coordinator::map_replay_interrupt(
    const atlas_upload_interrupt reason )
{
    switch( reason ) {
        case atlas_upload_interrupt::paused:
            planner_.note_pause_abort();
            return { recipe_outcome::failure };
        case atlas_upload_interrupt::texture_resources_invalidated:
            return { recipe_outcome::restart_required, renderer_recovery_severity::device_reset };
        case atlas_upload_interrupt::renderer_invalidated:
        case atlas_upload_interrupt::shader_boundary_lost:
            return { recipe_outcome::restart_required, renderer_recovery_severity::device_lost };
        case atlas_upload_interrupt::none:
            break;
    }
    return { recipe_outcome::success };
}

recipe_result renderer_resource_coordinator::setup_target_phase()
{
    if( SetupRenderTarget() ) {
        record_display_buffer_dims();
        return { recipe_outcome::success };
    }
    if( display_buffer_scope_recovery_pending() ) {
        return { recipe_outcome::restart_required, renderer_recovery_severity::device_lost };
    }
    return { recipe_outcome::failure };
}

void renderer_resource_coordinator::seed_renderer_policy( const std::string &renderer_name )
{
    renderer_name_ = renderer_name;
    software_renderer_ = IsRendererSoftware( renderer );
    record_display_buffer_dims();
}

void renderer_resource_coordinator::finish_bootstrap()
{
    planner_.finish_bootstrap();
}

bool renderer_resource_coordinator::take_repaint_request()
{

    if( !is_render_allowed() ) {
        return false;
    }
    return std::exchange( repaint_requested_, false );
}

bool renderer_resource_coordinator::lifecycle_paused() const
{
    return lifecycle_state_of( lifecycle_epoch_.load() ) == lifecycle_state::paused;
}

void renderer_resource_coordinator::begin_atlas_upload()
{
    ++atlas_upload_depth_;
}

void renderer_resource_coordinator::end_atlas_upload()
{
    cata_assert( atlas_upload_depth_ > 0 );
    --atlas_upload_depth_;
}



static bool test_fixture_acquired_video = false;
static bool test_fixture_had_prior_driver = false;
static std::string test_fixture_prior_driver;



static struct {
    int terminal_width;
    int terminal_height;
    int fontwidth;
    int fontheight;
    int scaling_factor;
    int window_width;
    int window_height;
    int drawable_width;
    int drawable_height;
    Uint32 pixel_format;
    bool direct3d_mode;
    bool needupdate;
    unsigned curses_render_epoch;
} test_fixture_saved;




static void set_videodriver_env( const char *const value )
{
    SDL_SetEnvironmentVariable( SDL_GetEnvironment(), "SDL_VIDEODRIVER", value, true );
}

static void clear_videodriver_env()
{
    SDL_UnsetEnvironmentVariable( SDL_GetEnvironment(), "SDL_VIDEODRIVER" );
}

static void restore_videodriver_env()
{
    if( test_fixture_had_prior_driver ) {
        set_videodriver_env( test_fixture_prior_driver.c_str() );
    } else {
        clear_videodriver_env();
    }
}

void renderer_recovery_test_support::reset_coordinator()
{
    renderer_resource_coordinator &c = renderer_coordinator;
    if( !c.replay_quarantine_.empty() ) {
        if( renderer ) {
            c.replay_quarantine_.drain_live_renderer();
        } else {

            c.replay_quarantine_.abandon_pre_lost_renderer();
        }
    }
    c.planner_.reset();
    c.atlas_upload_depth_ = 0;
    c.renderer_resource_generation_ = 0;
    c.repaint_requested_ = false;
    c.renderer_instance_generation_ = 0;
    c.gpu_textures_generation_ = 0;
    c.display_buffer_w_ = 0;
    c.display_buffer_h_ = 0;
    c.ui_relayout_pending_ = false;
    c.ui_relayout_count_ = 0;
    c.renderer_name_.clear();
    c.software_renderer_ = false;
    c.pending_severity_.store( renderer_recovery_severity::none );
    c.pending_resize_epoch_.store( 0 );
    c.lifecycle_epoch_.store( 0 );
    c.test_phase_action_ = renderer_resource_coordinator::test_phase_action::none;
    c.test_phase_countdown_ = 0;
    c.test_phase_fault_fired_ = false;
    c.test_replay_pause_countdown_ = 0;
    c.test_mode2_interrupt_countdown_ = 0;
    c.test_mode2_interrupt_ = atlas_upload_interrupt::none;
}

std::unique_ptr<cata_tiles> renderer_recovery_test_support::make_test_tiles(
    const std::shared_ptr<const tileset> &ts )
{
    std::unique_ptr<cata_tiles> tiles = std::make_unique<cata_tiles>( renderer, geometry, ts_cache );
    tiles->tileset_ptr = ts;
    tiles->set_draw_scale( 16 );
    return tiles;
}

void renderer_recovery_test_support::draw_test_map( cata_tiles &tiles,
        const tripoint_bub_ms &center, const int w, const int h )
{
    std::multimap<point, formatted_text> overlay_strings;
    color_block_overlay_container color_blocks;
    tiles.draw( point::zero, center, w * tiles.tile_width, h * tiles.tile_height, overlay_strings,
                color_blocks );
}

void renderer_recovery_test_support::draw_test_overmap( cata_tiles &tiles,
        const tripoint_abs_omt &center )
{

    const bool acquired_ttf = TTF_WasInit() == 0 && TTF_Init();
    Font_Ptr test_font = Font::load_font( renderer, pixel_format, PATH_INFO::fontdir() + "unifont.ttf",
                                          16, 8, 16, windowsPalette, false );
    font.swap( test_font );
    on_out_of_scope restore_font( [&test_font, acquired_ttf]() {
        font.swap( test_font );
        test_font.reset();
        if( acquired_ttf ) {
            TTF_Quit();
        }
    } );
    tiles.draw_om( point::zero, center, false );
}

void renderer_recovery_test_support::log_draw_light( cata_tiles &tiles,
        std::vector<drawn_sprite_record> *log )
{
    tiles.test_draw_light_log = log;
}

void renderer_recovery_test_support::set_has_animated_tiles( cata_tiles &tiles,
        const bool animated )
{
    tiles.has_animated_tiles_ = animated;
}

bool renderer_recovery_test_support::setup_software_renderer()
{
    if( renderer || window ) {
        return false;
    }
    cata_shader::test_reset_seams();
    cata_shader::clear_reprobe();
    const char *const prior = SDL_getenv( "SDL_VIDEODRIVER" );
    test_fixture_had_prior_driver = prior != nullptr;
    test_fixture_prior_driver = prior != nullptr ? prior : std::string();
    set_videodriver_env( "dummy" );

    test_fixture_acquired_video = !SDL_WasInit( SDL_INIT_VIDEO );
    if( test_fixture_acquired_video ) {
        const bool inited = SDL_InitSubSystem( SDL_INIT_VIDEO );
        if( !inited ) {
            test_fixture_acquired_video = false;
            restore_videodriver_env();
            return false;
        }
    }

    test_fixture_saved = {
        TERMINAL_WIDTH, TERMINAL_HEIGHT, fontwidth, fontheight, scaling_factor,
        WindowWidth, WindowHeight, DrawableWidth, DrawableHeight, pixel_format, direct3d_mode,
        needupdate, cata_cursesport::curses_render_epoch
    };

    TERMINAL_WIDTH = 8;
    TERMINAL_HEIGHT = 8;
    fontwidth = 8;
    fontheight = 8;
    scaling_factor = 1;
    WindowWidth = TERMINAL_WIDTH * fontwidth * scaling_factor;
    WindowHeight = TERMINAL_HEIGHT * fontheight * scaling_factor;

    window = CreateGameWindow( "", 0, WindowWidth, WindowHeight, CATA_WINDOW_HIDDEN );
    if( !window ) {
        teardown_software_renderer();
        return false;
    }
    renderer = create_game_renderer( {}, true );
    if( !renderer ) {
        teardown_software_renderer();
        return false;
    }
    detect_renderer_backend();
    pixel_format = SDL_PIXELFORMAT_ARGB8888;
    shared_variant_pass = std::make_unique<cata_shader::variant_pass>( renderer.get() );
    shared_lightmap = std::make_unique<smooth_lightmap>();

    apply_tile_atlas_options();
    geometry = std::make_unique<DefaultGeometryRenderer>();
    if( !SetupRenderTarget() ) {
        teardown_software_renderer();
        return false;
    }


    refresh_drawable_dims();
    renderer_coordinator.seed_renderer_policy( {} );
    renderer_coordinator.finish_bootstrap();
    return true;
}

void renderer_recovery_test_support::with_imgui_client(
    const std::vector<font_config> &gui_typefaces, const std::vector<font_config> &mono_typefaces,
    const std::function<void( cataimgui::client & )> &body )
{
    cataimgui::client client( renderer, window, geometry );
    cataimgui::add_cata_fonts( ImGui::GetIO(), gui_typefaces, mono_typefaces, false );
    body( client );
}

void renderer_recovery_test_support::teardown_software_renderer()
{


    fixture_font.reset();
    if( test_fixture_fonts_saved ) {
        font = std::move( test_fixture_saved_font );
        gui_font = std::move( test_fixture_saved_gui_font );
        terminal_glyphs_changed = test_fixture_saved_glyphs_changed;
        test_fixture_fonts_saved = false;
    }
    if( test_fixture_acquired_ttf ) {
        TTF_Quit();
        test_fixture_acquired_ttf = false;
    }
    ts_cache.release_live_atlases();



    cata_assert( display_buffer_scope_depth == 0 );
    display_buffer_scope_aborted = false;
    display_buffer_scope_invalid = false;
    display_buffer_scope_recovery_required = false;
    reset_coordinator();
    geometry.reset();
    reset_shared_lightmap();
    shared_lightmap.reset();
    shared_variant_pass.reset();
    cata_shader::test_reset_seams();
    cata_shader::clear_reprobe();
    test_shader_variants_override.reset();
    override_text_atlas( std::nullopt );
    display_buffer.reset();
    renderer.reset();
    window.reset();

    TERMINAL_WIDTH = test_fixture_saved.terminal_width;
    TERMINAL_HEIGHT = test_fixture_saved.terminal_height;
    fontwidth = test_fixture_saved.fontwidth;
    fontheight = test_fixture_saved.fontheight;
    scaling_factor = test_fixture_saved.scaling_factor;
    WindowWidth = test_fixture_saved.window_width;
    WindowHeight = test_fixture_saved.window_height;
    DrawableWidth = test_fixture_saved.drawable_width;
    DrawableHeight = test_fixture_saved.drawable_height;
    pixel_format = test_fixture_saved.pixel_format;
    direct3d_mode = test_fixture_saved.direct3d_mode;
    needupdate = test_fixture_saved.needupdate;
    test_drawable_override_w = -1;
    test_drawable_override_h = -1;
    cata_cursesport::curses_render_epoch = test_fixture_saved.curses_render_epoch;

    if( test_fixture_acquired_video ) {
        SDL_QuitSubSystem( SDL_INIT_VIDEO );
        test_fixture_acquired_video = false;
    }
    restore_videodriver_env();
}

std::shared_ptr<const tileset> renderer_recovery_test_support::install_synthetic_bundle(
    const std::string &tileset_id, const std::string &memory_map_mode,
    const uint64_t renderer_instance_generation, const uint64_t gpu_textures_generation )
{
    const std::optional<atlas_bake_plan> plan = resolve_atlas_bake_plan( memory_map_mode );
    if( !plan ) {
        renderer_coordinator.request_recovery( renderer_recovery_severity::device_lost );
        return nullptr;
    }
    return install_synthetic_bundle( tileset_id, memory_map_mode, renderer_instance_generation,
                                     gpu_textures_generation, *plan );
}

std::shared_ptr<const tileset> renderer_recovery_test_support::install_synthetic_bundle(
    const std::string &tileset_id, const std::string &memory_map_mode,
    const uint64_t renderer_instance_generation, const uint64_t gpu_textures_generation,
    const atlas_bake_plan &plan, const bool with_highlight, const std::vector<std::string> &tile_ids )
{
    std::vector<std::pair<std::string, tile_type>> tiles;
    for( const std::string &id : tile_ids ) {
        tile_type tile;
        tile.fg.add( std::vector<int> { 0 }, 1 );
        tiles.emplace_back( id, std::move( tile ) );
    }
    return install_tiles_bundle( tileset_id, memory_map_mode, renderer_instance_generation,
                                 gpu_textures_generation, plan, with_highlight, tiles );
}

std::shared_ptr<const tileset> renderer_recovery_test_support::install_tiles_bundle(
    const std::string &tileset_id, const std::string &memory_map_mode,
    const uint64_t renderer_instance_generation, const uint64_t gpu_textures_generation,
    const atlas_bake_plan &plan, const bool with_highlight,
    const std::vector<std::pair<std::string, tile_type>> &tiles )
{
    std::shared_ptr<tileset> ts = std::make_shared<tileset>();
    ts->tileset_id = tileset_id;
    for( const std::pair<std::string, tile_type> &t : tiles ) {
        ts->create_tile_type( t.first, tile_type( t.second ) );
    }
    atlas_replay_descriptor desc;
    desc.image_path_u8 = "tests/data/renderer_recovery_atlas.png";
    desc.sprite_width = 1;
    desc.sprite_height = 1;
    desc.atlas_offset = 0;
    desc.expected_tilecount = 1;
    if( with_highlight ) {

        ts->tile_width = 1;
        ts->tile_height = 1;
        ts->set_default_item_highlight_index( 1 );
    }
    return upload_test_bundle( std::move( ts ), desc, memory_map_mode, renderer_instance_generation,
                               gpu_textures_generation, plan );
}

std::shared_ptr<const tileset> renderer_recovery_test_support::install_atlas_bundle(
    const std::string &tileset_id, const std::string &image_path, const point &sprite_size,
    const int tilecount )
{
    std::shared_ptr<tileset> ts = std::make_shared<tileset>();
    ts->tileset_id = tileset_id;
    atlas_replay_descriptor desc;
    desc.image_path_u8 = image_path;
    desc.sprite_width = sprite_size.x;
    desc.sprite_height = sprite_size.y;
    desc.atlas_offset = 0;
    desc.expected_tilecount = tilecount;
    return upload_test_bundle( std::move( ts ), desc, "color_pixel_darken",
                               renderer_coordinator.instance_generation(), renderer_coordinator.textures_generation(),
                               atlas_bake_plan{} );
}

std::shared_ptr<const tileset> renderer_recovery_test_support::upload_test_bundle(
    std::shared_ptr<tileset> ts, const atlas_replay_descriptor &desc,
    const std::string &memory_map_mode, const uint64_t renderer_instance_generation,
    const uint64_t gpu_textures_generation, const atlas_bake_plan &plan )
{
    ts->append_atlas_descriptor( desc );
    ts->set_memory_map_mode_at_upload( memory_map_mode );
    tileset_cache::loader::upload_atlases( *ts, renderer, memory_map_mode,
                                           compute_tileset_filter_fingerprint( memory_map_mode ), plan,
                                           ts->get_atlas_descriptors(),
                                           renderer_instance_generation,
                                           gpu_textures_generation, false );
    ts->set_upload_generations( renderer_instance_generation, gpu_textures_generation );
    const tileset_cache_key key {
        ts->tileset_id, memory_map_mode, compute_tileset_filter_fingerprint( memory_map_mode )
    };
    ts_cache.track_bundle( key, ts );
    return ts;
}

std::shared_ptr<const tileset>
renderer_recovery_test_support::install_synthetic_bundle_with_highlight(
    const std::string &tileset_id, const std::string &memory_map_mode,
    const uint64_t renderer_instance_generation, const uint64_t gpu_textures_generation )
{
    return install_synthetic_bundle( tileset_id, memory_map_mode, renderer_instance_generation,
                                     gpu_textures_generation, atlas_bake_plan{}, true );
}

atlas_replay_quarantine::gate renderer_recovery_test_support::populate_mode2_quarantine(
    atlas_replay_quarantine &quarantine )
{
    tileset ts;
    ts.tileset_id = "synthetic_mode2_ts";
    atlas_replay_descriptor desc;
    desc.image_path_u8 = "tests/data/renderer_recovery_atlas.png";
    desc.sprite_width = 1;
    desc.sprite_height = 1;
    desc.atlas_offset = 0;
    desc.expected_tilecount = 1;
    ts.append_atlas_descriptor( desc );



    int polls = 0;
    const atlas_upload_poll poll = [&polls]() {
        return ++polls >= 3 ? atlas_upload_interrupt::texture_resources_invalidated
               : atlas_upload_interrupt::none;
    };
    tileset_cache::loader::upload_atlases( ts, renderer, "color_pixel_sepia_light",
                                           compute_tileset_filter_fingerprint( "color_pixel_sepia_light" ),
                                           atlas_bake_plan{}, ts.get_atlas_descriptors(),
                                           renderer_coordinator.instance_generation(),
                                           renderer_coordinator.textures_generation(),
                                           false, poll, &quarantine );
    cata_assert( !quarantine.empty() );
    return quarantine.last_batch_gate();
}

std::shared_ptr<const tileset> renderer_recovery_test_support::fetch_cached_bundle(
    const std::string &tileset_id, const std::string &memory_map_mode,
    const uint64_t current_renderer_instance_gen, const uint64_t current_gpu_textures_gen )
{
    return ts_cache.load_tileset( tileset_id, renderer, false, false, false, false,
                                  memory_map_mode, current_renderer_instance_gen,
                                  current_gpu_textures_gen );
}

bool renderer_recovery_test_support::cache_lookup_is_fresh(
    const std::string &tileset_id, const std::string &memory_map_mode,
    const uint64_t current_renderer_instance_gen, const uint64_t current_gpu_textures_gen )
{
    const tileset_cache_key key {
        tileset_id, memory_map_mode, compute_tileset_filter_fingerprint( memory_map_mode )
    };
    return ts_cache.find_fresh_cached( key, current_renderer_instance_gen,
                                       current_gpu_textures_gen ) != nullptr;
}

void renderer_recovery_test_support::arm_phase_fail_retry( const int phase_countdown )
{
    cata_assert( phase_countdown > 0 );
    renderer_coordinator.test_phase_action_ =
        renderer_resource_coordinator::test_phase_action::fail_retry;
    renderer_coordinator.test_phase_countdown_ = phase_countdown;
    renderer_coordinator.test_phase_fault_fired_ = false;
}

void renderer_recovery_test_support::arm_phase_queue_device_reset( const int phase_countdown )
{
    cata_assert( phase_countdown > 0 );
    renderer_coordinator.test_phase_action_ =
        renderer_resource_coordinator::test_phase_action::queue_device_reset;
    renderer_coordinator.test_phase_countdown_ = phase_countdown;
    renderer_coordinator.test_phase_fault_fired_ = false;
}

void renderer_recovery_test_support::arm_replay_pause( const int poll_countdown )
{
    cata_assert( poll_countdown > 0 );
    renderer_coordinator.test_replay_pause_countdown_ = poll_countdown;
}

void renderer_recovery_test_support::arm_mode2_interrupt( const int poll_countdown,
        const atlas_upload_interrupt interrupt )
{
    cata_assert( poll_countdown > 0 );
    renderer_coordinator.test_mode2_interrupt_countdown_ = poll_countdown;
    renderer_coordinator.test_mode2_interrupt_ = interrupt;
}

void renderer_recovery_test_support::arm_probe_unsafe( const int count )
{
    cata_assert( count > 0 );
    cata_shader::test_arm_probe_unsafe( count );
}

int renderer_recovery_test_support::probe_unsafe_remaining()
{
    return cata_shader::test_probe_unsafe_remaining();
}

void renderer_recovery_test_support::arm_flush_failure()
{
    cata_shader::test_arm_flush_failure();
}

void renderer_recovery_test_support::mark_shader_fault()
{
    cata_assert( shared_variant_pass );
    shared_variant_pass->shader_fault_ = true;
}

void renderer_recovery_test_support::simulate_draw_bind_failure()
{
    cata_assert( shared_variant_pass );
    shared_variant_pass->note_draw_bind_failure( false );
    display_buffer_scope_signal_recovery_required();
}

int renderer_recovery_test_support::variant_probe_count()
{
    return cata_shader::test_probe_runs();
}

bool renderer_recovery_test_support::run_present_gate()
{
    return present_gate();
}

void renderer_recovery_test_support::override_shader_variants_available(
    const std::optional<bool> available )
{
    test_shader_variants_override = available;
}

bool renderer_recovery_test_support::replay_quarantine_empty()
{
    return renderer_coordinator.replay_quarantine_.empty();
}

bool renderer_recovery_test_support::phase_fault_fired()
{
    return renderer_coordinator.test_phase_fault_fired_;
}

uint64_t renderer_recovery_test_support::ui_relayout_count()
{
    return renderer_coordinator.ui_relayout_count_;
}

void renderer_recovery_test_support::set_scaling_and_resize_window( const int scaling,
        const int window_w, const int window_h )
{
    scaling_factor = scaling;
    SetWindowSize( ::window.get(), window_w, window_h );
    renderer_coordinator.notify_resize();
}

void renderer_recovery_test_support::current_display_buffer_dims( int &w, int &h )
{
    get_display_buffer_dims( &w, &h );
}

void renderer_recovery_test_support::current_window_metrics( int &window_w, int &window_h,
        int &font_w, int &font_h, int &scaling, int &min_term_w, int &min_term_h )
{
    window_w = WindowWidth;
    window_h = WindowHeight;
    font_w = fontwidth;
    font_h = fontheight;
    scaling = scaling_factor;
    min_term_w = EVEN_MINIMUM_TERM_WIDTH;
    min_term_h = EVEN_MINIMUM_TERM_HEIGHT;
}

void renderer_recovery_test_support::current_drawable_dims( int &w, int &h )
{
    w = DrawableWidth;
    h = DrawableHeight;
}

void renderer_recovery_test_support::override_drawable_pixels( const int w, const int h )
{
    test_drawable_override_w = w;
    test_drawable_override_h = h;
    renderer_coordinator.notify_resize();
}

void renderer_recovery_test_support::set_needupdate( const bool armed )
{
    needupdate = armed;
}

bool renderer_recovery_test_support::needupdate_armed()
{
    return needupdate;
}

void renderer_recovery_test_support::pause_during_draw_scope( bool &bound_during,
        bool &null_after )
{
    {
        display_buffer_draw_scope scope;
        bound_during = !display_buffer_scope_is_invalid()
                       && GetRenderTarget( renderer ) == display_buffer.get();



        renderer_coordinator.notify_lifecycle( lifecycle_state::paused );
    }
    null_after = GetRenderTarget( renderer ) == nullptr;
}

bool renderer_resource_coordinator::apply_resize_only( const uint32_t serviced_resize_epoch )
{


    if( check_pause_abort() ) {
        return false;
    }


    int logical_w = 0;
    int logical_h = 0;
    GetWindowSize( ::window.get(), &logical_w, &logical_h );
    if( apply_resize_layout( logical_w, logical_h ) ) {
        ui_relayout_pending_ = true;
    }
    const point want = compute_display_buffer_dims();
    const bool buffer_changed = !display_buffer || want.x != display_buffer_w_
                                || want.y != display_buffer_h_;
    if( buffer_changed ) {
        const bind_result r = permanent_render_target_bind( renderer, nullptr,
                              shared_variant_pass_or_null() );
        if( r == bind_result::failed_in_switch ) {


            request_recovery( renderer_recovery_severity::device_lost );
            return false;
        }
        if( check_pause_abort() ) {
            return false;
        }
        display_buffer.reset();
        if( check_pause_abort() ) {
            return false;
        }
        if( !SetupRenderTarget() ) {
            if( display_buffer_scope_recovery_pending() ) {
                request_recovery( renderer_recovery_severity::device_lost );
            }
            return false;
        }
        record_display_buffer_dims();
        ++renderer_resource_generation_;
        display_buffer_scope_clear_recovery_required();
    }



    planner_.acknowledge_resize( serviced_resize_epoch );
    if( ui_relayout_pending_ ) {
        ui_relayout_pending_ = false;
        ++ui_relayout_count_;
        if( !test_mode ) {



            game_ui::init_ui();
            ui_manager::screen_resized();
        }
    }

    needupdate = true;
    const bool glyphs_changed = std::exchange( terminal_glyphs_changed, false );
    if( buffer_changed || glyphs_changed ) {
        run_post_success_full_invalidation();
    }
    return true;
}

void renderer_resource_coordinator::attempt_serviced_seq_cas( const uint64_t serviced_seq )
{



    uint64_t latest = lifecycle_epoch_.load();
    if( lifecycle_seq_of( latest ) == serviced_seq
        && lifecycle_state_of( latest ) == lifecycle_state::resumed_pending_rebuild ) {
        const uint64_t desired = make_lifecycle_epoch( serviced_seq, lifecycle_state::active );
        lifecycle_epoch_.compare_exchange_strong( latest, desired );
    }
}

void renderer_resource_coordinator::drain_pending()
{



    if( planner_.state() == renderer_recovery_state::bootstrapping
        || planner_.state() == renderer_recovery_state::recovering
        || atlas_upload_depth_ > 0 ) {
        return;
    }



    using action_kind = recovery_drain_planner::action_kind;
    recovery_drain_planner::action a = planner_.begin();
    for( ;; ) {
        switch( a.kind ) {
            case action_kind::load_entry_epoch:
                a = planner_.advance_entry_epoch( lifecycle_epoch_.load() );
                break;
            case action_kind::claim_initial_inbox: {
                const renderer_recovery_severity pulled =
                    pending_severity_.exchange( renderer_recovery_severity::none );
                const bool boundary = display_buffer_scope_recovery_pending();
                const uint32_t resize_epoch = pending_resize_epoch_.load();
                a = planner_.advance_initial_inbox( pulled, resize_epoch, boundary );
                break;
            }
            case action_kind::run_recipe:
                a = planner_.advance_recipe( run_recipe_for( a.severity ) );
                break;
            case action_kind::attempt_seq_cas:
                attempt_serviced_seq_cas( a.serviced_seq );
                a = planner_.advance_seq_cas_done();
                break;
            case action_kind::claim_after_success: {
                const renderer_recovery_severity pulled =
                    pending_severity_.exchange( renderer_recovery_severity::none );
                const uint64_t live = lifecycle_epoch_.load();
                a = planner_.advance_after_success( pulled, live );
                break;
            }
            case action_kind::claim_after_restart: {
                const uint64_t live = lifecycle_epoch_.load();
                const renderer_recovery_severity pulled =
                    pending_severity_.exchange( renderer_recovery_severity::none );
                a = planner_.advance_after_restart( live, pulled );
                break;
            }
            case action_kind::load_post_resize:
                a = planner_.advance_post_resize( pending_resize_epoch_.load() );
                break;
            case action_kind::apply_resize:
                a = planner_.advance_resize( apply_resize_only( a.resize_epoch ) );
                break;
            case action_kind::finish_ready:
            case action_kind::finish_retry:
                return;
        }
    }
}

recovery_drain_planner::action recovery_drain_planner::begin()
{
    state_ = renderer_recovery_state::recovering;
    abort_latch_ = true;
    current_claimed_ = renderer_recovery_severity::none;
    entry_epoch_implied_ = renderer_recovery_severity::none;
    restart_severity_pending_ = renderer_recovery_severity::none;
    recipe_pause_aborted_ = false;
    resize_fast_path_ = false;
    return { action_kind::load_entry_epoch };
}

recovery_drain_planner::action recovery_drain_planner::advance_entry_epoch(
    const uint64_t entry_epoch )
{
    const lifecycle_state entry_state = lifecycle_state_of( entry_epoch );
    if( entry_state == lifecycle_state::paused ) {


        return finish_ready_action();
    }


    entry_epoch_implied_ = ( entry_state == lifecycle_state::resumed_pending_rebuild )
                           ? renderer_recovery_severity::device_reset
                           : renderer_recovery_severity::none;
    serviced_lifecycle_seq_ = lifecycle_seq_of( entry_epoch );
    return { action_kind::claim_initial_inbox };
}

recovery_drain_planner::action recovery_drain_planner::advance_initial_inbox(
    const renderer_recovery_severity pulled, const uint32_t resize_epoch,
    const bool boundary_recovery_pending )
{
    renderer_recovery_severity claimed = severity_max( merge_retry( pulled ), entry_epoch_implied_ );
    if( boundary_recovery_pending ) {


        claimed = severity_max( claimed, renderer_recovery_severity::device_lost );
    }
    if( claimed == renderer_recovery_severity::none ) {


        if( resize_epoch != observed_resize_epoch_ ) {
            return emit_apply_resize( resize_epoch, true );
        }
        return { action_kind::load_post_resize };
    }
    return emit_run_recipe( claimed );
}

recovery_drain_planner::action recovery_drain_planner::advance_recipe( const recipe_result r )
{
    if( recipe_pause_aborted_ ) {

        return finish_retry_action();
    }
    if( r.outcome == recipe_outcome::failure ) {
        retry_severity_ = current_claimed_;
        return finish_retry_action();
    }
    if( r.outcome == recipe_outcome::restart_required ) {
        restart_severity_pending_ = r.restart_severity;
        return { action_kind::claim_after_restart };
    }
    return { action_kind::attempt_seq_cas, renderer_recovery_severity::none, 0,
             serviced_lifecycle_seq_ };
}

recovery_drain_planner::action recovery_drain_planner::advance_seq_cas_done()
{
    return { action_kind::claim_after_success };
}

recovery_drain_planner::action recovery_drain_planner::advance_after_success(
    const renderer_recovery_severity pulled, const uint64_t live_epoch )
{
    const renderer_recovery_severity new_work = merge_retry( pulled );
    renderer_recovery_severity next_epoch_implied = renderer_recovery_severity::none;
    if( lifecycle_state_of( live_epoch ) == lifecycle_state::resumed_pending_rebuild
        && lifecycle_seq_of( live_epoch ) != serviced_lifecycle_seq_ ) {

        next_epoch_implied = renderer_recovery_severity::device_reset;
        serviced_lifecycle_seq_ = lifecycle_seq_of( live_epoch );
    }
    const renderer_recovery_severity claimed = severity_max( new_work, next_epoch_implied );
    if( claimed == renderer_recovery_severity::none ) {
        return { action_kind::load_post_resize };
    }
    return emit_run_recipe( claimed );
}

recovery_drain_planner::action recovery_drain_planner::advance_after_restart(
    const uint64_t live_epoch, const renderer_recovery_severity pulled )
{
    if( lifecycle_state_of( live_epoch ) == lifecycle_state::resumed_pending_rebuild ) {



        serviced_lifecycle_seq_ = lifecycle_seq_of( live_epoch );
    }
    const renderer_recovery_severity claimed =
        severity_max( merge_retry( pulled ), restart_severity_pending_ );
    if( claimed == renderer_recovery_severity::none ) {
        return { action_kind::load_post_resize };
    }
    return emit_run_recipe( claimed );
}

recovery_drain_planner::action recovery_drain_planner::advance_post_resize(
    const uint32_t resize_epoch )
{
    if( resize_epoch != observed_resize_epoch_ ) {
        return emit_apply_resize( resize_epoch, false );
    }
    return finish_ready_action();
}

recovery_drain_planner::action recovery_drain_planner::advance_resize( const bool applied )
{
    if( applied ) {
        return finish_ready_action();
    }
    if( resize_fast_path_ ) {


        retry_severity_ = renderer_recovery_severity::none;
    }
    return finish_retry_action();
}

void recovery_drain_planner::note_pause_abort()
{
    retry_severity_ = severity_max( retry_severity_, current_claimed_ );
    state_ = renderer_recovery_state::retry_needed;
    recipe_pause_aborted_ = true;
}

renderer_recovery_severity recovery_drain_planner::merge_retry( const renderer_recovery_severity
        pulled )
{
    const renderer_recovery_severity claimed = severity_max( pulled, retry_severity_ );
    retry_severity_ = renderer_recovery_severity::none;
    return claimed;
}

recovery_drain_planner::action recovery_drain_planner::emit_run_recipe(
    const renderer_recovery_severity sev )
{
    current_claimed_ = sev;
    return { action_kind::run_recipe, sev };
}

recovery_drain_planner::action recovery_drain_planner::emit_apply_resize(
    const uint32_t resize_epoch, const bool fast_path )
{


    current_claimed_ = renderer_recovery_severity::none;
    resize_fast_path_ = fast_path;
    return { action_kind::apply_resize, renderer_recovery_severity::none, resize_epoch };
}

recovery_drain_planner::action recovery_drain_planner::finish_ready_action()
{
    state_ = renderer_recovery_state::ready;
    abort_latch_ = false;
    return { action_kind::finish_ready };
}

recovery_drain_planner::action recovery_drain_planner::finish_retry_action()
{
    state_ = renderer_recovery_state::retry_needed;
    return { action_kind::finish_retry };
}

static std::optional<std::pair<tripoint_abs_omt, std::string>> get_mission_arrow(
            const inclusive_cuboid<tripoint_abs_omt> &overmap_area, const tripoint_abs_omt &center )
{
    const tripoint_abs_omt mission_target = get_avatar().get_active_mission_target();

    if( mission_target == tripoint_abs_omt::invalid ) {
        return std::nullopt;
    }

    std::string mission_arrow_variant;
    if( overmap_area.contains( mission_target ) ) {
        mission_arrow_variant = "mission_cursor";
        return std::make_pair( mission_target, mission_arrow_variant );
    }

    inclusive_rectangle<point_abs_omt> area_flat( overmap_area.p_min.xy(), overmap_area.p_max.xy() );
    if( area_flat.contains( mission_target.xy() ) ) {
        int area_z = center.z();
        if( mission_target.z() > area_z ) {
            mission_arrow_variant = "mission_arrow_up";
        } else {
            mission_arrow_variant = "mission_arrow_down";
        }
        return std::make_pair( tripoint_abs_omt( mission_target.xy(), area_z ), mission_arrow_variant );
    }

    const std::vector<tripoint_abs_omt> traj = line_to( center,
            tripoint_abs_omt( mission_target.xy(), center.z() ) );

    if( traj.empty() ) {
        debugmsg( "Failed to gen overmap mission trajectory %s %s",
                  center.to_string(), mission_target.to_string() );
        return std::nullopt;
    }

    tripoint_abs_omt arr_pos = traj[0];
    for( auto it = traj.rbegin(); it != traj.rend(); it++ ) {
        if( overmap_area.contains( *it ) ) {
            arr_pos = *it;
            break;
        }
    }

    const int north_border_y = ( overmap_area.p_max.y() - overmap_area.p_min.y() ) / 3;
    const int south_border_y = north_border_y * 2;
    const int west_border_x = ( overmap_area.p_max.x() - overmap_area.p_min.x() ) / 3;
    const int east_border_x = west_border_x * 2;

    tripoint_abs_omt north_pmax( overmap_area.p_max );
    north_pmax.y() = overmap_area.p_min.y() + north_border_y;
    tripoint_abs_omt south_pmin( overmap_area.p_min );
    south_pmin.y() += south_border_y;
    tripoint_abs_omt west_pmax( overmap_area.p_max );
    west_pmax.x() = overmap_area.p_min.x() + west_border_x;
    tripoint_abs_omt east_pmin( overmap_area.p_min );
    east_pmin.x() += east_border_x;

    const inclusive_cuboid<tripoint_abs_omt> north_sector( overmap_area.p_min, north_pmax );
    const inclusive_cuboid<tripoint_abs_omt> south_sector( south_pmin, overmap_area.p_max );
    const inclusive_cuboid<tripoint_abs_omt> west_sector( overmap_area.p_min, west_pmax );
    const inclusive_cuboid<tripoint_abs_omt> east_sector( east_pmin, overmap_area.p_max );

    mission_arrow_variant = "mission_arrow_";
    if( north_sector.contains( arr_pos ) ) {
        mission_arrow_variant += 'n';
    } else if( south_sector.contains( arr_pos ) ) {
        mission_arrow_variant += 's';
    }
    if( west_sector.contains( arr_pos ) ) {
        mission_arrow_variant += 'w';
    } else if( east_sector.contains( arr_pos ) ) {
        mission_arrow_variant += 'e';
    }

    return std::make_pair( tripoint_abs_omt( arr_pos ), mission_arrow_variant );
}

std::pair<std::string, bool> cata_tiles::get_omt_id_rotation_and_subtile(
    const tripoint_abs_omt &omp, int &rota, int &subtile )
{
    auto oter_at = []( const tripoint_abs_omt & p ) {
        oter_id cur_ter = overmap_buffer.ter( p );
        om_vision_level vision = overmap_buffer.seen( p );
        if( cur_ter->blends_adjacent( vision ) ) {
            oter_vision::blended_omt info = oter_vision::get_blended_omt_info( p, vision );
            cur_ter = info.id;
        }

        if( !uistate.overmap_show_forest_trails &&
            ( cur_ter->get_type_id() == oter_type_forest_trail ) ) {
            return oter_id( "forest" );
        }

        return cur_ter;
    };

    oter_id ot_id = oter_at( omp );
    const oter_t &ot = *ot_id;
    oter_type_id ot_type_id = ot.get_type_id();
    const oter_type_t &ot_type = *ot_type_id;


    const std::array<oter_type_id, 4> neighborhood = {
        oter_at( omp + point::south )->get_type_id(),
        oter_at( omp + point::east )->get_type_id(),
        oter_at( omp + point::west )->get_type_id(),
        oter_at( omp + point::north )->get_type_id()
    };

    if( ot_type.has_connections() ) {

        char val = 0;


        for( int i = 0; i < 4; ++i ) {
            if( ot_type.connects_to( neighborhood[i] ) ) {
                val += 1 << i;
            }
        }

        get_rotation_and_subtile( val, -1, rota, subtile );
    } else if( ot_type.has_flag( oter_flags::water ) ) {

        char val = 0;


        for( int i = 0; i < 4; ++i ) {
            if( neighborhood[i]->has_flag( oter_flags::water ) ) {
                val += 1 << i;
            }
        }

        get_rotation_and_subtile( val, -1, rota, subtile );
    } else {


        ot.get_rotation_and_subtile( rota, subtile );
    }

    om_vision_level vision = overmap_buffer.seen( omp );
    std::string id = ot_id->get_tileset_id( vision );
    bool is_omt = id.substr( 0, 3 ) == "om#";
    return std::make_pair( id.substr( 3 ), is_omt );
}

static point draw_string( Font &font,
                          const SDL_Renderer_Ptr &renderer,
                          const std::string &str,
                          point p,
                          const unsigned char color )
{
    const char *cstr = str.c_str();
    int len = str.length();
    curses_text_batch.begin_pass( renderer, !text_atlas_enabled( renderer ) );
    while( len > 0 ) {
        const uint32_t ch32 = UTF8_getch( &cstr, &len );
        const std::string ch = utf32_to_utf8( ch32 );
        font.queue_char( curses_text_batch, renderer, ch, p, color );
        p.x += mk_wcwidth( ch32 ) * font.width;
    }

    curses_text_batch.flush( renderer );
    return p;
}

void cata_tiles::draw_om( const point &dest, const tripoint_abs_omt &center_abs_omt, bool blink )
{
    display_buffer_draw_scope draw_scope;
    if( display_buffer_scope_is_invalid() || !g ) {
        return;
    }
    has_animated_tiles_ = false;

#if defined(__ANDROID__)


    if( tile_width == 0 || tile_height == 0 ) {
        return;
    }
#endif

    int width = OVERMAP_WINDOW_TERM_WIDTH * font->width;
    int height = OVERMAP_WINDOW_TERM_HEIGHT * font->height;

    {

        SDL_Rect clipRect = { dest.x, dest.y, width, height };
        RenderSetClipRect( renderer, &clipRect );


        geometry->rect( renderer, clipRect, SDL_Color() );
    }

    const point s = get_window_base_tile_counts( { width, height } );
    const half_open_rectangle<point> full_base_range = get_window_full_base_tile_range( { width, height } );

    op = point( dest.x * fontwidth, dest.y * fontheight );
    screentile_width = s.x;
    screentile_height = s.y;

    const half_open_rectangle<point> any_tile_range = get_window_any_tile_range( { width, height }, 0 );
    const int min_col = any_tile_range.p_min.x;
    const int max_col = any_tile_range.p_max.x;
    const int min_row = any_tile_range.p_min.y;
    const int max_row = any_tile_range.p_max.y;
    int height_3d = 0;
    avatar &you = get_avatar();
    const tripoint_abs_omt avatar_pos = you.pos_abs_omt();
    tripoint_abs_omt center_pos = center_abs_omt;
    const bool overmap_only_auto_travel = g->overmap_data.overmap_only_auto_travel;
    if( overmap_only_auto_travel ) {
        center_pos = you.pos_abs_omt();
    }
    const tripoint_abs_omt origin = center_pos - point( s.x / 2, s.y / 2 );
    const tripoint_abs_omt corner_NW = origin + any_tile_range.p_min;
    const tripoint_abs_omt corner_SE = origin + any_tile_range.p_max + point::north_west;
    const inclusive_cuboid<tripoint_abs_omt> overmap_area( corner_NW, corner_SE );

    const tripoint_abs_omt full_corner_NW = origin + full_base_range.p_min;
    const tripoint_abs_omt full_corner_SE = origin + full_base_range.p_max + point::north_west;
    const inclusive_cuboid<tripoint_abs_omt> full_om_tile_area( full_corner_NW, full_corner_SE );

    const bool has_debug_vision = you.has_trait( trait_DEBUG_NIGHTVISION );

    const int sight_points = !has_debug_vision ?
                             you.overmap_modified_sight_range( g->light_level( you.posz() ) ) :
                             100;
    const bool showhordes = uistate.overmap_show_hordes;
    const bool show_map_revealed = uistate.overmap_show_revealed_omts;
    std::unordered_set<tripoint_abs_omt> &revealed_highlights = get_avatar().map_revealed_omts;
    const bool viewing_weather = uistate.overmap_debug_weather || uistate.overmap_visible_weather;
    const bool draw_overlays = blink || overmap_only_auto_travel;
    o = origin.xy().raw();

    const auto global_omt_to_draw_position = []( const tripoint_abs_omt & omp ) {

        return tripoint_abs_omt( omp.xy(), 0 );
    };

    for( int row = min_row; row < max_row; row++ ) {
        if( renderer_should_abort_frame() ) {



            break;
        }
        for( int col = min_col; col < max_col; col++ ) {
            const tripoint_abs_omt omp = origin + point( col, row );

            const om_vision_level vision = overmap_buffer.seen( omp );
            const bool los = overmap_buffer.seen_more_than( omp, om_vision_level::details ) &&
                             ( you.overmap_los( omp, sight_points ) || uistate.overmap_debug_mongroup ||
                               you.has_trait( trait_DEBUG_CLAIRVOYANCE ) );

            std::string id;
            TILE_CATEGORY category = TILE_CATEGORY::OVERMAP_TERRAIN;
            int rotation = 0;
            int subtile = -1;
            map_extra_id mx;

            if( viewing_weather ) {
                const tripoint_abs_omt omp_sky( omp.xy(), OVERMAP_HEIGHT );
                if( uistate.overmap_debug_weather ||
                    you.overmap_los( omp_sky, sight_points * 2 ) ) {
                    id = overmap_ui::get_weather_at_point( omp_sky ).c_str();
                    category = TILE_CATEGORY::OVERMAP_WEATHER;
                } else {
                    id = "unexplored_terrain";
                }
            } else if( vision == om_vision_level::unseen ) {
                id = "unknown_terrain";
            } else {
                bool is_omt = false;
                std::tie( id, is_omt ) = get_omt_id_rotation_and_subtile( omp, rotation, subtile );
                mx = overmap_buffer.extra( omp );
                if( !is_omt ) {
                    category = TILE_CATEGORY::OVERMAP_VISION_LEVEL;
                }
            }

            const lit_level ll = overmap_buffer.is_explored( omp ) ? lit_level::LOW : lit_level::LIT;

            draw_from_id_string( id, category,
                                 category == TILE_CATEGORY::OVERMAP_TERRAIN ? "overmap_terrain" : "",
                                 omp, subtile, rotation, ll, false, height_3d );
            if( !mx.is_empty() && mx->visibility != map_extra_visibility::none ) {
                draw_from_id_string( mx.str(), TILE_CATEGORY::MAP_EXTRA, "map_extra", omp,
                                     0, 0, ll, false );
            }

            if( draw_overlays && show_map_revealed ) {
                auto it = revealed_highlights.find( omp );
                if( it != revealed_highlights.end() ) {
                    draw_from_id_string( "highlight", omp, 0, 0, lit_level::LIT, false );
                }
            }

            if( vision != om_vision_level::unseen ) {
                if( draw_overlays && uistate.overmap_debug_mongroup ) {
                    std::vector<std::unordered_map<tripoint_abs_ms, horde_entity>*> hordes = overmap_buffer.hordes_at(
                                omp );
                    if( !hordes.empty() ) {
                        draw_from_id_string( "mon_zombie", omp, 0, 0, lit_level::LIT, false );
                    }
                }
                if( showhordes && los ) {
                    const int horde_size = overmap_buffer.get_horde_size( omp,
                                           horde_map_flavors::active | horde_map_flavors::idle );
                    if( horde_size >= HORDE_VISIBILITY_SIZE ) {


                        int sprite_size = 1;
                        if( horde_size < 5 ) {
                            sprite_size = 1;
                        } else if( horde_size < 13 ) {
                            sprite_size = 2;
                        } else if( horde_size < 27 ) {
                            sprite_size = 3;
                        } else if( horde_size < 47 ) {
                            sprite_size = 4;
                        } else if( horde_size < 73 ) {
                            sprite_size = 5;
                        } else if( horde_size < 106 ) {
                            sprite_size = 6;
                        } else if( horde_size < 147 ) {
                            sprite_size = 7;
                        } else if( horde_size < 195 ) {
                            sprite_size = 8;
                        } else if( horde_size < 251 ) {
                            sprite_size = 9;
                        } else {
                            sprite_size = 10;
                        }

                        if( find_tile_with_season( id ) ) {
                            // NOLINTNEXTLINE(cata-translate-string-literal)
                            draw_from_id_string( string_format( "overmap_horde_%d", sprite_size ),
                                                 omp, 0, 0, lit_level::LIT, false );
                        } else {


                            switch( sprite_size ) {
                                case 1:
                                    draw_from_id_string( "mon_zombie", omp, 0, 0, lit_level::LIT,
                                                         false );
                                    break;
                                case 2:
                                    draw_from_id_string( "mon_zombie_tough", omp, 0, 0,
                                                         lit_level::LIT, false );
                                    break;
                                case 3:
                                    draw_from_id_string( "mon_zombie_brute", omp, 0, 0,
                                                         lit_level::LIT, false );
                                    break;
                                case 4:
                                    draw_from_id_string( "mon_zombie_hulk", omp, 0, 0,
                                                         lit_level::LIT, false );
                                    break;
                                case 5:
                                    draw_from_id_string( "mon_zombie_necro", omp, 0, 0,
                                                         lit_level::LIT, false );
                                    break;
                                case 6:
                                case 7:
                                case 8:
                                case 9:
                                case 10:
                                    draw_from_id_string( "mon_zombie_master", omp, 0, 0,
                                                         lit_level::LIT, false );
                            };
                        }
                    }
                }
            }
            if( ( uistate.place_terrain || uistate.place_special ) &&
                overmap_ui::is_generated_omt( omp.xy() ) ) {

                draw_from_id_string( "highlight", omp, 0, 0, lit_level::LIT, false );
            }

            if( draw_overlays && overmap_buffer.has_vehicle( omp ) ) {
                const std::string tile_id = overmap_buffer.get_vehicle_tile_id( omp );
                if( find_tile_looks_like( tile_id, TILE_CATEGORY::OVERMAP_NOTE, "" ) ) {
                    draw_from_id_string( tile_id, TILE_CATEGORY::OVERMAP_NOTE,
                                         "overmap_note", omp, 0, 0, lit_level::LIT, false );
                } else {
                    const std::string ter_sym = overmap_buffer.get_vehicle_ter_sym( omp );
                    std::string note_name = "note_" + ter_sym + "_cyan";
                    draw_from_id_string( note_name, TILE_CATEGORY::OVERMAP_NOTE,
                                         "overmap_note", omp, 0, 0, lit_level::LIT, false );
                }
            }

            if( draw_overlays && uistate.overmap_show_map_notes ) {
                if( overmap_buffer.has_note( omp ) ) {
                    nc_color ter_color = c_black;
                    std::string ter_sym = " ";

                    std::tie( ter_sym, ter_color, std::ignore ) =
                        overmap_ui::get_note_display_info( overmap_buffer.note( omp ) );

                    std::string note_name = "note_" + ter_sym + "_" + string_from_color( ter_color );
                    draw_from_id_string( note_name, TILE_CATEGORY::OVERMAP_NOTE, "overmap_note",
                                         omp, 0, 0, lit_level::LIT, false );
                } else if( overmap_buffer.is_marked_dangerous( omp ) ) {
                    draw_from_id_string( "note_X_red", TILE_CATEGORY::OVERMAP_NOTE, "overmap_note",
                                         omp, 0, 0, lit_level::LIT, false );

                }
            }
        }
    }

    if( uistate.place_terrain ) {
        const oter_str_id &terrain_id = uistate.place_terrain->id;
        const oter_t &terrain = *terrain_id;
        std::string id = terrain.get_type_id().str();
        int rotation;
        int subtile;
        terrain.get_rotation_and_subtile( rotation, subtile );
        draw_from_id_string( id, global_omt_to_draw_position( center_pos ), subtile, rotation,
                             lit_level::LOW, true );
    }
    if( uistate.place_special ) {
        for( const overmap_special_terrain &s_ter : uistate.place_special->preview_terrains() ) {
            if( s_ter.p.z() == 0 ) {
                const point_rel_omt rp( om_direction::rotate( s_ter.p.xy(), uistate.omedit_rotation ) );
                oter_id rotated_id = s_ter.terrain->get_rotated( uistate.omedit_rotation );
                const oter_t &terrain = *rotated_id;
                std::string id = terrain.get_type_id().str();
                int rotation;
                int subtile;
                terrain.get_rotation_and_subtile( rotation, subtile );

                draw_from_id_string( id, TILE_CATEGORY::OVERMAP_TERRAIN, "overmap_terrain",
                                     global_omt_to_draw_position( center_pos + rp ), 0,
                                     rotation, lit_level::LOW, true );
            }
        }
    }

    auto npcs_near_player = overmap_buffer.get_npcs_near_player( sight_points );


    for( const shared_ptr_fast<npc> &guy : npcs_near_player ) {
        const tripoint_abs_omt &guy_loc = guy->pos_abs_omt();
        if( guy_loc.z() == center_pos.z() &&
            ( has_debug_vision || overmap_buffer.seen_more_than( guy_loc, om_vision_level::details ) ) &&
            !guy->guaranteed_hostile() ) {
            draw_entity_with_overlays( *guy, global_omt_to_draw_position( guy_loc ),
                                       lit_level::LIT,
                                       height_3d );
        }
    }

    draw_entity_with_overlays( get_player_character(),
                               global_omt_to_draw_position( avatar_pos ),
                               lit_level::LIT, height_3d );
    if( !overmap_only_auto_travel ) {
        draw_from_id_string( "cursor", global_omt_to_draw_position( center_pos ), 0, 0, lit_level::LIT,
                             false );
    }

    if( draw_overlays ) {

        for( const tripoint_abs_omt &pos : you.omt_path ) {
            if( pos.z() == center_pos.z() ) {
                draw_from_id_string( "highlight", global_omt_to_draw_position( pos ), 0, 0, lit_level::LIT,
                                     false );
            }
        }

        if( g->follower_path_to_show ) {
            for( const tripoint_abs_omt &pos : g->follower_path_to_show->omt_path ) {
                if( pos.z() == center_pos.z() ) {
                    draw_from_id_string( "highlight", global_omt_to_draw_position( pos ), 0, 0, lit_level::LIT,
                                         false );
                }
            }
        }


        const std::optional<std::pair<tripoint_abs_omt, std::string>> mission_arrow =
                    get_mission_arrow( full_om_tile_area, center_pos );
        if( mission_arrow ) {
            draw_from_id_string( mission_arrow->second, global_omt_to_draw_position( mission_arrow->first ), 0,
                                 0, lit_level::LIT, false );
        }
    }

    if( !viewing_weather && uistate.overmap_show_city_labels ) {

        const auto abs_sm_to_draw_label = [&]( const tripoint_abs_sm & city_pos, const int label_length ) {
            const point_abs_omt omt_pos = global_omt_to_draw_position( project_to<coords::omt>
                                          ( city_pos ) ).xy();
            const point draw_point = player_to_screen( omt_pos );

            return draw_point + point( ( tile_width - label_length * fontwidth ) / 2,
                                       ( tile_height - fontheight ) / 2 );
        };


        const auto label_bg = [&]( const tripoint_abs_sm & pos, const std::string & name ) {
            const int name_length = utf8_width( name );
            const point draw_pos = abs_sm_to_draw_label( pos, name_length );
            SDL_Rect clipRect = { draw_pos.x, draw_pos.y, name_length * fontwidth, fontheight };

            geometry->rect( renderer, clipRect, SDL_Color() );

            draw_string( *font, renderer, name, draw_pos, 11 );
        };



        const int radius = project_to<coords::sm>( tripoint_abs_omt( std::max( s.x, s.y ),
                           0, 0 ) ).x() / 2;

        for( const city_reference &city : overmap_buffer.get_cities_near(
                 project_to<coords::sm>( center_pos ), radius ) ) {
            const tripoint_abs_omt city_center = project_to<coords::omt>( city.abs_sm_pos );
            if( overmap_buffer.seen_more_than( city_center, om_vision_level::outlines ) &&
                overmap_area.contains( city_center ) ) {
                label_bg( city.abs_sm_pos, city.city->name );
            }
        }

        for( const camp_reference &camp : overmap_buffer.get_camps_near(
                 project_to<coords::sm>( center_pos ), radius ) ) {
            const tripoint_abs_omt camp_center = project_to<coords::omt>( camp.abs_sm_pos );
            if( overmap_buffer.seen_more_than( camp_center, om_vision_level::outlines ) &&
                overmap_area.contains( camp_center ) ) {
                label_bg( camp.abs_sm_pos, camp.camp->camp_name() );
            }
        }
    }

    std::vector<std::pair<nc_color, std::string>> notes_window_text;

    if( overmap_only_auto_travel ) {

        notes_window_text.emplace_back( c_yellow, _( "AUTO TRAVELING" ) );
    }

    if( viewing_weather ) {

        notes_window_text.emplace_back( c_yellow, _( "WEATHER MODE" ) );
    }

    if( uistate.overmap_show_map_notes ) {
        const std::string &note_text = overmap_buffer.note( center_pos );
        if( !note_text.empty() ) {
            const std::tuple<char, nc_color, size_t> note_info = overmap_ui::get_note_display_info(
                        note_text );
            const size_t pos = std::get<2>( note_info );
            if( pos != std::string::npos ) {
                notes_window_text.emplace_back( std::get<1>( note_info ), note_text.substr( pos ) );
            }
            if( overmap_buffer.is_marked_dangerous( center_pos ) ) {
                notes_window_text.emplace_back( c_red, _( "DANGEROUS AREA!" ) );
            }
        }
    }

    if( has_debug_vision ||
        overmap_buffer.seen_more_than( center_pos, om_vision_level::details ) ) {
        for( const auto &npc : npcs_near_player ) {
            if( !npc->marked_for_death && npc->pos_abs_omt() == center_pos && !npc->guaranteed_hostile() ) {
                notes_window_text.emplace_back( npc->basic_symbol_color(), npc->get_name() );
            }
        }
    }

    for( om_vehicle &v : overmap_buffer.get_vehicle( center_pos ) ) {
        notes_window_text.emplace_back( c_white, v.name );
    }

    if( ! g->overmap_data.message.empty() ) {
        const int padding = 2;
        SDL_Rect message_background_rect = {
            0,
            0,
            fontwidth * utf8_width( g->overmap_data.message ) + padding * 2,
            fontheight + padding * 2
        };
        geometry->rect( renderer, message_background_rect, SDL_Color{ 0, 0, 0, 175 } );
        draw_string( *font, renderer, g->overmap_data.message, point( padding, padding ),
                     cata_cursesport::colorpairs[c_white.to_color_pair_index()].FG );
    }


    if( !notes_window_text.empty() ) {
        constexpr int padding = 2;

        const auto draw_note_text = [&]( const point & draw_pos, const std::string & name,
        nc_color & color ) {
            char note_fg_color = color == c_yellow ? 11 :
                                 cata_cursesport::colorpairs[color.to_color_pair_index()].FG;
            return draw_string( *font, renderer, name, draw_pos, note_fg_color );
        };


        auto center_sm = project_to<coords::sm>( tripoint_abs_omt( center_pos.x() + 1,
                         center_pos.y(), center_pos.z() ) );
        const point_abs_omt omt_pos = global_omt_to_draw_position( project_to<coords::omt>
                                      ( center_sm ) ).xy();
        point draw_point = player_to_screen( omt_pos );
        draw_point += point( padding, padding );


        nc_color header_color = c_white;
        const std::string header_string = _( "-- Notes: --" );
        SDL_Rect header_background_rect = {
            draw_point.x - padding,
            draw_point.y - padding,
            fontwidth * utf8_width( header_string ) + padding * 2,
            fontheight + padding * 2
        };
        geometry->rect( renderer, header_background_rect, SDL_Color{ 0, 0, 0, 175 } );
        draw_note_text( draw_point, header_string, header_color );
        draw_point.y += fontheight + padding * 2;

        const int starting_x = draw_point.x;

        for( auto &line : notes_window_text ) {
            const auto color_segments = split_by_color( line.second );
            std::stack<nc_color> color_stack;
            nc_color default_color = std::get<0>( line );
            color_stack.push( default_color );
            std::vector<std::tuple<nc_color, std::string>> colored_lines;

            draw_point.x = starting_x;

            int line_length = 0;
            for( auto seg : color_segments ) {
                if( seg.empty() ) {
                    continue;
                }

                if( seg[0] == '<' ) {
                    const color_tag_parse_result::tag_type type = update_color_stack(
                                color_stack, seg, report_color_error::no );
                    if( type != color_tag_parse_result::non_color_tag ) {
                        seg = rm_prefix( seg );
                    }
                }

                nc_color &color = color_stack.empty() ? default_color : color_stack.top();
                colored_lines.emplace_back( color, seg );
                line_length += utf8_width( seg );
            }


            SDL_Rect background_rect = {
                draw_point.x - padding,
                draw_point.y - padding,
                fontwidth *line_length + padding * 2,
                fontheight + padding * 2
            };
            geometry->rect( renderer, background_rect, SDL_Color{ 0, 0, 0, 175 } );


            for( auto &colored_line : colored_lines ) {
                std::string &text = std::get<1>( colored_line );
                draw_point.x = draw_note_text( draw_point, text, std::get<0>( colored_line ) ).x;
            }

            draw_point.y += fontheight + padding;
        }
    }

    RenderSetClipRect( renderer, nullptr );



    if( cata_shader::variant_pass *vp = get_shared_variant_pass() ) {
        if( !vp->flush() ) {
            draw_scope.abort_unbind();
            display_buffer_scope_signal_recovery_required();
            throw std::runtime_error(
                "cata_tiles::draw_om: variant_pass flush failed at end of frame; renderer in undefined state" );
        }
    }
}

static bool draw_window( Font_Ptr &font, const catacurses::window &w, const point &offset,
                         const bool force_full = false )
{
    if( !unbind_sprite_shader() ) {
        return false;
    }
    if( scaling_factor > 1 ) {
        const point buffer_dims = compute_display_buffer_dims();
        RenderSetLogicalSize( renderer, buffer_dims.x, buffer_dims.y );
    }

    cata_cursesport::WINDOW *const win = w.get<cata_cursesport::WINDOW>();


    static const std::string space_string = " ";

    const bool option_use_draw_ascii_lines_routine = get_option<bool>( "USE_DRAW_ASCII_LINES_ROUTINE" );




    const bool rows_aligned = win->pos.y * font->height == offset.y;
    curses_text_batch.begin_pass( renderer, !text_atlas_enabled( renderer ) );
    bool update = false;
    for( int j = 0; j < win->height; j++ ) {


        if( !force_full && !win->line[j].touched ) {
            continue;
        }






        curses_text_batch.add_rect( SDL_Rect{ win->pos.x * font->width, ( win->pos.y + j ) * font->height,
                                              win->width * font->width, font->height },
                                    color_as_sdl( catacurses::black ) );
        update = true;
        win->line[j].touched = false;

        int queued_span_end = 0;
        for( int i = 0; i < win->width; i++ ) {
            const cursecell &cell = win->line[j].chars[i];

            const point draw( offset + point( i * font->width, j * font->height ) );
            if( draw.x + font->width > WindowWidth || draw.y + font->height > WindowHeight ) {

                continue;
            }

            if( cell.ch.empty() ) {
                continue;
            }



            if( i < queued_span_end ) {
                curses_text_batch.flush( renderer );
                queued_span_end = 0;
            }


            if( cell.ch == space_string ) {
                if( cell.BG != catacurses::black ) {
                    curses_text_batch.add_rect( SDL_Rect{ draw.x, draw.y, font->width, font->height },
                                                color_as_sdl( cell.BG ) );
                }
                continue;
            }
            const int codepoint = UTF8_getch( cell.ch );
            const catacurses::base_color FG = cell.FG;
            const catacurses::base_color BG = cell.BG;
            int cw = ( codepoint == UNKNOWN_UNICODE ) ? 1 : utf8_width( cell.ch );
            if( cw < 1 ) {

                continue;
            }
            bool use_draw_ascii_lines_routine = option_use_draw_ascii_lines_routine;
            unsigned char uc = static_cast<unsigned char>( cell.ch[0] );
            switch( codepoint ) {
                case LINE_XOXO_UNICODE:
                    uc = LINE_XOXO_C;
                    break;
                case LINE_OXOX_UNICODE:
                    uc = LINE_OXOX_C;
                    break;
                case LINE_XXOO_UNICODE:
                    uc = LINE_XXOO_C;
                    break;
                case LINE_OXXO_UNICODE:
                    uc = LINE_OXXO_C;
                    break;
                case LINE_OOXX_UNICODE:
                    uc = LINE_OOXX_C;
                    break;
                case LINE_XOOX_UNICODE:
                    uc = LINE_XOOX_C;
                    break;
                case LINE_XXXO_UNICODE:
                    uc = LINE_XXXO_C;
                    break;
                case LINE_XXOX_UNICODE:
                    uc = LINE_XXOX_C;
                    break;
                case LINE_XOXX_UNICODE:
                    uc = LINE_XOXX_C;
                    break;
                case LINE_OXXX_UNICODE:
                    uc = LINE_OXXX_C;
                    break;
                case LINE_XXXX_UNICODE:
                    uc = LINE_XXXX_C;
                    break;
                case UNKNOWN_UNICODE:
                    use_draw_ascii_lines_routine = true;
                    break;
                default:
                    use_draw_ascii_lines_routine = false;
                    break;
            }
            if( cell.BG != catacurses::black ) {
                curses_text_batch.add_rect( SDL_Rect{ draw.x, draw.y, font->width * cw, font->height },
                                            color_as_sdl( BG ) );
            }
            if( use_draw_ascii_lines_routine ) {
                font->queue_ascii_lines( curses_text_batch, uc, draw, FG );
            } else {
                font->queue_char( curses_text_batch, renderer, cell.ch, draw, FG );
            }
            queued_span_end = std::max( queued_span_end, i + cw );
        }
        if( !rows_aligned ) {
            curses_text_batch.flush( renderer );
        }
    }
    curses_text_batch.flush( renderer );
    win->draw = false;

    return update;
}

static bool draw_window( Font_Ptr &font, const catacurses::window &w,
                         const bool force_full = false )
{
    cata_cursesport::WINDOW *const win = w.get<cata_cursesport::WINDOW>();


    return draw_window( font, w, point( win->pos.x * ::fontwidth, win->pos.y * ::fontheight ),
                        force_full );
}

bool renderer_recovery_test_support::install_test_font( const std::string &typeface,
        const int w, const int h, const int size, const bool blending,
        const Uint32 font_pixel_format )
{
    if( !renderer ) {
        return false;
    }
    if( TTF_WasInit() == 0 ) {
        if( !TTF_Init() ) {
            return false;
        }
        test_fixture_acquired_ttf = true;
    }

    fontwidth = w;
    fontheight = h;
    const Uint32 format = font_pixel_format == SDL_PIXELFORMAT_UNKNOWN ? pixel_format :
                          font_pixel_format;
    fixture_font = Font::load_font( renderer, format, typeface, size, w, h, windowsPalette,
                                    blending );
    return static_cast<bool>( fixture_font );
}

bool renderer_recovery_test_support::draw_test_window( const catacurses::window &w )
{
    if( !fixture_font ) {
        return false;
    }
    draw_window( fixture_font, w, true );
    return true;
}

Font *renderer_recovery_test_support::test_font()
{
    return fixture_font.get();
}

bool renderer_recovery_test_support::rebuild_terminal_fonts( const point &cell,
        const int point_size, const std::vector<font_config> &typefaces,
        const std::vector<font_config> &gui_typefaces )
{
    if( !renderer ) {
        return false;
    }
    if( TTF_WasInit() == 0 ) {
        if( !TTF_Init() ) {
            return false;
        }
        test_fixture_acquired_ttf = true;
    }
    if( !test_fixture_fonts_saved ) {

        test_fixture_saved_font = std::move( font );
        test_fixture_saved_gui_font = std::move( gui_font );
        test_fixture_saved_glyphs_changed = terminal_glyphs_changed;
        test_fixture_fonts_saved = true;
    }
    return ::rebuild_terminal_fonts( cell, point_size, typefaces, gui_typefaces, false );
}

point renderer_recovery_test_support::terminal_cell()
{
    return point( fontwidth, fontheight );
}

const Font *renderer_recovery_test_support::terminal_font()
{
    return font.get();
}

const Font *renderer_recovery_test_support::terminal_gui_font()
{
    return gui_font.get();
}



static bool draw_minimap_window( Font_Ptr &font, const catacurses::window &w, const bool force_full,
                                 const display_buffer_draw_scope &draw_scope, const std::function<void()> &paint )
{
    draw_window( font, w, force_full );
    if( !draw_scope.should_draw() ) {
        return false;
    }
    clear_window_area( w );
    paint();
    return true;
}

bool renderer_recovery_test_support::draw_test_minimap_window( const catacurses::window &w,
        const bool fail_unbind, int &paints )
{
    display_buffer_draw_scope draw_scope;
    if( !fixture_font || !draw_scope.should_draw() ) {
        return false;
    }

    if( fail_unbind ) {
        cata_shader::test_arm_flush_failure();
    }
    return draw_minimap_window( fixture_font, w, true, draw_scope, [&paints]() {
        ++paints;
    } );
}

void cata_cursesport::curses_drawwindow( const catacurses::window &w )
{
    display_buffer_draw_scope draw_scope;
    if( !draw_scope.should_draw() ) {


        return;
    }
    if( scaling_factor > 1 ) {
        const point buffer_dims = compute_display_buffer_dims();
        RenderSetLogicalSize( renderer, buffer_dims.x, buffer_dims.y );
    }
    WINDOW *const win = w.get<WINDOW>();

    const bool force_full = win->last_render_epoch != curses_render_epoch;
    bool update = false;
    if( g && w == g->w_terrain && use_tiles ) {

        color_block_overlay_container color_blocks;


        std::multimap<point, formatted_text> overlay_strings;



        tilecontext->draw(
            point( win->pos.x * fontwidth, win->pos.y * fontheight ),
            g->ter_view_p,
            TERRAIN_WINDOW_TERM_WIDTH * font->width,
            TERRAIN_WINDOW_TERM_HEIGHT * font->height,
            overlay_strings,
            color_blocks );


        if( !color_blocks.second.empty() ) {
            SDL_BlendMode blend_mode;
            GetRenderDrawBlendMode( renderer, blend_mode );
            SetRenderDrawBlendMode( renderer, color_blocks.first );
            for( const auto &e : color_blocks.second ) {
                geometry->rect( renderer, e.first, tilecontext->get_tile_width(),
                                tilecontext->get_tile_height(), e.second );
            }
            SetRenderDrawBlendMode( renderer, blend_mode );
        }


        point prev_coord;
        int x_offset = 0;
        int alignment_offset = 0;
        for( const auto &iter : overlay_strings ) {
            const point coord = iter.first;
            const formatted_text ft = iter.second;


            if( coord != prev_coord ) {
                x_offset = 0;
            }


            if( x_offset == 0 ) {
                int full_text_length = 0;
                const auto range = overlay_strings.equal_range( coord );
                for( auto ri = range.first; ri != range.second; ++ri ) {
                    full_text_length += utf8_width( ri->second.text );
                }

                alignment_offset = 0;
                if( ft.alignment == text_alignment::center ) {
                    alignment_offset = full_text_length / 2;
                } else if( ft.alignment == text_alignment::right ) {
                    alignment_offset = full_text_length - 1;
                }
            }

            int width = 0;
            curses_text_batch.begin_pass( renderer, !text_atlas_enabled( renderer ) );
            for( const char32_t ch : utf8_view( ft.text ) ) {
                const point p0( win->pos.x * fontwidth, win->pos.y * fontheight );
                const point p( coord + p0 + point( ( x_offset - alignment_offset + width ) * map_font->width, 0 ) );


                if( p.x < p0.x || p.x > p0.x + ( TERRAIN_WINDOW_TERM_WIDTH - 1 ) * font->width
                    || p.y < p0.y || p.y > p0.y + ( TERRAIN_WINDOW_TERM_HEIGHT - 1 ) * font->height ) {
                    continue;
                }


                map_font->queue_char( curses_text_batch, renderer, utf32_to_utf8( ch ), p, ft.color );
                width += mk_wcwidth( ch );
            }

            curses_text_batch.flush( renderer );

            prev_coord = coord;
            x_offset = width;
        }

        update = true;
    } else if( g && w == g->w_terrain && map_font ) {




        int partial_width = std::max( TERRAIN_WINDOW_TERM_WIDTH * fontwidth - TERRAIN_WINDOW_WIDTH *
                                      map_font->width, 0 );
        int partial_height = std::max( TERRAIN_WINDOW_TERM_HEIGHT * fontheight - TERRAIN_WINDOW_HEIGHT *
                                       map_font->height, 0 );

        if( partial_height > 0 ) {
            geometry->rect( renderer, point( win->pos.x * map_font->width,
                                             ( win->pos.y + TERRAIN_WINDOW_HEIGHT ) * map_font->height ),
                            TERRAIN_WINDOW_WIDTH * map_font->width + partial_width, partial_height,
                            color_as_sdl( catacurses::black ) );
        }

        if( partial_width > 0 ) {
            geometry->rect( renderer, point( ( win->pos.x + TERRAIN_WINDOW_WIDTH ) * map_font->width,
                                             win->pos.y * map_font->height ),
                            partial_width,
                            TERRAIN_WINDOW_HEIGHT * map_font->height + partial_height,
                            color_as_sdl( catacurses::black ) );
        }

        update = draw_window( map_font, w, force_full );
    } else if( g && w == g->w_overmap && use_tiles && use_tiles_overmap ) {
        overmap_tilecontext->draw_om( win->pos, overmap_ui::redraw_info.center,
                                      overmap_ui::redraw_info.blink );
        update = true;
    } else if( g && w == g->w_overmap && overmap_font ) {

        update = draw_window( overmap_font, w, force_full );
    } else if( g && w == g->w_pixel_minimap && pixel_minimap_option ) {



        update = draw_minimap_window( font, w, force_full, draw_scope, [&]() {
            tilecontext->draw_minimap(
                point( win->pos.x * fontwidth, win->pos.y * fontheight ),
            { get_player_character().pos_bub().xy(), g->ter_view_p.z() },
            win->width * font->width, win->height * font->height );
        } );

    } else {

        update = draw_window( font, w, force_full );
    }
    if( update ) {
        needupdate = true;
    }



    if( renderer_coordinator.should_abort_frame() ) {
        return;
    }
    win->last_render_epoch = curses_render_epoch;
}

static int alt_buffer = 0;
static bool alt_down = false;

static void begin_alt_code()
{
    alt_buffer = 0;
    alt_down = true;
}

static bool add_alt_code( char c )
{
    if( alt_down && c >= '0' && c <= '9' ) {
        alt_buffer = alt_buffer * 10 + ( c - '0' );
        return true;
    }
    return false;
}

static int end_alt_code()
{
    alt_down = false;
    return alt_buffer;
}

static SDL_Keycode sdl_keycode_opposite_arrow( SDL_Keycode key )
{
    switch( key ) {
        case SDLK_UP:
            return SDLK_DOWN;
        case SDLK_DOWN:
            return SDLK_UP;
        case SDLK_LEFT:
            return SDLK_RIGHT;
        case SDLK_RIGHT:
            return SDLK_LEFT;
    }
    return 0;
}

static bool sdl_keycode_is_arrow( SDL_Keycode key )
{
    return static_cast<bool>( sdl_keycode_opposite_arrow( key ) );
}

static int arrow_combo_to_numpad( SDL_Keycode mod, SDL_Keycode key )
{
    if( ( mod == SDLK_UP    && key == SDLK_RIGHT ) ||
        ( mod == SDLK_RIGHT && key == SDLK_UP ) ) {
        return KEY_NUM( 9 );
    }
    if( mod == SDLK_UP    && key == SDLK_UP ) {
        return KEY_NUM( 8 );
    }
    if( ( mod == SDLK_UP    && key == SDLK_LEFT ) ||
        ( mod == SDLK_LEFT  && key == SDLK_UP ) ) {
        return KEY_NUM( 7 );
    }
    if( mod == SDLK_RIGHT && key == SDLK_RIGHT ) {
        return KEY_NUM( 6 );
    }
    if( mod == sdl_keycode_opposite_arrow( key ) ) {
        return KEY_NUM( 5 );
    }
    if( mod == SDLK_LEFT  && key == SDLK_LEFT ) {
        return KEY_NUM( 4 );
    }
    if( ( mod == SDLK_DOWN  && key == SDLK_RIGHT ) ||
        ( mod == SDLK_RIGHT && key == SDLK_DOWN ) ) {
        return KEY_NUM( 3 );
    }
    if( mod == SDLK_DOWN  && key == SDLK_DOWN ) {
        return KEY_NUM( 2 );
    }
    if( ( mod == SDLK_DOWN  && key == SDLK_LEFT ) ||
        ( mod == SDLK_LEFT  && key == SDLK_DOWN ) ) {
        return KEY_NUM( 1 );
    }
    return 0;
}

static int arrow_combo_modifier = 0;

static int handle_arrow_combo( SDL_Keycode key )
{
    if( !arrow_combo_modifier ) {
        arrow_combo_modifier = key;
        return 0;
    }
    return arrow_combo_to_numpad( arrow_combo_modifier, key );
}

static void end_arrow_combo()
{
    arrow_combo_modifier = 0;
}








static int sdl_keysym_to_curses( const CataKeysym &keysym )
{

    const std::string diag_mode = get_option<std::string>( "DIAG_MOVE_WITH_MODIFIERS_MODE" );

    if( diag_mode == "mode1" ) {
        if( keysym.mod & KMOD_CTRL && sdl_keycode_is_arrow( keysym.sym ) ) {
            return handle_arrow_combo( keysym.sym );
        } else {
            end_arrow_combo();
        }
    }

    if( diag_mode == "mode2" ) {

        if( keysym.mod & KMOD_SHIFT ) {
            switch( keysym.sym ) {
                case SDLK_LEFT:
                    return inp_mngr.get_first_char_for_action( "LEFTUP" );
                case SDLK_RIGHT:
                    return inp_mngr.get_first_char_for_action( "RIGHTDOWN" );
                case SDLK_UP:
                    return inp_mngr.get_first_char_for_action( "RIGHTUP" );
                case SDLK_DOWN:
                    return inp_mngr.get_first_char_for_action( "LEFTDOWN" );
            }
        }

        if( keysym.mod & KMOD_CTRL ) {
            switch( keysym.sym ) {
                case SDLK_LEFT:
                    return inp_mngr.get_first_char_for_action( "LEFTDOWN" );
                case SDLK_RIGHT:
                    return inp_mngr.get_first_char_for_action( "RIGHTUP" );
                case SDLK_UP:
                    return inp_mngr.get_first_char_for_action( "LEFTUP" );
                case SDLK_DOWN:
                    return inp_mngr.get_first_char_for_action( "RIGHTDOWN" );
            }
        }
    }

    if( diag_mode == "mode3" ) {

        if( keysym.mod & KMOD_SHIFT ) {
            switch( keysym.sym ) {
                case SDLK_LEFT:
                    return inp_mngr.get_first_char_for_action( "LEFTUP" );
                case SDLK_RIGHT:
                    return inp_mngr.get_first_char_for_action( "RIGHTUP" );
            }
        }

        if( keysym.mod & KMOD_CTRL ) {
            switch( keysym.sym ) {
                case SDLK_LEFT:
                    return inp_mngr.get_first_char_for_action( "LEFTDOWN" );
                case SDLK_RIGHT:
                    return inp_mngr.get_first_char_for_action( "RIGHTDOWN" );
            }
        }
    }

    if( diag_mode == "mode4" ) {
        if( ( keysym.mod & KMOD_SHIFT ) || ( keysym.mod & KMOD_CTRL ) ) {
            const int count = IsScancodePressed( SDL_SCANCODE_LEFT )
                              + IsScancodePressed( SDL_SCANCODE_RIGHT )
                              + IsScancodePressed( SDL_SCANCODE_UP )
                              + IsScancodePressed( SDL_SCANCODE_DOWN );
            if( count == 2 ) {
                switch( keysym.sym ) {
                    case SDLK_LEFT:
                        if( IsScancodePressed( SDL_SCANCODE_UP ) ) {
                            return inp_mngr.get_first_char_for_action( "LEFTUP" );
                        }
                        if( IsScancodePressed( SDL_SCANCODE_DOWN ) ) {
                            return inp_mngr.get_first_char_for_action( "LEFTDOWN" );
                        }
                        return 0;
                    case SDLK_RIGHT:
                        if( IsScancodePressed( SDL_SCANCODE_UP ) ) {
                            return inp_mngr.get_first_char_for_action( "RIGHTUP" );
                        }
                        if( IsScancodePressed( SDL_SCANCODE_DOWN ) ) {
                            return inp_mngr.get_first_char_for_action( "RIGHTDOWN" );
                        }
                        return 0;
                    case SDLK_UP:
                        if( IsScancodePressed( SDL_SCANCODE_LEFT ) ) {
                            return inp_mngr.get_first_char_for_action( "LEFTUP" );
                        }
                        if( IsScancodePressed( SDL_SCANCODE_RIGHT ) ) {
                            return inp_mngr.get_first_char_for_action( "RIGHTUP" );
                        }
                        return 0;
                    case SDLK_DOWN:
                        if( IsScancodePressed( SDL_SCANCODE_LEFT ) ) {
                            return inp_mngr.get_first_char_for_action( "LEFTDOWN" );
                        }
                        if( IsScancodePressed( SDL_SCANCODE_RIGHT ) ) {
                            return inp_mngr.get_first_char_for_action( "RIGHTDOWN" );
                        }
                        return 0;
                }
            } else if( count > 0 ) {
                return 0;
            }
        }
    }

    if( keysym.mod & KMOD_CTRL && keysym.sym >= 'a' && keysym.sym <= 'z' ) {

        return keysym.sym - 'a' + '\1';
    }
    switch( keysym.sym ) {

        case SDLK_RALT:
        case SDLK_LALT:
            begin_alt_code();
            return -1;

        case SDLK_KP_ENTER:
            return KEY_ENTER;
        case SDLK_RETURN:
        case SDLK_RETURN2:
            return '\n';
        case SDLK_BACKSPACE:
        case SDLK_KP_BACKSPACE:
            return KEY_BACKSPACE;
        case SDLK_DELETE:
            return KEY_DC;
        case SDLK_ESCAPE:
            return KEY_ESCAPE;
        case SDLK_TAB:
            if( keysym.mod & KMOD_SHIFT ) {
                return KEY_BTAB;
            }
            return '\t';
        case SDLK_LEFT:
            return KEY_LEFT;
        case SDLK_RIGHT:
            return KEY_RIGHT;
        case SDLK_UP:
            return KEY_UP;
        case SDLK_DOWN:
            return KEY_DOWN;
        case SDLK_PAGEUP:
            return KEY_PPAGE;
        case SDLK_PAGEDOWN:
            return KEY_NPAGE;
        case SDLK_HOME:
            return KEY_HOME;
        case SDLK_END:
            return KEY_END;
        case SDLK_F1:
            return KEY_F( 1 );
        case SDLK_F2:
            return KEY_F( 2 );
        case SDLK_F3:
            return KEY_F( 3 );
        case SDLK_F4:
            return KEY_F( 4 );
        case SDLK_F5:
            return KEY_F( 5 );
        case SDLK_F6:
            return KEY_F( 6 );
        case SDLK_F7:
            return KEY_F( 7 );
        case SDLK_F8:
            return KEY_F( 8 );
        case SDLK_F9:
            return KEY_F( 9 );
        case SDLK_F10:
            return KEY_F( 10 );
        case SDLK_F11:
            return KEY_F( 11 );
        case SDLK_F12:
            return KEY_F( 12 );
        case SDLK_F13:
            return KEY_F( 13 );
        case SDLK_F14:
            return KEY_F( 14 );
        case SDLK_F15:
            return KEY_F( 15 );


        default:
            return 0;
    }
}

static int sdl_keypad_scancode_to_keycode( SDL_Scancode scancode )
{


    switch( scancode ) {
        case SDL_SCANCODE_KP_DIVIDE:
            return keycode::kp_divide;
        case SDL_SCANCODE_KP_MULTIPLY:
            return keycode::kp_multiply;
        case SDL_SCANCODE_KP_MINUS:
            return keycode::kp_minus;
        case SDL_SCANCODE_KP_PLUS:
            return keycode::kp_plus;
        case SDL_SCANCODE_KP_ENTER:
            return keycode::kp_enter;
        case SDL_SCANCODE_KP_1:
            return keycode::kp_1;
        case SDL_SCANCODE_KP_2:
            return keycode::kp_2;
        case SDL_SCANCODE_KP_3:
            return keycode::kp_3;
        case SDL_SCANCODE_KP_4:
            return keycode::kp_4;
        case SDL_SCANCODE_KP_5:
            return keycode::kp_5;
        case SDL_SCANCODE_KP_6:
            return keycode::kp_6;
        case SDL_SCANCODE_KP_7:
            return keycode::kp_7;
        case SDL_SCANCODE_KP_8:
            return keycode::kp_8;
        case SDL_SCANCODE_KP_9:
            return keycode::kp_9;
        case SDL_SCANCODE_KP_0:
            return keycode::kp_0;
        case SDL_SCANCODE_KP_PERIOD:
            return keycode::kp_period;
        default:
            return 0;
    }
}

static input_event sdl_keysym_to_keycode_evt( const CataKeysym &keysym )
{
    switch( keysym.sym ) {
        case SDLK_LCTRL:
        case SDLK_LSHIFT:
        case SDLK_LALT:
        case SDLK_RCTRL:
        case SDLK_RSHIFT:
        case SDLK_RALT:
            return input_event();
    }

    input_event evt;
    evt.type = input_event_t::keyboard_code;
    if( keysym.mod & KMOD_CTRL ) {
        evt.modifiers.emplace( keymod_t::ctrl );
    }
    if( keysym.mod & KMOD_ALT ) {
        evt.modifiers.emplace( keymod_t::alt );
    }
    if( keysym.mod & KMOD_SHIFT ) {
        evt.modifiers.emplace( keymod_t::shift );
    }
    const int keypad_keycode = sdl_keypad_scancode_to_keycode( keysym.scancode );
    evt.sequence.emplace_back( keypad_keycode ? keypad_keycode : keysym.sym );
    return evt;
}



static bool rebuild_terminal_fonts( const point &cell, const int point_size,
                                    const std::vector<font_config> &typefaces,
                                    const std::vector<font_config> &gui_typefaces, const bool blending )
{
    Font_Ptr new_font;
    Font_Ptr new_gui_font;
    try {
        new_font = std::make_unique<FontFallbackList>( renderer, pixel_format, cell.x, cell.y,
                   windowsPalette, typefaces, point_size, blending );
        new_gui_font = std::make_unique<FontFallbackList>( renderer, pixel_format, cell.x, cell.y,
                       windowsPalette, gui_typefaces, point_size, blending );
    } catch( const std::exception &err ) {
        dbg( D_WARNING ) << "failed to build " << cell.x << "x" << cell.y << " terminal fonts: "
                         << err.what();
        return false;
    }
    font = std::move( new_font );
    gui_font = std::move( new_gui_font );
    ::fontwidth = cell.x;
    ::fontheight = cell.y;
    terminal_glyphs_changed = true;
    if( imclient ) {
        imclient->reload_fonts( gui_typefaces, typefaces );
    }
    return true;
}




static bool apply_resize_layout( int w, int h )
{
    const bool logical_changed = w != WindowWidth || h != WindowHeight;
    WindowWidth = w;
    WindowHeight = h;
    refresh_drawable_dims();
#if defined(__ANDROID__)
    ( void )logical_changed;
    const android_text_layout want = android_desired_layout();
    android_buffer_scale = want.scale;
    point grid = want.grid;
    bool cell_changed = false;
    if( want.cell != point( fontwidth, fontheight ) ) {
        cell_changed = rebuild_terminal_fonts( want.cell, android_font_point_size( want.cell ),
                                               android_fonts.typeface, android_fonts.gui_typeface,
                                               android_fonts.fontblending );
        if( !cell_changed ) {
            grid = android_grid_for( point( fontwidth, fontheight ), want.scale );
        }
    }

    if( !cell_changed && grid.x == TERMINAL_WIDTH && grid.y == TERMINAL_HEIGHT ) {
        return false;
    }
    TERMINAL_WIDTH = grid.x;
    TERMINAL_HEIGHT = grid.y;
    catacurses::stdscr = catacurses::newwin( TERMINAL_HEIGHT, TERMINAL_WIDTH, point::zero );
    return true;
#else
    if( logical_changed ) {


        TERMINAL_WIDTH = std::max( WindowWidth / fontwidth / scaling_factor, EVEN_MINIMUM_TERM_WIDTH );
        TERMINAL_HEIGHT = std::max( WindowHeight / fontheight / scaling_factor, EVEN_MINIMUM_TERM_HEIGHT );
        catacurses::stdscr = catacurses::newwin( TERMINAL_HEIGHT, TERMINAL_WIDTH, point::zero );
    }
    return logical_changed;
#endif
}

void resize_term( const int cell_w, const int cell_h )
{
    const int w = cell_w * fontwidth * scaling_factor;
    const int h = cell_h * fontheight * scaling_factor;
    SetWindowSize( window.get(), w, h );


    renderer_coordinator.notify_resize();
}

void request_terminal_relayout()
{
    renderer_coordinator.notify_resize();

    PushWakeEvent();
}

void toggle_fullscreen_window()
{

#if defined(EMSCRIPTEN)
    return;
#endif

    static int restore_win_w = get_option<int>( "TERMINAL_X" ) * fontwidth * scaling_factor;
    static int restore_win_h = get_option<int>( "TERMINAL_Y" ) * fontheight * scaling_factor;

    if( fullscreen ) {
        if( !SetWindowFullscreen( window.get(), FullscreenMode::windowed ) ) {
            printErrorIf( true, "SetWindowFullscreen(windowed) failed" );
            return;
        }
        RestoreWindow( window.get() );
        SetWindowSize( window.get(), restore_win_w, restore_win_h );
        SetWindowMinimumSize( window.get(), fontwidth * EVEN_MINIMUM_TERM_WIDTH * scaling_factor,
                              fontheight * EVEN_MINIMUM_TERM_HEIGHT * scaling_factor );
    } else {
        restore_win_w = WindowWidth;
        restore_win_h = WindowHeight;
        if( !SetWindowFullscreen( window.get(), FullscreenMode::fullscreen_desktop ) ) {
            printErrorIf( true, "SetWindowFullscreen(fullscreen_desktop) failed" );
            return;
        }
    }

    renderer_coordinator.notify_resize();
    fullscreen = !fullscreen;
}

#if defined(__ANDROID__)
static float finger_down_x = -1.0f;
static float finger_down_y = -1.0f;
static float finger_curr_x = -1.0f;
static float finger_curr_y = -1.0f;
static float second_finger_down_x = -1.0f;
static float second_finger_down_y = -1.0f;
static float second_finger_curr_x = -1.0f;
static float second_finger_curr_y = -1.0f;
static float third_finger_down_x = -1.0f;
static float third_finger_down_y = -1.0f;
static float third_finger_curr_x = -1.0f;
static float third_finger_curr_y = -1.0f;

static uint32_t finger_down_time = 0;

static uint32_t finger_repeat_time = 0;

static uint32_t last_tap_time = 0;

static uint32_t ac_back_down_time = 0;

static bool is_two_finger_touch = false;

static bool is_three_finger_touch = false;

static bool is_quick_shortcut_touch = false;
static bool quick_shortcuts_toggle_handled = false;

uint32_t finger_repeat_delay = 500;

static bool needs_sdl_surface_visibility_refresh = true;



static constexpr int CATA_MAX_FINGERS = 3;
static constexpr SDL_FingerID INVALID_FINGER_ID = -1;
static SDL_FingerID finger_id_slots[CATA_MAX_FINGERS] = {
    INVALID_FINGER_ID, INVALID_FINGER_ID, INVALID_FINGER_ID
};

static int finger_slot_for( SDL_FingerID id, bool create_if_missing )
{
    for( int i = 0; i < CATA_MAX_FINGERS; ++i ) {
        if( finger_id_slots[i] == id ) {
            return i;
        }
    }
    if( create_if_missing ) {
        for( int i = 0; i < CATA_MAX_FINGERS; ++i ) {
            if( finger_id_slots[i] == INVALID_FINGER_ID ) {
                finger_id_slots[i] = id;
                return i;
            }
        }
    }
    return -1;
}

static void finger_slot_clear( SDL_FingerID id )
{
    for( int i = 0; i < CATA_MAX_FINGERS; ++i ) {
        if( finger_id_slots[i] == id ) {
            finger_id_slots[i] = INVALID_FINGER_ID;
            return;
        }
    }
}


using quick_shortcuts_t = std::list<input_event>;
std::map<std::string, quick_shortcuts_t> quick_shortcuts_map;




input_context touch_input_context;

std::string get_quick_shortcut_name( const std::string &category )
{
    if( category == "DEFAULTMODE" &&
        g->check_zone( zone_type_id( "NO_AUTO_PICKUP" ), get_player_character().pos_bub() ) &&
        get_option<bool>( "ANDROID_SHORTCUT_ZONE" ) ) {
        return "DEFAULTMODE____SHORTCUTS";
    }
    return category;
}

float android_get_display_density()
{
    JNIEnv *env = ( JNIEnv * )GetAndroidJNIEnv();
    jobject activity = ( jobject )GetAndroidActivity();
    jclass clazz( env->GetObjectClass( activity ) );
    jmethodID method_id = env->GetMethodID( clazz, "getDisplayDensity", "()F" );
    jfloat ans = env->CallFloatMethod( activity, method_id );
    env->DeleteLocalRef( activity );
    env->DeleteLocalRef( clazz );
    return ans;
}


static float quick_shortcut_height()
{
    const float shortcut_dimensions_authored_density = 3.0f;
    const float screen_density_scale = android_get_display_density() /
                                       shortcut_dimensions_authored_density;
    return std::floor( screen_density_scale * get_option<int>( "ANDROID_SHORTCUT_HEIGHT" ) );
}


static void get_quick_shortcut_dimensions( quick_shortcuts_t &qsl, float &border, float &width,
        float &height )
{
    const float shortcut_dimensions_authored_density = 3.0f;
    float screen_density_scale = android_get_display_density() / shortcut_dimensions_authored_density;
    border = std::floor( screen_density_scale * get_option<int>( "ANDROID_SHORTCUT_BORDER" ) );
    width = std::floor( screen_density_scale * get_option<int>( "ANDROID_SHORTCUT_WIDTH_MAX" ) );
    float min_width = std::floor( screen_density_scale * std::min(
                                      get_option<int>( "ANDROID_SHORTCUT_WIDTH_MIN" ),
                                      get_option<int>( "ANDROID_SHORTCUT_WIDTH_MAX" ) ) );
    float usable_window_width = WindowWidth * get_option<int>( "ANDROID_SHORTCUT_SCREEN_PERCENTAGE" ) *
                                0.01f;
    if( width * qsl.size() > usable_window_width ) {
        width *= usable_window_width / ( width * qsl.size() );
        if( width < min_width ) {
            width = min_width;
        }
    }
    width = std::floor( width );
    height = quick_shortcut_height();
}





struct quick_shortcut_layout {
    float border = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    int per_row = 0;
    int count = 0;
    float top = 0.0f;
    bool right = false;
};

static quick_shortcut_layout get_quick_shortcut_layout( quick_shortcuts_t &qsl )
{
    quick_shortcut_layout layout;
    get_quick_shortcut_dimensions( qsl, layout.border, layout.width, layout.height );
    layout.right = get_option<std::string>( "ANDROID_SHORTCUT_POSITION" ) == "right";
    const float usable_width = WindowWidth * get_option<int>( "ANDROID_SHORTCUT_SCREEN_PERCENTAGE" ) *
                               0.01f;
    int rows = 1;
    layout.top = WindowHeight - layout.height;
    if( terminal_layout::is_portrait( point( WindowWidth, WindowHeight ) ) ) {
        const float density_scale = quick_shortcut_height() /
                                    std::max( 1, get_option<int>( "ANDROID_SHORTCUT_HEIGHT" ) );
        layout.width = std::floor( density_scale * get_option<int>( "ANDROID_SHORTCUT_WIDTH_MAX" ) );
        const std::optional<SDL_Rect> ime_frame = android_visible_ime_frame();
        if( ime_frame ) {
            layout.top = ime_frame->y + ime_frame->h - layout.height;
        } else {
            const SDL_Rect bounds = android_layout_bounds();
            const float free_top = static_cast<float>( bounds.y + bounds.h );
            const int fit_rows = static_cast<int>( ( WindowHeight - free_top ) / layout.height );
            if( fit_rows >= 1 ) {
                rows = fit_rows;
                layout.top = free_top;
            }
        }
    }
    layout.per_row = layout.width > 0.0f ? static_cast<int>( usable_width / layout.width ) : 0;
    layout.count = std::min( static_cast<int>( qsl.size() ), layout.per_row * rows );
    return layout;
}


static SDL_FRect quick_shortcut_cell( const quick_shortcut_layout &layout, const int i )
{
    const int col = i % layout.per_row;
    const int row = i / layout.per_row;
    const float x = layout.right ? WindowWidth - ( col + 1 ) * layout.width : col * layout.width;
    return SDL_FRect{ x, layout.top + row * layout.height, layout.width, layout.height };
}


input_event *get_quick_shortcut_under_finger( bool down = false )
{

    if( !quick_shortcuts_enabled ) {
        return NULL;
    }

    quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name(
                                 touch_input_context.get_category() )];

    const quick_shortcut_layout layout = get_quick_shortcut_layout( qsl );
    const float finger_x = down ? finger_down_x : finger_curr_x;
    const float finger_y = down ? finger_down_y : finger_curr_y;
    int i = 0;
    for( auto it = qsl.begin(); it != qsl.end() && i < layout.count; ++it, ++i ) {
        const SDL_FRect cell = quick_shortcut_cell( layout, i );
        if( finger_x >= cell.x && finger_x < cell.x + cell.w &&
            finger_y >= cell.y && finger_y < cell.y + cell.h ) {
            return &( *it );
        }
    }

    return NULL;
}


bool ignore_action_for_quick_shortcuts( const std::string &action )
{
    return ( action == "UP"
             || action == "DOWN"
             || action == "LEFT"
             || action == "RIGHT"
             || action == "LEFTUP"
             || action == "LEFTDOWN"
             || action == "RIGHTUP"
             || action == "RIGHTDOWN"
             || action == "QUIT"
             || action == "CONFIRM"
             || action == "MOVE_SINGLE_ITEM"
             || action == "MOVE_ARMOR"
             || action == "ANY_INPUT"
             || action ==
             "DELETE_TEMPLATE"
           );
}


void add_quick_shortcut( quick_shortcuts_t &qsl, input_event &event, bool back,
                         bool reset_shortcut_last_used_action_counter )
{
    if( reset_shortcut_last_used_action_counter && g ) {
        event.shortcut_last_used_action_counter =
            g->get_user_action_counter();
    }
    if( back ) {
        qsl.push_back( event );
    } else {
        qsl.push_front( event );
    }
}


void reorder_quick_shortcut( quick_shortcuts_t &qsl, int key, bool back )
{
    for( const auto &event : qsl ) {
        if( event.get_first_input() == key ) {
            input_event event_copy = event;
            qsl.remove( event );
            add_quick_shortcut( qsl, event_copy, back, false );
            break;
        }
    }
}

void reorder_quick_shortcuts( quick_shortcuts_t &qsl )
{


    bool shortcut_right = get_option<std::string>( "ANDROID_SHORTCUT_POSITION" ) == "right";
    if( shortcut_right ) {
        reorder_quick_shortcut( qsl, KEY_PPAGE, false );
        reorder_quick_shortcut( qsl, KEY_NPAGE, false );
        reorder_quick_shortcut( qsl, KEY_BTAB, false );
        reorder_quick_shortcut( qsl, '\t', false );
        reorder_quick_shortcut( qsl, '<', false );
        reorder_quick_shortcut( qsl, '>', false );
        reorder_quick_shortcut( qsl, '?', false );
    } else {
        reorder_quick_shortcut( qsl, KEY_NPAGE, false );
        reorder_quick_shortcut( qsl, KEY_PPAGE, false );
        reorder_quick_shortcut( qsl, '\t', false );
        reorder_quick_shortcut( qsl, KEY_BTAB, false );
        reorder_quick_shortcut( qsl, '>', false );
        reorder_quick_shortcut( qsl, '<', false );
        reorder_quick_shortcut( qsl, '?', false );
    }
}

int choose_best_key_for_action( const std::string &action, const std::string &category )
{
    const std::vector<input_event> &events = inp_mngr.get_input_for_action( action, category );
    int best_key = -1;
    for( const auto &events_event : events ) {
        if( events_event.type == input_event_t::keyboard_char && events_event.sequence.size() == 1 ) {
            bool is_ascii_char = isprint( events_event.sequence.front() ) &&
                                 events_event.sequence.front() < 0xFF;
            bool is_best_ascii_char = best_key >= 0 && isprint( best_key ) && best_key < 0xFF;
            if( best_key < 0 || ( is_ascii_char && !is_best_ascii_char ) ) {
                best_key = events_event.sequence.front();
            }
        }
    }
    return best_key;
}

bool add_key_to_quick_shortcuts( int key, const std::string &category, bool back )
{
    if( key > 0 ) {
        quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name( category )];
        input_event event = input_event( key, input_event_t::keyboard_char );
        quick_shortcuts_t::iterator it = std::find( qsl.begin(), qsl.end(), event );
        if( it != qsl.end() ) {
            ( *it ).shortcut_last_used_action_counter =
                g->get_user_action_counter();
        } else {
            add_quick_shortcut( qsl, event, back,
                                true );
            return true;
        }
    }
    return false;
}
bool add_best_key_for_action_to_quick_shortcuts( std::string action_str,
        const std::string &category, bool back )
{
    int best_key = choose_best_key_for_action( action_str, category );
    return add_key_to_quick_shortcuts( best_key, category, back );
}

bool add_best_key_for_action_to_quick_shortcuts( action_id action, const std::string &category,
        bool back )
{
    return add_best_key_for_action_to_quick_shortcuts( action_ident( action ), category, back );
}

void remove_action_from_quick_shortcuts( std::string action_str, const std::string &category )
{
    quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name( category )];
    const std::vector<input_event> &events = inp_mngr.get_input_for_action( action_str, category );
    for( const auto &event : events ) {
        qsl.remove( event );
    }
}

void remove_action_from_quick_shortcuts( action_id action, const std::string &category )
{
    remove_action_from_quick_shortcuts( action_ident( action ), category );
}


bool remove_expired_actions_from_quick_shortcuts( const std::string &category )
{
    int remove_turns = get_option<int>( "ANDROID_SHORTCUT_REMOVE_TURNS" );
    if( remove_turns <= 0 ) {
        return false;
    }


    if( category != "DEFAULTMODE" ) {
        return false;
    }

    bool ret = false;
    quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name( category )];
    quick_shortcuts_t::iterator it = qsl.begin();
    while( it != qsl.end() ) {
        if( g->get_user_action_counter() - ( *it ).shortcut_last_used_action_counter > remove_turns ) {
            it = qsl.erase( it );
            ret = true;
        } else {
            ++it;
        }
    }
    return ret;
}

void remove_stale_inventory_quick_shortcuts()
{
    if( get_option<bool>( "ANDROID_INVENTORY_AUTOADD" ) ) {
        quick_shortcuts_t &qsl = quick_shortcuts_map["INVENTORY"];
        quick_shortcuts_t::iterator it = qsl.begin();
        bool in_inventory;
        int key;
        bool valid;
        while( it != qsl.end() ) {
            key = ( *it ).get_first_input();
            valid = inv_chars.valid( key );
            in_inventory = false;
            if( valid ) {
                Character &player_character = get_player_character();

                std::optional<const item *> item = player_character.worn.item_worn_with_inv_let( key );
                if( item ) {
                    in_inventory = true;
                }
                if( !in_inventory ) {

                    item_location wielded = player_character.get_wielded_item();
                    if( wielded && wielded->invlet == key ) {
                        in_inventory = true;
                    }
                }
            }
            if( valid && !in_inventory ) {
                it = qsl.erase( it );
            } else {
                ++it;
            }
        }
    }
}



static void android_request_repaint()
{
    needupdate = true;
    ui_manager::redraw_invalidated();
}

static void reset_touch_state()
{
    third_finger_down_x = third_finger_curr_x = second_finger_down_x = second_finger_curr_x =
                              finger_down_x = finger_curr_x = -1.0f;
    third_finger_down_y = third_finger_curr_y = second_finger_down_y = second_finger_curr_y =
                              finger_down_y = finger_curr_y = -1.0f;
    is_two_finger_touch = false;
    is_three_finger_touch = false;
    finger_down_time = 0;
    finger_repeat_time = 0;
}



static void cancel_touch()
{
    reset_touch_state();
    is_quick_shortcut_touch = false;
    for( SDL_FingerID &slot_id : finger_id_slots ) {
        slot_id = INVALID_FINGER_ID;
    }
}





static bool android_keyboard_occludes_shortcuts()
{
    if( !IsTextInputActive( ::window.get() ) ) {
        return false;
    }
    SDL_Rect frame;
    bool has_frame = false;
    bool visible = false;
    visible_frame_inbox.read_frame( frame, has_frame, visible );
    if( !has_frame ) {
        return true;
    }
    return visible && frame.h > 0 && !android_shortcuts_above_keyboard();
}


void draw_quick_shortcuts()
{

    if( !quick_shortcuts_enabled ||
        android_keyboard_occludes_shortcuts() ||
        ( get_option<bool>( "ANDROID_HIDE_HOLDS" ) && !is_quick_shortcut_touch && finger_down_time > 0 &&
          GetTicks() - finger_down_time >= static_cast<uint32_t>(
              get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) ) {
        return;
    }

    bool shortcut_right = get_option<std::string>( "ANDROID_SHORTCUT_POSITION" ) == "right";
    std::string &category = touch_input_context.get_category();
    bool is_default_mode = category == "DEFAULTMODE";
    quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name( category )];
    if( qsl.empty() || !touch_input_context.get_registered_manual_keys().empty() ) {
        if( category == "DEFAULTMODE" ) {
            const std::string default_gameplay_shortcuts =
                get_option<std::string>( "ANDROID_SHORTCUT_DEFAULTS" );
            for( const auto &c : default_gameplay_shortcuts ) {
                add_key_to_quick_shortcuts( c, category, true );
            }
        } else {



            if( !touch_input_context.get_registered_manual_keys().empty() ) {
                qsl.clear();
            }


            std::vector<std::string> &registered_actions = touch_input_context.get_registered_actions();
            for( std::vector<std::string>::iterator it = registered_actions.begin();
                 it != registered_actions.end(); ++it ) {
                std::string &action = *it;
                if( ignore_action_for_quick_shortcuts( action ) ) {
                    continue;
                }

                add_best_key_for_action_to_quick_shortcuts( action, category, !shortcut_right );
            }


            std::vector<input_context::manual_key> &registered_manual_keys =
                touch_input_context.get_registered_manual_keys();
            for( const auto &manual_key : registered_manual_keys ) {
                input_event event( manual_key.key, input_event_t::keyboard_char );
                add_quick_shortcut( qsl, event, !shortcut_right, true );
            }
        }
    }


    if( !is_default_mode ) {
        reorder_quick_shortcuts( qsl );
    }

    const quick_shortcut_layout layout = get_quick_shortcut_layout( qsl );
    const float border = layout.border;
    const float width = layout.width;
    const float height = layout.height;
    input_event *hovered_quick_shortcut = get_quick_shortcut_under_finger();
    SDL_Rect rect;
    bool hovered, show_hint;
    int i = 0;
    for( std::list<input_event>::iterator it = qsl.begin(); it != qsl.end() &&
         i < layout.count; ++it ) {
        const SDL_FRect cell = quick_shortcut_cell( layout, i );
        input_event &event = *it;
        std::string text = event.text;
        int key = event.get_first_input();
        float default_text_scale = std::floor( 0.75f * ( height /
                                               font->height ) );
        float text_scale = default_text_scale;
        if( text.empty() || text == " " ) {
            text = inp_mngr.get_keyname( key, event.type );
            text_scale = std::min( text_scale, 0.75f * ( width / ( font->width * utf8_width( text ) ) ) );
        }
        hovered = is_quick_shortcut_touch && hovered_quick_shortcut == &event;
        show_hint = hovered &&
                    GetTicks() - finger_down_time > static_cast<uint32_t>
                    ( get_option<int>( "ANDROID_INITIAL_DELAY" ) );
        std::string hint_text = "none";
        if( show_hint ) {
            if( touch_input_context.get_category() == "INVENTORY" && inv_chars.valid( key ) ) {
                Character &player_character = get_player_character();

                if( hint_text == "none" ) {

                    std::optional<const item *> item = player_character.worn.item_worn_with_inv_let( key );
                    if( item ) {
                        hint_text = item.value()->display_name();
                    }
                }
                if( hint_text == "none" ) {

                    item_location wielded = player_character.get_wielded_item();
                    if( wielded && wielded->invlet == key ) {
                        hint_text = player_character.get_wielded_item()->display_name();
                    }
                }
            } else {

                hint_text = touch_input_context.get_action_name( touch_input_context.input_to_action( event ) );
                if( hint_text == "ERROR" ) {
                    hint_text = touch_input_context.get_action_name_for_manual_key( key );
                }
            }
            if( hint_text == "ERROR" || hint_text == "none" || hint_text.empty() ) {
                show_hint = false;
            }
        }
        rect = { static_cast<int>( cell.x + border ), static_cast<int>( cell.y ), static_cast<int>( width - border * 2 ), static_cast<int>( height ) };
        if( hovered ) {
            SetRenderDrawColor( renderer, 0, 0, 0, 255 );
        } else {
            SetRenderDrawColor( renderer, 0, 0, 0,
                                get_option<int>( "ANDROID_SHORTCUT_OPACITY_BG" ) * 0.01f * 255.0f );
        }
        SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
        RenderFillRect( renderer, &rect );
        if( hovered ) {

            rect = { static_cast<int>( cell.x + border ), static_cast<int>( cell.y - height * 1.2f ), static_cast<int>( width - border * 2 ), static_cast<int>( height ) };
            SetRenderDrawColor( renderer, 0, 0, 196, 255 );
            RenderFillRect( renderer, &rect );

            if( show_hint ) {

                rect = { 0, static_cast<int>( ( WindowHeight - height ) * 0.5f ), static_cast<int>( WindowWidth ), static_cast<int>( height ) };
                SetRenderDrawColor( renderer, 0, 0, 0,
                                    get_option<int>( "ANDROID_SHORTCUT_OPACITY_BG" ) * 0.01f * 255.0f );
                RenderFillRect( renderer, &rect );
            }
        }
        SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
        RenderSetScale( renderer, text_scale, text_scale );
        int text_x = ( cell.x + 0.5f * width - ( font->width * utf8_width( text ) ) * text_scale * 0.5f ) /
                     text_scale;

        int text_y = ( cell.y + ( height - font->height * text_scale ) * 0.5f ) / text_scale;
        font->OutputChar( renderer, text, point( text_x + 1, text_y + 1 ), 0,
                          get_option<int>( "ANDROID_SHORTCUT_OPACITY_SHADOW" ) * 0.01f );
        font->OutputChar( renderer, text, point( text_x, text_y ),
                          get_option<int>( "ANDROID_SHORTCUT_COLOR" ),
                          get_option<int>( "ANDROID_SHORTCUT_OPACITY_FG" ) * 0.01f );
        if( hovered ) {

            font->OutputChar( renderer, text,
                              point( text_x, text_y - ( height * 1.2f / text_scale ) ),
                              get_option<int>( "ANDROID_SHORTCUT_COLOR" ) );
            if( show_hint ) {

                text_scale = default_text_scale;
                hint_text = text + " " + hint_text;
                hint_text = remove_color_tags( hint_text );
                const float safe_margin = 0.9f;
                int hint_length = utf8_width( hint_text );
                if( WindowWidth * safe_margin < font->width * text_scale * hint_length ) {
                    text_scale *= ( WindowWidth * safe_margin ) / ( font->width * text_scale *
                                  hint_length );
                }
                RenderSetScale( renderer, text_scale, text_scale );
                text_x = ( WindowWidth - ( ( font->width  * hint_length ) * text_scale ) ) * 0.5f / text_scale;
                text_y = ( WindowHeight - font->height * text_scale ) * 0.5f / text_scale;
                font->OutputChar( renderer, hint_text, point( text_x + 1, text_y + 1 ), 0,
                                  get_option<int>( "ANDROID_SHORTCUT_OPACITY_SHADOW" ) * 0.01f );
                font->OutputChar( renderer, hint_text, point( text_x, text_y ),
                                  get_option<int>( "ANDROID_SHORTCUT_COLOR" ),
                                  get_option<int>( "ANDROID_SHORTCUT_OPACITY_FG" ) * 0.01f );
            }
        }
        RenderSetScale( renderer, 1.0f, 1.0f );
        i++;
    }
}


static float android_touch_deadzone()
{
    return get_option<float>( "ANDROID_DEADZONE_RANGE" ) * get_option<float>( "ANDROID_JOYSTICK_SCALE" )
           *
           std::max( WindowWidth, WindowHeight );
}


static float android_touch_repeat_range()
{
    return std::max( 0.01f, get_option<float>( "ANDROID_REPEAT_DELAY_RANGE" ) ) *
           get_option<float>( "ANDROID_JOYSTICK_SCALE" ) * std::max( WindowWidth, WindowHeight );
}



static float android_touch_straight_width()
{
    if( !touch_input_context.is_action_registered( "LEFTUP" ) ) {
        return 90.0f;
    }
    return get_option<float>( "ANDROID_JOYSTICK_STRAIGHT_ANGLE" );
}


static float android_touch_speed()
{
    const float delta_x = finger_curr_x - finger_down_x;
    const float delta_y = finger_curr_y - finger_down_y;
    return touch_joystick::speed_fraction( std::sqrt( delta_x * delta_x + delta_y * delta_y ),
                                           android_touch_deadzone(), android_touch_repeat_range() );
}


static void draw_joystick_band( const SDL_FPoint &center, const float inner, const float outer,
                                const float start, const float end, const SDL_FColor &color )
{
    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;
    for( const float degrees : touch_joystick::band_angles( start, end ) ) {
        const float radians = degrees * static_cast<float>( M_PI ) / 180.0f;
        const float cos_a = std::cos( radians );
        const float sin_a = std::sin( radians );
        const float r_in = touch_joystick::octagon_radius( degrees, inner );
        const float r_out = touch_joystick::octagon_radius( degrees, outer );
        const int first = static_cast<int>( vertices.size() );
        vertices.push_back( { { center.x + r_in * cos_a, center.y + r_in * sin_a }, color, { 0.0f, 0.0f } } );
        vertices.push_back( { { center.x + r_out * cos_a, center.y + r_out * sin_a }, color, { 0.0f, 0.0f } } );
        if( first > 0 ) {
            indices.insert( indices.end(), { first - 2, first - 1, first + 1, first - 2, first + 1, first } );
        }
    }
    RenderGeometry( renderer, nullptr, vertices.data(), static_cast<int>( vertices.size() ),
                    indices.data(), static_cast<int>( indices.size() ) );
}


static void draw_joystick_spoke( const SDL_FPoint &center, const float inner, const float outer,
                                 const float degrees, const float half_width, const SDL_FColor &color )
{
    const float radians = degrees * static_cast<float>( M_PI ) / 180.0f;
    const float cos_a = std::cos( radians );
    const float sin_a = std::sin( radians );
    const float r_in = touch_joystick::octagon_radius( degrees, inner );
    const float r_out = touch_joystick::octagon_radius( degrees, outer );
    const float nx = -sin_a * half_width;
    const float ny = cos_a * half_width;
    const std::array<SDL_Vertex, 4> v = { {
            { { center.x + r_in *cos_a - nx, center.y + r_in *sin_a - ny }, color, { 0.0f, 0.0f } },
            { { center.x + r_out *cos_a - nx, center.y + r_out *sin_a - ny }, color, { 0.0f, 0.0f } },
            { { center.x + r_out *cos_a + nx, center.y + r_out *sin_a + ny }, color, { 0.0f, 0.0f } },
            { { center.x + r_in *cos_a + nx, center.y + r_in *sin_a + ny }, color, { 0.0f, 0.0f } },
        }
    };
    static constexpr std::array<int, 6> idx = { 0, 1, 2, 0, 2, 3 };
    RenderGeometry( renderer, nullptr, v.data(), static_cast<int>( v.size() ), idx.data(),
                    static_cast<int>( idx.size() ) );
}

void draw_virtual_joystick()
{


    if( !get_option<bool>( "ANDROID_SHOW_VIRTUAL_JOYSTICK" ) ||
        finger_down_time <= 0 ||
        GetTicks() - finger_down_time <= static_cast<uint32_t>
        ( get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ||
        is_quick_shortcut_touch ||
        is_two_finger_touch ||
        is_three_finger_touch ) {
        return;
    }

    const float opacity = get_option<int>( "ANDROID_VIRTUAL_JOYSTICK_OPACITY" ) * 0.01f;
    const SDL_FColor frame{ 1.0f, 1.0f, 1.0f, std::min( 1.0f, opacity * 2.0f ) };
    const SDL_FColor backdrop{ 0.0f, 0.0f, 0.0f, opacity };
    const SDL_FColor highlight{ 1.0f, 1.0f, 1.0f, opacity * 0.75f };
    const SDL_FColor speed{ 1.0f, 0.85f, 0.4f, std::min( 1.0f, opacity * 2.5f ) };

    const SDL_FPoint center{ finger_down_x, finger_down_y };
    const float deadzone = android_touch_deadzone();
    const float rim = deadzone + android_touch_repeat_range();
    const float line = std::max( 1.0f, rim * 0.015f );
    const float straight_width = android_touch_straight_width();

    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
    draw_joystick_band( center, deadzone, rim, 0.0f, 360.0f, backdrop );

    const float delta_x = finger_curr_x - finger_down_x;
    const float delta_y = finger_curr_y - finger_down_y;
    const touch_joystick::direction dir = touch_joystick::classify( delta_x, delta_y, deadzone,
                                          straight_width );
    if( dir != touch_joystick::direction::none ) {
        const std::pair<float, float> slice = touch_joystick::wedge( dir, straight_width );
        draw_joystick_band( center, deadzone, rim, slice.first, slice.second, highlight );
        const float reach = deadzone + ( rim - deadzone ) * android_touch_speed();
        draw_joystick_band( center, deadzone, reach, slice.first, slice.second, speed );
    }


    using touch_joystick::direction;
    std::vector<direction> slices = { direction::right, direction::down, direction::left, direction::up };
    if( straight_width < 90.0f ) {
        slices.insert( slices.end(), { direction::down_right, direction::down_left, direction::up_left,
                                       direction::up_right
                                     } );
    }
    for( const direction slice_dir : slices ) {
        draw_joystick_spoke( center, deadzone, rim, touch_joystick::wedge( slice_dir,
                             straight_width ).first,
                             line * 0.5f, frame );
    }
    draw_joystick_band( center, deadzone - line, deadzone, 0.0f, 360.0f, frame );
    draw_joystick_band( center, rim - line, rim, 0.0f, 360.0f, frame );


    const float knob = rim * 0.2f;
    float knob_x = delta_x;
    float knob_y = delta_y;
    const float knob_dist = std::sqrt( knob_x * knob_x + knob_y * knob_y );
    if( knob_dist > rim ) {
        knob_x *= rim / knob_dist;
        knob_y *= rim / knob_dist;
    }
    draw_joystick_band( SDL_FPoint{ center.x + knob_x, center.y + knob_y }, 0.0f, knob, 0.0f, 360.0f,
                        frame );
    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void draw_gamepad_radial_menu()
{
    if( !gamepad::is_active() || !gamepad::is_alt_held() ) {
        return;
    }

    int stick_idx = -1;
    gamepad::direction selected_dir = gamepad::direction::NONE;
    if( gamepad::is_radial_left_open() ) {
        stick_idx = 0;
        selected_dir = gamepad::get_radial_left_direction();
    } else if( gamepad::is_radial_right_open() ) {
        stick_idx = 1;
        selected_dir = gamepad::get_radial_right_direction();
    }

    if( stick_idx == -1 ) {
        return;
    }

    input_context *ctxt = input_context::input_context_stack.back();
    if( !ctxt ) {
        return;
    }

    int w;
    int h;
    GetRendererOutputSize( renderer, &w, &h );


    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
    geometry->rect( renderer, { 0, 0, w, h }, { 0, 0, 0, 150 } );
    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );

    const point center( w / 2, h / 2 );
    const float radius = h * 0.8f / 2.0f;



    float text_scale = static_cast<float>( h ) / 720.0f * 1.0f;
    RenderSetScale( renderer, text_scale, text_scale );

    static const std::vector<gamepad::direction> directions = {
        gamepad::direction::N, gamepad::direction::NE, gamepad::direction::E,
        gamepad::direction::SE, gamepad::direction::S, gamepad::direction::SW,
        gamepad::direction::W, gamepad::direction::NW
    };

    for( size_t i = 0; i < directions.size(); ++i ) {
        gamepad::direction dir = directions[i];
        int joy_code = gamepad::direction_to_radial_joy( dir, stick_idx );
        input_event event( joy_code, input_event_t::gamepad );
        const std::string &action = ctxt->input_to_action( event );

        std::string label;
        if( action == "ERROR" ) {
            label = "None";
        } else {
            label = ctxt->get_action_name( action );
            if( label.empty() ) {
                label = "None";
            }
        }


        float angle = i * ( 2.0f * M_PI / 8.0f );
        float target_x = center.x + radius * std::sin( angle );
        float target_y = center.y - radius * std::cos( angle );


        const point draw( static_cast<int>( target_x / text_scale ) - ( font->width * utf8_width(
                              label ) / 2 ),
                          static_cast<int>( target_y / text_scale ) - ( font->height / 2 ) );

        bool is_selected = dir == selected_dir;

        if( is_selected ) {

            font->OutputChar( renderer, label, draw,
                              11, 1.0f );
        } else {
            font->OutputChar( renderer, label, draw,
                              15, 1.0f );
        }
    }


    RenderSetScale( renderer, 1.0f, 1.0f );
}

#if defined(__ANDROID__)
void update_finger_repeat_delay()
{
    const float t = android_touch_speed();
    float repeat_delay_min = static_cast<float>( get_option<int>( "ANDROID_REPEAT_DELAY_MIN" ) );
    float repeat_delay_max = static_cast<float>( get_option<int>( "ANDROID_REPEAT_DELAY_MAX" ) );
    finger_repeat_delay = lerp<float>( std::max( repeat_delay_min, repeat_delay_max ),
                                       std::min( repeat_delay_min, repeat_delay_max ),
                                       std::pow( t, get_option<float>( "ANDROID_SENSITIVITY_POWER" ) ) );
}




bool is_string_input( input_context &ctx )
{
    std::string &category = ctx.get_category();
    return category == "STRING_INPUT"
           || category == "STRING_EDITOR"
           || category == "HELP_KEYBINDINGS";
}





static bool android_wants_text_input( input_context &ctx )
{
    return is_string_input( ctx ) || ctx.allow_text_entry
           || cataimgui::client::want_text_input();
}

int get_key_event_from_string( const std::string &str )
{
    if( !str.empty() ) {
        return str[0];
    }
    return -1;
}

static input_event touch_direction_input( const touch_joystick::direction dir )
{
    using touch_joystick::direction;
    switch( dir ) {
        case direction::right:
            return input_event( KEY_RIGHT, input_event_t::keyboard_char );
        case direction::down:
            return input_event( KEY_DOWN, input_event_t::keyboard_char );
        case direction::left:
            return input_event( KEY_LEFT, input_event_t::keyboard_char );
        case direction::up:
            return input_event( KEY_UP, input_event_t::keyboard_char );
        case direction::down_right:
            return input_event( JOY_LS_DOWN_RIGHT, input_event_t::gamepad );
        case direction::down_left:
            return input_event( JOY_LS_DOWN_LEFT, input_event_t::gamepad );
        case direction::up_left:
            return input_event( JOY_LS_UP_LEFT, input_event_t::gamepad );
        case direction::up_right:
            return input_event( JOY_LS_UP_RIGHT, input_event_t::gamepad );
        case direction::none:
            break;
    }
    return input_event();
}


void handle_finger_input( uint32_t ticks )
{

    const float delta_x = finger_curr_x - finger_down_x;
    const float delta_y = finger_curr_y - finger_down_y;
    const bool is_default_mode = touch_input_context.get_category() == "DEFAULTMODE";
    const touch_joystick::direction dir = touch_joystick::classify( delta_x, delta_y,
                                          android_touch_deadzone(), android_touch_straight_width() );
    if( dir != touch_joystick::direction::none ) {
        last_input = touch_direction_input( dir );
    } else {
        if( ticks - finger_down_time >= static_cast<uint32_t>
            ( get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) {


            if( is_default_mode ) {
                last_input = input_event( get_key_event_from_string( get_option<std::string>( "ANDROID_TAP_KEY" ) ),
                                          input_event_t::keyboard_char );
            }
        } else {
            if( last_tap_time > 0 &&
                ticks - last_tap_time < static_cast<uint32_t>( get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) {

                last_input = input_event( is_default_mode ? KEY_ESCAPE : KEY_ESCAPE, input_event_t::keyboard_char );
                last_tap_time = 0;
            } else {

                last_tap_time = ticks;
            }
        }
    }
    if( last_input.type != input_event_t::error ) {
        imclient->process_cata_input( last_input );
    }
}

bool android_is_hardware_keyboard_available()
{
    JNIEnv *env = ( JNIEnv * )GetAndroidJNIEnv();
    jobject activity = ( jobject )GetAndroidActivity();
    jclass clazz( env->GetObjectClass( activity ) );
    jmethodID method_id = env->GetMethodID( clazz, "isHardwareKeyboardAvailable", "()Z" );
    jboolean ans = env->CallBooleanMethod( activity, method_id );
    env->DeleteLocalRef( activity );
    env->DeleteLocalRef( clazz );
    return ans;
}

void android_vibrate()
{
    int vibration_ms = get_option<int>( "ANDROID_VIBRATION" );
    if( vibration_ms > 0 && !android_is_hardware_keyboard_available() ) {
        JNIEnv *env = ( JNIEnv * )GetAndroidJNIEnv();
        jobject activity = ( jobject )GetAndroidActivity();
        jclass clazz( env->GetObjectClass( activity ) );
        jmethodID method_id = env->GetMethodID( clazz, "vibrate", "(I)V" );
        env->CallVoidMethod( activity, method_id, vibration_ms );
        env->DeleteLocalRef( activity );
        env->DeleteLocalRef( clazz );
    }
}
#endif

#if !defined(__ANDROID__)
static bool window_focus = false;
static bool text_input_active_when_regaining_focus = false;
#endif





static void focus_aware_start_text_input()
{

    if( IsTextInputActive( ::window.get() ) ) {
        return;
    }
#if defined(__ANDROID__)
    StartTextInput( ::window.get() );
#else
    if( window_focus ) {
        StartTextInput( ::window.get() );
    } else {
        text_input_active_when_regaining_focus = true;
    }
#endif
}

static void focus_aware_stop_text_input()
{
#if defined(__ANDROID__)
    StopTextInput( ::window.get() );
#else
    if( window_focus ) {
        StopTextInput( ::window.get() );
    } else {
        text_input_active_when_regaining_focus = false;
    }
#endif
}


static void CheckMessages()
{
    SDL_Event ev;
    bool quit = false;
    bool text_refresh = false;
    bool is_repeat = false;

    drain_renderer_recovery();

    if( renderer_coordinator.take_repaint_request() ) {


        catacurses::wnoutrefresh( catacurses::stdscr );
        ui_manager::redraw_invalidated();
    }

#if defined(__ANDROID__)
    static uint32_t last_seen_visible_frame_seq = 0;
    const uint32_t cur_visible_frame_seq = visible_frame_inbox.even_sequence();
    if( cur_visible_frame_seq != last_seen_visible_frame_seq ) {
        last_seen_visible_frame_seq = cur_visible_frame_seq;
        needupdate = true;
        ui_manager::redraw_invalidated();
    }
    static uint32_t last_seen_safe_area_seq = 0;
    const uint32_t cur_safe_area_seq = safe_area_inbox.even_sequence();
    if( cur_safe_area_seq != last_seen_safe_area_seq ) {
        last_seen_safe_area_seq = cur_safe_area_seq;

        needupdate = true;
        ui_manager::redraw_invalidated();
        request_terminal_relayout();
    }
    static uint32_t last_seen_outer_size_seq = 0;
    const uint32_t cur_outer_size_seq = outer_size_inbox.even_sequence();
    if( cur_outer_size_seq != last_seen_outer_size_seq ) {
        last_seen_outer_size_seq = cur_outer_size_seq;

        request_terminal_relayout();
    }

    uint32_t ticks = GetTicks();


    if( android_is_hardware_keyboard_available() && !IsTextInputActive( ::window.get() ) ) {
        focus_aware_start_text_input();
    }


    if( needs_sdl_surface_visibility_refresh ) {
        needs_sdl_surface_visibility_refresh = false;


        JNIEnv *env = ( JNIEnv * )GetAndroidJNIEnv();
        jobject activity = ( jobject )GetAndroidActivity();
        jclass clazz( env->GetObjectClass( activity ) );
        jmethodID method_id = env->GetMethodID( clazz, "show_sdl_surface", "()V" );
        env->CallVoidMethod( activity, method_id );
        env->DeleteLocalRef( activity );
        env->DeleteLocalRef( clazz );
    }



    if( !a11y::enabled() ) {

    input_context *new_input_context = input_context::input_context_stack.back();


    if( new_input_context &&
        !touch_joystick::is_bare_context( new_input_context->get_registered_actions() ) &&
        *new_input_context != touch_input_context ) {


        if( touch_input_context.allow_text_entry &&
            !android_wants_text_input( *new_input_context ) &&
            IsTextInputActive( ::window.get() ) &&
            get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
            focus_aware_stop_text_input();
        }

        touch_input_context = *new_input_context;
        needupdate = true;
        ui_manager::redraw_invalidated();
    }

    }
    bool is_default_mode = touch_input_context.get_category() == "DEFAULTMODE";
    quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name(
                                 touch_input_context.get_category() )];

    if( !a11y::enabled() ) {

    if( !needupdate ) {



        if( is_default_mode && get_option<bool>( "ANDROID_SHORTCUT_AUTOADD" ) ) {
            static int last_moves_since_last_save = -1;
            if( last_moves_since_last_save != g->get_moves_since_last_save() ) {
                last_moves_since_last_save = g->get_moves_since_last_save();


                std::set<action_id> actions;


                std::set<action_id> actions_remove;

                Character &player_character = get_player_character();

                if( !player_character.get_hostile_creatures( MAX_VIEW_DISTANCE ).empty() ) {

                    if( !player_character.controlling_vehicle ) {
                        actions.insert( ACTION_CYCLE_MOVE );
                    }

                    item_location wielded = player_character.get_wielded_item();
                    if( wielded ) {
                        if( wielded->is_gun() || wielded->has_flag( flag_REACH_ATTACK ) ) {
                            actions.insert( ACTION_FIRE );
                        }
                    }
                }


                if( player_character.is_running() ) {
                    actions.insert( ACTION_TOGGLE_RUN );
                }

                if( player_character.is_crouching() ) {
                    actions.insert( ACTION_TOGGLE_CROUCH );
                }

                if( player_character.is_prone() ) {
                    actions.insert( ACTION_TOGGLE_PRONE );
                }


                if( std::find( actions.begin(), actions.end(), ACTION_CYCLE_MOVE ) == actions.end() ) {
                    actions_remove.insert( ACTION_CYCLE_MOVE );
                }

                map &here = get_map();


                for( int dx = -1; dx <= 1; dx++ ) {
                    for( int dy = -1; dy <= 1; dy++ ) {
                        int x = player_character.posx( here ) + dx;
                        int y = player_character.posy( here ) + dy;
                        int z = player_character.posz();
                        const tripoint pos( x, y, z );
                        const tripoint_bub_ms bub_pos( pos );

                        {
                            const optional_vpart_position vp = here.veh_at( bub_pos );
                            if( vp ) {
                                if( const std::optional<vpart_reference> controlpart = vp.part_with_feature( "CONTROLS", true ) ) {
                                    actions.insert( ACTION_CONTROL_VEHICLE );
                                }
                                const std::optional<vpart_reference> openablepart = vp.part_with_feature( "OPENABLE", true );
                                if( openablepart && openablepart->part().open && ( dx != 0 ||
                                        dy != 0 ) ) {
                                    actions.insert( ACTION_CLOSE );
                                }
                                const std::optional<vpart_reference> curtainpart = vp.part_with_feature( "CURTAIN", true );
                                if( curtainpart && curtainpart->part().open && ( dx != 0 || dy != 0 ) ) {
                                    actions.insert( ACTION_CLOSE );
                                }
                                const std::optional<vpart_reference> cargopart = vp.cargo();
                                if( cargopart && ( !cargopart->items().empty() ) ) {
                                    actions.insert( ACTION_PICKUP );
                                }
                            }
                        }

                        if( dx != 0 || dy != 0 ) {




                            if( can_interact_at( ACTION_CLOSE, here, bub_pos ) ) {
                                actions.insert( ACTION_CLOSE );
                            }
                            if( can_interact_at( ACTION_EXAMINE, here, bub_pos ) ) {
                                actions.insert( ACTION_EXAMINE );
                            }
                        } else {

                            if( can_interact_at( ACTION_BUTCHER, here, bub_pos ) ) {
                                actions.insert( ACTION_BUTCHER );
                            } else {
                                actions_remove.insert( ACTION_BUTCHER );
                            }

                            if( can_interact_at( ACTION_MOVE_UP, here, bub_pos ) ) {
                                actions.insert( ACTION_MOVE_UP );
                            } else {
                                actions_remove.insert( ACTION_MOVE_UP );
                            }

                            if( can_interact_at( ACTION_MOVE_DOWN, here, bub_pos ) ) {
                                actions.insert( ACTION_MOVE_DOWN );
                            } else {
                                actions_remove.insert( ACTION_MOVE_DOWN );
                            }
                        }


                        if( can_interact_at( ACTION_PICKUP, here, bub_pos ) ) {
                            actions.insert( ACTION_PICKUP );
                        }
                    }
                }


                if( std::find( actions.begin(), actions.end(), ACTION_CONTROL_VEHICLE ) == actions.end() ) {
                    actions_remove.insert( ACTION_CONTROL_VEHICLE );
                }


                if( std::find( actions.begin(), actions.end(), ACTION_CLOSE ) == actions.end() ) {
                    actions_remove.insert( ACTION_CLOSE );
                }


                if( std::find( actions.begin(), actions.end(), ACTION_EXAMINE ) == actions.end() ) {
                    actions_remove.insert( ACTION_EXAMINE );
                }


                if( std::find( actions.begin(), actions.end(), ACTION_PICKUP ) == actions.end() ) {
                    actions_remove.insert( ACTION_PICKUP );
                }


                if( g && !g->check_safe_mode_allowed( false ) ) {
                    actions.insert( ACTION_IGNORE_ENEMY );
                    actions.insert( ACTION_TOGGLE_SAFEMODE );
                } else {
                    actions_remove.insert( ACTION_IGNORE_ENEMY );
                    actions_remove.insert( ACTION_TOGGLE_SAFEMODE );
                }


                if( player_character.get_hunger() > 100 || player_character.get_thirst() > 40 ) {
                    actions.insert( ACTION_EAT );
                }


                if( player_character.get_sleepiness() > sleepiness_levels::DEAD_TIRED ) {
                    actions.insert( ACTION_SLEEP );
                }

                for( const auto &action : actions ) {
                    if( add_best_key_for_action_to_quick_shortcuts( action, touch_input_context.get_category(),
                            !get_option<bool>( "ANDROID_SHORTCUT_AUTOADD_FRONT" ) ) ) {
                        needupdate = true;
                    }
                }

                size_t old_size = qsl.size();
                for( const auto &action_remove : actions_remove ) {
                    remove_action_from_quick_shortcuts( action_remove, touch_input_context.get_category() );
                }
                if( qsl.size() != old_size ) {
                    needupdate = true;
                }
            }
        }

        if( remove_expired_actions_from_quick_shortcuts( touch_input_context.get_category() ) ) {
            needupdate = true;
        }


        if( ac_back_down_time > 0 &&
            ticks - ac_back_down_time > static_cast<uint32_t>
            ( get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) {
            if( !quick_shortcuts_toggle_handled ) {
                quick_shortcuts_enabled = !quick_shortcuts_enabled;

                request_terminal_relayout();
                quick_shortcuts_toggle_handled = true;
                android_request_repaint();
                refresh_display();


                {
                    JNIEnv *env = ( JNIEnv * )GetAndroidJNIEnv();
                    jobject activity = ( jobject )GetAndroidActivity();
                    jclass clazz( env->GetObjectClass( activity ) );
                    jstring toast_message = env->NewStringUTF( quick_shortcuts_enabled ? "Shortcuts visible" :
                                            "Shortcuts hidden" );
                    jmethodID method_id = env->GetMethodID( clazz, "toast", "(Ljava/lang/String;)V" );
                    env->CallVoidMethod( activity, method_id, toast_message );
                    env->DeleteLocalRef( activity );
                    env->DeleteLocalRef( clazz );
                }
            }
        }


        if( !is_quick_shortcut_touch && !is_two_finger_touch && !is_three_finger_touch &&
            finger_down_time > 0 &&
            ticks - finger_down_time > static_cast<uint32_t>
            ( get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) {


            if( ticks - finger_repeat_time > finger_repeat_delay && !HasEvent( CATA_FINGERCANCELED ) ) {
                handle_finger_input( ticks );
                finger_repeat_time = ticks;

                while( SDL_PollEvent( &ev ) ) {
                    if( ev.type == CATA_FINGERUP ) {
                        reset_touch_state();
                        finger_slot_clear( GetFingerID( ev ) );

                        break;
                    }
                    if( ev.type == CATA_FINGERCANCELED ) {
                        cancel_touch();
                        break;
                    }
                }
                return;
            }
        }


        if( !is_quick_shortcut_touch && !is_two_finger_touch && !is_three_finger_touch &&
            last_tap_time > 0 &&
            ticks - last_tap_time >= static_cast<uint32_t>
            ( get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) {

            last_tap_time = ticks;
            last_input = input_event( is_default_mode ? get_key_event_from_string(
                                          get_option<std::string>( "ANDROID_TAP_KEY" ) ) : '\n', input_event_t::keyboard_char );
            last_tap_time = 0;
            return;
        }


        if( is_quick_shortcut_touch && finger_down_time > 0 &&
            ticks - finger_down_time > static_cast<uint32_t>
            ( get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) {
            needupdate = true;
        }
    }
    } else {
        finger_down_time = 0;
        last_tap_time = 0;
        ac_back_down_time = 0;
        is_quick_shortcut_touch = false;
    }
#endif

    last_input = input_event();

#if defined(__ANDROID__)
    static std::uint64_t last_reader_epoch = 0;
    const std::uint64_t reader_epoch = a11y::android::lifecycle_epoch();
    if( reader_epoch != last_reader_epoch ) {
        last_reader_epoch = reader_epoch;

        last_input.type = input_event_t::timeout;
    }
#endif

    using cata::options::mouse;

    int imgui_buf_w = 0;
    int imgui_buf_h = 0;
    get_display_buffer_dims( &imgui_buf_w, &imgui_buf_h );
    while( SDL_PollEvent( &ev ) ) {
#if defined(__ANDROID__)


        if( a11y::enabled() && ( ev.type == CATA_FINGERDOWN ||
                                ev.type == CATA_FINGERMOTION || ev.type == CATA_FINGERUP ||
                                ev.type == CATA_FINGERCANCELED || ev.type == CATA_MOUSEBUTTONDOWN ||
                                ev.type == CATA_MOUSEBUTTONUP || ev.type == CATA_MOUSEMOTION ||
                                ev.type == CATA_MOUSEWHEEL ) ) {
            continue;
        }
#endif



        SDL_Event ev_display = ev;
        convert_event_to_display_buffer_coords( &ev_display );
        imclient->process_input( &ev_display, imgui_buf_w, imgui_buf_h, scaling_factor );



        if( IsWindowEvent( ev ) ) {
            switch( GetWindowEventID( ev ) ) {
#if defined(__ANDROID__)





                case CATA_WINDOWEVENT_FOCUS_LOST:
                    if( world_generator &&
                        world_generator->active_world &&
                        g && g->uquit == QUIT_NO &&
                        get_option<bool>( "ANDROID_QUICKSAVE" ) &&
                        !std::uncaught_exception() ) {
                        g->quicksave();
                    }
                    break;


#else
                case CATA_WINDOWEVENT_FOCUS_LOST:
                    window_focus = false;
                    if( IsTextInputActive( ::window.get() ) ) {
                        text_input_active_when_regaining_focus = true;

                        StopTextInput( ::window.get() );

                        last_input = input_event();
                        last_input.type = input_event_t::keyboard_char;
                        last_input.edit.clear();
                        last_input.edit_refresh = true;
                        text_refresh = true;
                    } else {
                        text_input_active_when_regaining_focus = false;
                    }
                    break;
                case CATA_WINDOWEVENT_FOCUS_GAINED:
                    window_focus = true;

                    if( text_input_active_when_regaining_focus ) {
                        StartTextInput( ::window.get() );
                    }
                    break;
#endif
                case CATA_WINDOWEVENT_SHOWN:
                case CATA_WINDOWEVENT_MINIMIZED:
                    break;
                case CATA_WINDOWEVENT_EXPOSED:
                    needupdate = true;
                    break;
                case CATA_WINDOWEVENT_SAFE_AREA_CHANGED:


                    needupdate = true;
                    ui_manager::redraw_invalidated();
#if defined(__ANDROID__)

                    request_terminal_relayout();
#endif
                    break;
#if defined(__ANDROID__)
                case CATA_WINDOWEVENT_RESTORED:
                    needs_sdl_surface_visibility_refresh = true;
                    if( android_is_hardware_keyboard_available() ) {
                        focus_aware_stop_text_input();
                        focus_aware_start_text_input();
                    }
                    break;
#endif
                default:
                    break;
            }
        } else {
            switch( ev.type ) {
                case CATA_KEYDOWN: {
#if defined(__ANDROID__)

                    if( GetKeysym( ev ).sym == SDLK_AC_BACK && ac_back_down_time == 0 ) {
                        ac_back_down_time = ticks;
                        quick_shortcuts_toggle_handled = false;
                    }
#endif
                    is_repeat = ev.key.repeat;

                    if( get_option<std::string>( "HIDE_CURSOR" ) != "show" && IsCursorVisible() ) {
                        HideCursor();
                    }
                    keyboard_mode mode = keyboard_mode::keychar;
#if !defined(__ANDROID__) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE == 1)
                    if( !IsTextInputActive( ::window.get() ) ) {
                        mode = keyboard_mode::keycode;
                    }
#endif
                    if( mode == keyboard_mode::keychar ) {
                        const int lc = sdl_keysym_to_curses( GetKeysym( ev ) );
                        if( lc <= 0 ) {

                            break;
                        } else if( add_alt_code( lc ) ) {

                        } else {
                            last_input = input_event( lc, input_event_t::keyboard_char );
#if defined(__ANDROID__)
                            if( !android_is_hardware_keyboard_available() ) {
                                if( !android_wants_text_input( touch_input_context ) ) {
                                    if( get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
                                        focus_aware_stop_text_input();
                                    }


                                    if( !last_input.text.empty() ||
                                        !inp_mngr.get_keyname( lc, input_event_t::keyboard_char ).empty() ) {
                                        qsl.remove( last_input );
                                        add_quick_shortcut( qsl, last_input, false, true );
                                        android_request_repaint();
                                        refresh_display();
                                    }
                                } else if( lc == '\n' || lc == KEY_ESCAPE ) {
                                    if( get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
                                        focus_aware_stop_text_input();
                                    }
                                }
                            }
#endif
                        }
                    } else {
                        last_input = sdl_keysym_to_keycode_evt( GetKeysym( ev ) );
                    }
                }
                break;
                case CATA_KEYUP: {
#if defined(__ANDROID__)

                    if( GetKeysym( ev ).sym == SDLK_AC_BACK ) {
                        if( ticks - ac_back_down_time <= static_cast<uint32_t>
                            ( get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) {
                            if( cataimgui::client::want_text_input() ) {



                                cataimgui::client::clear_text_focus();
                            } else if( IsTextInputActive( ::window.get() ) ) {
                                focus_aware_stop_text_input();
                            } else {
                                focus_aware_start_text_input();
                            }
                            android_request_repaint();
                        }
                        ac_back_down_time = 0;
                    }
#endif
                    keyboard_mode mode = keyboard_mode::keychar;
#if !defined(__ANDROID__) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE == 1)
                    if( !IsTextInputActive( ::window.get() ) ) {
                        mode = keyboard_mode::keycode;
                    }
#endif
                    is_repeat = ev.key.repeat;
                    if( mode == keyboard_mode::keychar ) {
                        if( GetKeysym( ev ).sym == SDLK_LALT || GetKeysym( ev ).sym == SDLK_RALT ) {
                            int code = end_alt_code();
                            if( code ) {
                                last_input = input_event( code, input_event_t::keyboard_char );
                                last_input.text = utf32_to_utf8( code );
                            }
                        }
                    } else if( is_repeat ) {
                        last_input = sdl_keysym_to_keycode_evt( GetKeysym( ev ) );
                    }
                }
                break;
                case CATA_TEXTINPUT:
                    if( !add_alt_code( *ev.text.text ) ) {
                        if( strlen( ev.text.text ) > 0 ) {
                            const unsigned lc = UTF8_getch( ev.text.text );
                            last_input = input_event( lc, input_event_t::keyboard_char );
#if defined(__ANDROID__)
                            if( !android_is_hardware_keyboard_available() ) {
                                if( !android_wants_text_input( touch_input_context ) ) {
                                    if( get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
                                        focus_aware_stop_text_input();
                                    }

                                    quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name(
                                                                 touch_input_context.get_category() )];
                                    qsl.remove( last_input );
                                    add_quick_shortcut( qsl, last_input, false, true );
                                    android_request_repaint();
                                    refresh_display();
                                } else if( lc == '\n' || lc == KEY_ESCAPE ) {
                                    if( get_option<bool>( "ANDROID_AUTO_KEYBOARD" ) ) {
                                        focus_aware_stop_text_input();
                                    }
                                }
                            }
#endif
                        } else {

                            last_input = input_event();
                            last_input.type = input_event_t::keyboard_char;
                        }
                        last_input.text = ev.text.text;
                        text_refresh = true;
                    }
                    break;
                case CATA_TEXTEDITING: {
                    if( strlen( ev.edit.text ) > 0 ) {
                        const unsigned lc = UTF8_getch( ev.edit.text );
                        last_input = input_event( lc, input_event_t::keyboard_char );
                    } else {

                        last_input = input_event();
                        last_input.type = input_event_t::keyboard_char;
                    }


                    last_input.edit = std::string( ev.edit.text );
                    last_input.edit_refresh = true;
                    text_refresh = true;
                    break;
                }
#if defined(SDL_HINT_IME_SUPPORT_EXTENDED_TEXT)
                case SDL_TEXTEDITING_EXT: {
                    if( !ev.editExt.text ) {
                        break;
                    }
                    if( strlen( ev.editExt.text ) > 0 ) {
                        const unsigned lc = UTF8_getch( ev.editExt.text );
                        last_input = input_event( lc, input_event_t::keyboard_char );
                    } else {

                        last_input = input_event();
                        last_input.type = input_event_t::keyboard_char;
                    }


                    last_input.edit = std::string( ev.editExt.text );
                    last_input.edit_refresh = true;
                    text_refresh = true;
                    SDL_free( ev.editExt.text );
                    break;
                }
#endif
                case CATA_MOUSEMOTION:
                    if( ! mouse.enabled ) {
                        break;
                    }
                    if( get_option<std::string>( "HIDE_CURSOR" ) == "show" ||
                        get_option<std::string>( "HIDE_CURSOR" ) == "hidekb" ) {



                        if( !IsCursorVisible() ) {
                            ShowCursor();
                        }


                        if( !cataimgui::client::want_capture_mouse() ) {
                            last_input = input_event( MouseInput::Move, input_event_t::mouse );
                        }
                    }
                    break;

                case CATA_MOUSEBUTTONDOWN:
                    if( ! mouse.enabled ) {
                        break;
                    }
                    if( cataimgui::client::want_capture_mouse() ) {
                        break;
                    }
                    switch( ev.button.button ) {
                        case SDL_BUTTON_LEFT:
                            last_input = input_event( MouseInput::LeftButtonPressed, input_event_t::mouse );
                            break;
                        case SDL_BUTTON_RIGHT:
                            last_input = input_event( MouseInput::RightButtonPressed, input_event_t::mouse );
                            break;
                        case SDL_BUTTON_X1:
                            last_input = input_event( MouseInput::X1ButtonPressed, input_event_t::mouse );
                            break;
                        case SDL_BUTTON_X2:
                            last_input = input_event( MouseInput::X2ButtonPressed, input_event_t::mouse );
                            break;
                    }
                    break;

                case CATA_MOUSEBUTTONUP:
                    if( ! mouse.enabled ) {
                        break;
                    }
                    if( cataimgui::client::want_capture_mouse() ) {
                        break;
                    }
                    switch( ev.button.button ) {
                        case SDL_BUTTON_LEFT:
                            last_input = input_event( MouseInput::LeftButtonReleased, input_event_t::mouse );
                            break;
                        case SDL_BUTTON_RIGHT:
                            last_input = input_event( MouseInput::RightButtonReleased, input_event_t::mouse );
                            break;
                        case SDL_BUTTON_X1:
                            last_input = input_event( MouseInput::X1ButtonReleased, input_event_t::mouse );
                            break;
                        case SDL_BUTTON_X2:
                            last_input = input_event( MouseInput::X2ButtonReleased, input_event_t::mouse );
                            break;
                    }
                    break;

                case CATA_MOUSEWHEEL:
                    if( ! mouse.enabled ) {
                        break;
                    }
                    if( cataimgui::client::want_capture_mouse() ) {
                        break;
                    }
                    if( ev.wheel.y > 0 ) {
                        last_input = input_event( MouseInput::ScrollWheelUp, input_event_t::mouse );
                    } else if( ev.wheel.y < 0 ) {
                        last_input = input_event( MouseInput::ScrollWheelDown, input_event_t::mouse );
                    }
                    break;

#if defined(__ANDROID__)
                case CATA_FINGERMOTION: {
                    const int slot = finger_slot_for( GetFingerID( ev ), false );
                    if( slot == 0 ) {
                        if( !is_quick_shortcut_touch ) {
                            update_finger_repeat_delay();
                        }
                        needupdate = true;
                        ui_manager::redraw_invalidated();
                        finger_curr_x = GetFingerX( ev, WindowWidth );
                        finger_curr_y = GetFingerY( ev, WindowHeight );

                        if( get_option<bool>( "ANDROID_VIRTUAL_JOYSTICK_FOLLOW" ) && !is_two_finger_touch &&
                            !is_three_finger_touch ) {

                            float delta_x = finger_curr_x - finger_down_x;
                            float delta_y = finger_curr_y - finger_down_y;
                            float dist = std::sqrt( delta_x * delta_x + delta_y * delta_y );
                            float max_dist = android_touch_deadzone() + android_touch_repeat_range();
                            if( dist > max_dist ) {
                                float delta_ratio = ( dist / max_dist ) - 1.0f;
                                finger_down_x += delta_x * delta_ratio;
                                finger_down_y += delta_y * delta_ratio;
                            }
                        }

                    } else if( slot == 1 ) {
                        second_finger_curr_x = GetFingerX( ev, WindowWidth );
                        second_finger_curr_y = GetFingerY( ev, WindowHeight );
                    } else if( slot == 2 ) {
                        third_finger_curr_x = GetFingerX( ev, WindowWidth );
                        third_finger_curr_y = GetFingerY( ev, WindowHeight );
                    }
                    break;
                }
                case CATA_FINGERDOWN:
                    if( finger_slot_for( GetFingerID( ev ), true ) == 0 ) {
                        finger_down_x = finger_curr_x = GetFingerX( ev, WindowWidth );
                        finger_down_y = finger_curr_y = GetFingerY( ev, WindowHeight );
                        finger_down_time = ticks;
                        finger_repeat_time = 0;
                        is_quick_shortcut_touch = get_quick_shortcut_under_finger() != NULL;
                        if( !is_quick_shortcut_touch ) {
                            update_finger_repeat_delay();
                        }
                        ui_manager::redraw_invalidated();
                        needupdate = true;
                    } else if( finger_slot_for( GetFingerID( ev ), true ) == 1 ) {
                        if( !is_quick_shortcut_touch ) {
                            second_finger_down_x = second_finger_curr_x = GetFingerX( ev, WindowWidth );
                            second_finger_down_y = second_finger_curr_y = GetFingerY( ev, WindowHeight );
                            is_two_finger_touch = true;
                        }
                    } else if( finger_slot_for( GetFingerID( ev ), true ) == 2 ) {
                        if( !is_quick_shortcut_touch ) {
                            third_finger_down_x = third_finger_curr_x = GetFingerX( ev, WindowWidth );
                            third_finger_down_y = third_finger_curr_y = GetFingerY( ev, WindowHeight );
                            is_three_finger_touch = true;
                            is_two_finger_touch = false;
                        }
                    }
                    break;
                case CATA_FINGERUP: {
                    const int slot = finger_slot_for( GetFingerID( ev ), false );
                    if( slot == 0 ) {
                        finger_curr_x = GetFingerX( ev, WindowWidth );
                        finger_curr_y = GetFingerY( ev, WindowHeight );
                        if( is_quick_shortcut_touch ) {
                            input_event *quick_shortcut = get_quick_shortcut_under_finger();
                            if( quick_shortcut ) {
                                last_input = *quick_shortcut;
                                if( get_option<bool>( "ANDROID_SHORTCUT_MOVE_FRONT" ) ) {
                                    quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name(
                                                                 touch_input_context.get_category() )];
                                    reorder_quick_shortcut( qsl, quick_shortcut->get_first_input(), false );
                                }
                                quick_shortcut->shortcut_last_used_action_counter = g->get_user_action_counter();
                            } else {

                                quick_shortcut = get_quick_shortcut_under_finger( true );
                                if( quick_shortcut &&
                                    ticks - finger_down_time <= static_cast<uint32_t>( get_option<int>( "ANDROID_INITIAL_DELAY" ) )
                                    &&
                                    finger_curr_y < finger_down_y &&
                                    finger_down_y - finger_curr_y > std::abs( finger_down_x - finger_curr_x ) ) {

                                    quick_shortcuts_t &qsl = quick_shortcuts_map[get_quick_shortcut_name(
                                                                 touch_input_context.get_category() )];
                                    qsl.remove( *quick_shortcut );
                                }
                            }
                        } else {
                            if( is_two_finger_touch ) {

                                if( is_default_mode ) {
                                    float x1 = ( finger_curr_x - finger_down_x );
                                    float y1 = ( finger_curr_y - finger_down_y );
                                    float d1 = std::sqrt( x1 * x1 + y1 * y1 );

                                    float x2 = ( second_finger_curr_x - second_finger_down_x );
                                    float y2 = ( second_finger_curr_y - second_finger_down_y );
                                    float d2 = std::sqrt( x2 * x2 + y2 * y2 );

                                    float longest_window_edge = std::max( WindowWidth, WindowHeight );

                                    if( std::max( d1, d2 ) < get_option<float>( "ANDROID_DEADZONE_RANGE" ) * longest_window_edge ) {
                                        last_input = input_event( get_key_event_from_string(
                                                                      get_option<std::string>( "ANDROID_2_TAP_KEY" ) ), input_event_t::keyboard_char );
                                    } else {
                                        float dot = ( x1 * x2 + y1 * y2 ) / ( d1 * d2 );
                                        if( dot > 0.0f ) {
                                            float dratio = d1 / d2;
                                            const float dist_ratio = 0.3f;
                                            if( dratio > dist_ratio &&
                                                dratio < ( 1.0f /
                                                           dist_ratio ) ) {
                                                float xavg = 0.5f * ( x1 + x2 );
                                                float yavg = 0.5f * ( y1 + y2 );
                                                if( xavg > 0 && xavg > std::abs( yavg ) ) {
                                                    last_input = input_event( get_key_event_from_string(
                                                                                  get_option<std::string>( "ANDROID_2_SWIPE_LEFT_KEY" ) ), input_event_t::keyboard_char );
                                                } else if( xavg < 0 && -xavg > std::abs( yavg ) ) {
                                                    last_input = input_event( get_key_event_from_string(
                                                                                  get_option<std::string>( "ANDROID_2_SWIPE_RIGHT_KEY" ) ), input_event_t::keyboard_char );
                                                } else if( yavg > 0 && yavg > std::abs( xavg ) ) {
                                                    last_input = input_event( get_key_event_from_string(
                                                                                  get_option<std::string>( "ANDROID_2_SWIPE_DOWN_KEY" ) ), input_event_t::keyboard_char );
                                                } else {
                                                    last_input = input_event( get_key_event_from_string(
                                                                                  get_option<std::string>( "ANDROID_2_SWIPE_UP_KEY" ) ), input_event_t::keyboard_char );
                                                }
                                            }
                                        } else {

                                            float down_x = finger_down_x - second_finger_down_x;
                                            float down_y = finger_down_y - second_finger_down_y;
                                            float down_dist = std::sqrt( down_x * down_x + down_y * down_y );

                                            float curr_x = finger_curr_x - second_finger_curr_x;
                                            float curr_y = finger_curr_y - second_finger_curr_y;
                                            float curr_dist = std::sqrt( curr_x * curr_x + curr_y * curr_y );

                                            const float zoom_ratio = 0.9f;
                                            if( curr_dist < down_dist * zoom_ratio ) {
                                                last_input = input_event( get_key_event_from_string(
                                                                              get_option<std::string>( "ANDROID_PINCH_IN_KEY" ) ), input_event_t::keyboard_char );
                                            } else if( curr_dist > down_dist / zoom_ratio ) {
                                                last_input = input_event( get_key_event_from_string(
                                                                              get_option<std::string>( "ANDROID_PINCH_OUT_KEY" ) ), input_event_t::keyboard_char );
                                            }
                                        }
                                    }
                                }
                            } else if( is_three_finger_touch ) {

                                float x1 = ( finger_curr_x - finger_down_x );
                                float y1 = ( finger_curr_y - finger_down_y );
                                float d1 = std::sqrt( x1 * x1 + y1 * y1 );

                                float x2 = ( second_finger_curr_x - second_finger_down_x );
                                float y2 = ( second_finger_curr_y - second_finger_down_y );
                                float d2 = std::sqrt( x2 * x2 + y2 * y2 );

                                float x3 = ( third_finger_curr_x - third_finger_down_x );
                                float y3 = ( third_finger_curr_y - third_finger_down_y );
                                float d3 = std::sqrt( x3 * x3 + y3 * y3 );

                                float longest_window_edge = std::max( WindowWidth, WindowHeight );

                                if( std::max( d1, std::max( d2,
                                                            d3 ) ) < get_option<float>( "ANDROID_DEADZONE_RANGE" ) * longest_window_edge ) {
                                    int three_tap_key = 0;
                                    if( three_tap_key == 0 ) {
                                        quick_shortcuts_enabled = !quick_shortcuts_enabled;
                                        request_terminal_relayout();

                                        quick_shortcuts_toggle_handled = true;


                                        {
                                            JNIEnv *env = ( JNIEnv * )GetAndroidJNIEnv();
                                            jobject activity = ( jobject )GetAndroidActivity();
                                            jclass clazz( env->GetObjectClass( activity ) );
                                            jstring toast_message = env->NewStringUTF( quick_shortcuts_enabled ? "Shortcuts visible" :
                                                                    "Shortcuts hidden" );
                                            jmethodID method_id = env->GetMethodID( clazz, "toast", "(Ljava/lang/String;)V" );
                                            env->CallVoidMethod( activity, method_id, toast_message );
                                            env->DeleteLocalRef( activity );
                                            env->DeleteLocalRef( clazz );
                                        }
                                    } else {
                                        last_input = input_event( three_tap_key, input_event_t::keyboard_char );
                                    }
                                } else {
                                    float dot = ( x1 * x2 + y1 * y2 ) / ( d1 * d2 );
                                    float dot2 = ( x1 * x3 + y1 * y3 ) / ( d1 * d3 );
                                    if( dot > 0.0f &&
                                        dot2 > 0.0f ) {
                                        float dratio = d1 / d2;
                                        const float dist_ratio = 0.3f;
                                        if( dratio > dist_ratio &&
                                            dratio < ( 1.0f /
                                                       dist_ratio ) ) {
                                            float xavg = 0.5f * ( x1 + x2 );
                                            float yavg = 0.5f * ( y1 + y2 );
                                            if( xavg > 0 && xavg > std::abs( yavg ) ) {
                                                last_input = input_event( '\t', input_event_t::keyboard_char );
                                            } else if( xavg < 0 && -xavg > std::abs( yavg ) ) {
                                                last_input = input_event( KEY_BTAB, input_event_t::keyboard_char );
                                            } else if( yavg > 0 && yavg > std::abs( xavg ) ) {
                                                last_input = input_event( KEY_NPAGE, input_event_t::keyboard_char );
                                            } else {
                                                last_input = input_event( KEY_PPAGE, input_event_t::keyboard_char );
                                            }
                                        }
                                    }
                                }

                            } else if( ticks - finger_down_time <= static_cast<uint32_t>(
                                           get_option<int>( "ANDROID_INITIAL_DELAY" ) ) ) {
                                handle_finger_input( ticks );
                            }
                        }
                        reset_touch_state();

                        android_request_repaint();
                        refresh_display();
                    } else if( slot == 1 ) {
                        if( is_two_finger_touch ) {


                            second_finger_curr_x = GetFingerX( ev, WindowWidth );
                            second_finger_curr_y = GetFingerY( ev, WindowHeight );
                        }
                    } else if( slot == 2 ) {
                        if( is_three_finger_touch ) {


                            third_finger_curr_x = GetFingerX( ev, WindowWidth );
                            third_finger_curr_y = GetFingerY( ev, WindowHeight );
                        }
                    }
                    finger_slot_clear( GetFingerID( ev ) );

                    break;
                }
                case CATA_FINGERCANCELED:
                    cancel_touch();
                    android_request_repaint();
                    break;
#endif

                case CATA_QUIT:
                    quit = true;
                    break;
                default:
                    if( gamepad::is_gamepad_event( ev ) ) {
                        if( gamepad::handle_event( ev ) ) {
                            needupdate = true;
                            ui_manager::redraw_invalidated();
                        }
                    }
                    break;
            }
        }
        if( text_refresh && !is_repeat ) {
            break;
        }
    }
    if( needupdate ) {
        try_sdl_update();
    }
    if( quit ) {
        catacurses::endwin();
        exit( 0 );
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


static std::pair<float, float> get_display_scale( int display_index )
{
    SDL_Window_Ptr probe = CreateGameWindow( "probe", display_index, 16, 16,
                           CATA_WINDOW_HIDDEN | CATA_WINDOW_HIGH_DPI );
    if( !probe ) {
        return std::make_pair( 1.0f, 1.0f );
    }

    int lw = 0;
    int lh = 0;
    GetWindowSize( probe.get(), &lw, &lh );
    int pw = 0;
    int ph = 0;
    GetWindowSizeInPixels( probe.get(), &pw, &ph );
    probe.reset();

    float scale_w = lw ? static_cast<float>( pw ) / static_cast<float>( lw ) : 1.0f;
    float scale_h = lh ? static_cast<float>( ph ) / static_cast<float>( lh ) : 1.0f;
    return std::make_pair( scale_w, scale_h );
}

static void init_term_size_and_scaling_factor()
{
    scaling_factor = 1;
#if defined(__ANDROID__)

    point terminal( EVEN_MINIMUM_TERM_WIDTH, EVEN_MINIMUM_TERM_HEIGHT );
#else
    point terminal( get_option<int>( "TERMINAL_X" ), get_option<int>( "TERMINAL_Y" ) );
#endif

#if !defined(__ANDROID__)

    if( get_option<std::string>( "SCALING_FACTOR" ) == "2" ) {
        scaling_factor = 2;
    } else if( get_option<std::string>( "SCALING_FACTOR" ) == "4" ) {
        scaling_factor = 4;
    }

    if( scaling_factor > 1 ) {

        int max_width;
        int max_height;

        int current_display_id = std::stoi( get_option<std::string>( "DISPLAY" ) );
        SDL_DisplayMode current_display;

        if( GetDesktopDisplayMode( current_display_id, &current_display ) ) {
            if( get_option<std::string>( "FULLSCREEN" ) == "no" ) {


                SDL_Window_Ptr test_window = CreateGameWindow(
                                                 "test_window", current_display_id,
                                                 EVEN_MINIMUM_TERM_WIDTH * fontwidth,
                                                 EVEN_MINIMUM_TERM_HEIGHT * fontheight,
                                                 CATA_WINDOW_RESIZABLE | CATA_WINDOW_HIGH_DPI | CATA_WINDOW_MAXIMIZED );

                GetWindowSizeInPixels( test_window.get(), &max_width, &max_height );


                test_window.reset();
                SDL_QuitSubSystem( SDL_INIT_VIDEO );
                SDL_InitSubSystem( SDL_INIT_VIDEO );

            } else {

                auto [ dpi_scale_w, dpi_scale_h ] = get_display_scale( current_display_id );
                max_width = dpi_scale_w * current_display.w;
                max_height = dpi_scale_h * current_display.h;
            }
        } else {
            dbg( D_WARNING ) << "Failed to get current Display Mode, assuming infinite display size.";
            max_width = INT_MAX;
            max_height = INT_MAX;
        }

        if( terminal.x * fontwidth > max_width ||
            EVEN_MINIMUM_TERM_WIDTH * fontwidth * scaling_factor > max_width ) {
            if( EVEN_MINIMUM_TERM_WIDTH * fontwidth * scaling_factor > max_width ) {
                dbg( D_WARNING ) << "SCALING_FACTOR set too high for display size, resetting to 1";
                scaling_factor = 1;
                terminal.x = max_width / fontwidth;
                terminal.y = max_height / fontheight;
                get_options().get_option( "SCALING_FACTOR" ).setValue( "1" );
            } else {
                terminal.x = max_width / fontwidth;
            }
        }

        if( terminal.y * fontheight > max_height ||
            EVEN_MINIMUM_TERM_HEIGHT * fontheight * scaling_factor > max_height ) {
            if( EVEN_MINIMUM_TERM_HEIGHT * fontheight * scaling_factor > max_height ) {
                dbg( D_WARNING ) << "SCALING_FACTOR set too high for display size, resetting to 1";
                scaling_factor = 1;
                terminal.x = max_width / fontwidth;
                terminal.y = max_height / fontheight;
                get_options().get_option( "SCALING_FACTOR" ).setValue( "1" );
            } else {
                terminal.y = max_height / fontheight;
            }
        }

        terminal.x -= terminal.x % scaling_factor;
        terminal.y -= terminal.y % scaling_factor;

        terminal.x = std::max( EVEN_MINIMUM_TERM_WIDTH * scaling_factor, terminal.x );
        terminal.y = std::max( EVEN_MINIMUM_TERM_HEIGHT * scaling_factor, terminal.y );

        get_options().get_option( "TERMINAL_X" ).setValue(
            std::max( EVEN_MINIMUM_TERM_WIDTH * scaling_factor, terminal.x ) );
        get_options().get_option( "TERMINAL_Y" ).setValue(
            std::max( EVEN_MINIMUM_TERM_HEIGHT * scaling_factor, terminal.y ) );

        get_options().save();
    }

#endif

    TERMINAL_WIDTH = terminal.x / scaling_factor;
    TERMINAL_HEIGHT = terminal.y / scaling_factor;
}


void catacurses::init_interface()
{
    last_input = input_event();
    inputdelay = -1;

    InitSDL();

    get_options().init();
    get_options().load();
    set_language_from_options();

    font_loader fl;
    fl.load();
#if !defined(__ANDROID__)

    fl.fontwidth = get_option<int>( "FONT_WIDTH" );
    fl.fontheight = get_option<int>( "FONT_HEIGHT" );
    fl.fontsize = get_option<int>( "FONT_SIZE" );
#endif
    fl.fontblending = get_option<bool>( "FONT_BLENDING" );
    fl.map_fontsize = get_option<int>( "MAP_FONT_SIZE" );
    fl.map_fontwidth = get_option<int>( "MAP_FONT_WIDTH" );
    fl.map_fontheight = get_option<int>( "MAP_FONT_HEIGHT" );
    fl.overmap_fontsize = get_option<int>( "OVERMAP_FONT_SIZE" );
    fl.overmap_fontwidth = get_option<int>( "OVERMAP_FONT_WIDTH" );
    fl.overmap_fontheight = get_option<int>( "OVERMAP_FONT_HEIGHT" );
    ::fontwidth = fl.fontwidth;
    ::fontheight = fl.fontheight;

    init_term_size_and_scaling_factor();

#if defined(__ANDROID__)
    android_fonts = fl;
#endif

    WinCreate();
#if defined(__ANDROID__)

    fl.fontwidth = ::fontwidth;
    fl.fontheight = ::fontheight;
    fl.fontsize = android_font_point_size( point( ::fontwidth, ::fontheight ) );
#endif
    refresh_mouse_config();
    dbg( D_INFO ) << "Initializing SDL Tiles context";
    fartilecontext = std::make_shared<cata_tiles>( renderer, geometry, ts_cache );
    if( use_far_tiles ) {
        try {

            ui_adaptor dummy( ui_adaptor::disable_uis_below{} );
            fartilecontext->load_tileset( get_option<std::string>( "DISTANT_TILES" ),
 true, false,
 true, false );
        } catch( const std::exception &err ) {
            dbg( D_ERROR ) << "failed to check for tileset: " << err.what();



            use_far_tiles = false;
        }
    }
    closetilecontext = std::make_shared<cata_tiles>( renderer, geometry, ts_cache );
    try {

        ui_adaptor dummy( ui_adaptor::disable_uis_below{} );
        closetilecontext->load_tileset( get_option<std::string>( "TILES" ),
 true, false,
 true, false );
        tilecontext = closetilecontext;
    } catch( const std::exception &err ) {
        dbg( D_ERROR ) << "failed to check for tileset: " << err.what();



        use_tiles = false;
    }
    portrait_tilecontext = std::make_shared<cata_tiles>( renderer, geometry, ts_cache );
    try {

        ui_adaptor dummy( ui_adaptor::disable_uis_below{} );
        portrait_tilecontext->load_tileset( get_option<std::string>( "TILES" ),
 true, false,
 true, false );
    } catch( const std::exception &err ) {
        dbg( D_ERROR ) << "failed to check for tileset: " << err.what();
    }
    overmap_tilecontext = std::make_unique<cata_tiles>( renderer, geometry, ts_cache );
    try {

        ui_adaptor dummy( ui_adaptor::disable_uis_below{} );
        overmap_tilecontext->load_tileset( get_option<std::string>( "OVERMAP_TILES" ),
 true, false,
 true, true );
    } catch( const std::exception &err ) {
        dbg( D_ERROR ) << "failed to check for overmap tileset: " << err.what();



        use_tiles = false;
    }

    color_loader<SDL_Color>().load( windowsPalette );
    init_colors();

#if defined(SDL_SOUND)

    if( sound_init_success ) {
        load_soundset();
    }
#endif

    font = std::make_unique<FontFallbackList>( renderer, pixel_format, fl.fontwidth, fl.fontheight,
            windowsPalette, fl.typeface, fl.fontsize, fl.fontblending );
    gui_font = std::make_unique<FontFallbackList>( renderer, pixel_format, fl.fontwidth, fl.fontheight,
               windowsPalette, fl.gui_typeface, fl.fontsize, fl.fontblending );
    map_font = std::make_unique<FontFallbackList>( renderer, pixel_format, fl.map_fontwidth,
               fl.map_fontheight,
               windowsPalette, fl.map_typeface, fl.map_fontsize, fl.fontblending );
    overmap_font = std::make_unique<FontFallbackList>( renderer, pixel_format, fl.overmap_fontwidth,
                   fl.overmap_fontheight,
                   windowsPalette, fl.overmap_typeface, fl.overmap_fontsize, fl.fontblending );
    stdscr = newwin( get_terminal_height(), get_terminal_width(), point::zero );

    imclient->load_fonts( gui_font, font, windowsPalette, fl.gui_typeface, fl.typeface );
#if defined(__ANDROID__)

    renderer_coordinator.notify_resize();
#endif


    renderer_coordinator.finish_bootstrap();
    drain_renderer_recovery();
}


void load_tileset()
{
    if( !tilecontext || !use_tiles ) {
        return;
    }
    closetilecontext->load_tileset( get_option<std::string>( "TILES" ),
 false, false,
 true, false );
    if( use_far_tiles ) {
        fartilecontext->load_tileset( get_option<std::string>( "DISTANT_TILES" ),
 false, false,
 true, false );
    }
    if( use_tiles ) {
        portrait_tilecontext->load_tileset( get_option<std::string>( "PORTRAIT_TILES" ),
 false, false,
 true, false );
    }
    tilecontext = closetilecontext;
    tilecontext->do_tile_loading_report();

    if( overmap_tilecontext ) {
        overmap_tilecontext->load_tileset( get_option<std::string>( "OVERMAP_TILES" ),
 false, false,
 true, true );
        overmap_tilecontext->do_tile_loading_report();
    }
}


void catacurses::endwin()
{
    tilecontext.reset();
    closetilecontext.reset();
    fartilecontext.reset();
    overmap_tilecontext.reset();
    portrait_tilecontext.reset();
    font.reset();
    gui_font.reset();
    map_font.reset();
    overmap_font.reset();
    ui_manager::reset();
    WinDestroy();
}

template<>
SDL_Color color_loader<SDL_Color>::from_rgb( const int r, const int g, const int b )
{
    SDL_Color result;
    result.b = b;
    result.g = g;
    result.r = r;
    result.a = 0xFF;
    return result;
}

void input_manager::set_timeout( const int delay )
{
    input_timeout = delay;
    inputdelay = delay;
}

void input_manager::pump_events()
{
    if( test_mode ) {
        return;
    }


    CheckMessages();

    last_input = input_event();
    previously_pressed_key = 0;
}



#if !defined(EMSCRIPTEN)

static std::optional<uint32_t> android_touch_wait_ms( const uint32_t now )
{
#if defined(__ANDROID__)
    touch_timers t;
    t.now = now;
    t.initial_delay = static_cast<uint32_t>( get_option<int>( "ANDROID_INITIAL_DELAY" ) );
    t.finger_down_time = finger_down_time;
    t.finger_repeat_time = finger_repeat_time;
    t.finger_repeat_delay = finger_repeat_delay;
    t.last_tap_time = last_tap_time;
    t.back_down_time = ac_back_down_time;
    t.back_toggle_handled = quick_shortcuts_toggle_handled;
    t.quick_shortcut_touch = is_quick_shortcut_touch;
    t.multi_finger_touch = is_two_finger_touch || is_three_finger_touch;
    t.last_present = lastupdate;
    t.present_interval = interval;
    return touch_wait_ms( t );
#else
    static_cast<void>( now );
    return std::nullopt;
#endif
}


static int input_wait_ms( const std::optional<uint32_t> &input_ms )
{
    const uint32_t now = GetTicks();
    input_wait_state s;
    s.input_ms = input_ms;
    if( needupdate ) {

        s.present_ms = ms_until_elapsed_reaches( lastupdate, now, interval );
    }
    s.platform_ms = android_touch_wait_ms( now );
    return input_wait_timeout_ms( s );
}
#endif

input_event input_manager::get_input_event( const keyboard_mode preferred_keyboard_mode )
{
    if( test_mode ) {

        throw std::runtime_error( "input_manager::get_input_event called in test mode" );
    }




    SDL_PumpEvents();
    drain_renderer_recovery();

#if !defined(__ANDROID__) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE == 1)
    if( actual_keyboard_mode( preferred_keyboard_mode ) == keyboard_mode::keychar ) {
        focus_aware_start_text_input();
    } else {
        focus_aware_stop_text_input();
    }
#else

    ( void ) preferred_keyboard_mode;
#endif

    previously_pressed_key = 0;







    wnoutrefresh( catacurses::stdscr );

    if( needupdate ) {
        refresh_display();
    } else if( ui_adaptor::has_imgui() ) {
        try_sdl_update();
    }

#if defined(EMSCRIPTEN)

    if( inputdelay < 0 ) {
        do {
            CheckMessages();
            if( last_input.type != input_event_t::error ) {
                break;
            }
            SDL_Delay( 1 );
        } while( last_input.type == input_event_t::error );
    } else if( inputdelay > 0 ) {
        uint32_t starttime = GetTicks();
        uint32_t endtime = 0;
        bool timedout = false;
        do {
            CheckMessages();
            endtime = GetTicks();
            if( last_input.type != input_event_t::error ) {
                break;
            }
            SDL_Delay( 1 );
            timedout = endtime >= starttime + inputdelay;
            if( timedout ) {
                last_input.type = input_event_t::timeout;
            }
        } while( !timedout );
    } else {
        CheckMessages();
    }
#else
    if( inputdelay < 0 ) {
        CheckMessages();
        while( last_input.type == input_event_t::error ) {
            WaitForEvent( input_wait_ms( std::nullopt ) );
            CheckMessages();
        }
    } else if( inputdelay > 0 ) {
        const uint32_t starttime = GetTicks();
        const uint32_t timeout = static_cast<uint32_t>( inputdelay );
        CheckMessages();
        while( last_input.type == input_event_t::error ) {
            const uint32_t left = ms_until_elapsed_reaches( starttime, GetTicks(), timeout );
            if( left == 0 ) {
                last_input.type = input_event_t::timeout;
                break;
            }
            WaitForEvent( input_wait_ms( left ) );
            CheckMessages();
        }
    } else {
        CheckMessages();
    }
#endif




    {
        point raw;
        GetMouseState( &raw.x, &raw.y );
        const SDL_Point buf_pt = window_to_display_buffer_coords( SDL_Point{ raw.x, raw.y } );
        last_input.mouse_pos.x = buf_pt.x;
        last_input.mouse_pos.y = buf_pt.y;
    }
    if( last_input.type == input_event_t::keyboard_char ) {
        previously_pressed_key = last_input.get_first_input();
#if defined(__ANDROID__)
        android_vibrate();
#endif
    }
#if defined(__ANDROID__)
    else if( last_input.type == input_event_t::gamepad ) {
        android_vibrate();
    }
#endif

    return last_input;
}

bool gamepad_available()
{
    return gamepad::is_active();
}

void rescale_tileset( int size )
{

    if( size <= pow( 2, get_option<int>( "SWAP_ZOOM" ) + 1 ) && use_far_tiles ) {
        tilecontext = fartilecontext;
        g->mark_main_ui_adaptor_resize();
    } else {
        tilecontext = closetilecontext;
        g->mark_main_ui_adaptor_resize();
    }
    tilecontext->set_draw_scale( size );
}

static window_dimensions get_window_dimensions( const catacurses::window &win,
        const point &pos, const point &size )
{
    window_dimensions dim;
    if( use_tiles && g && win == g->w_terrain ) {

        dim.scaled_font_size.x = tilecontext->get_tile_width();
        dim.scaled_font_size.y = tilecontext->get_tile_height();
    } else if( map_font && g && win == g->w_terrain ) {

        dim.scaled_font_size.x = map_font->width;
        dim.scaled_font_size.y = map_font->height;
    } else if( overmap_font && g && win == g->w_overmap ) {
        if( use_tiles && use_tiles_overmap ) {

            dim.scaled_font_size.x = overmap_tilecontext->get_tile_width();
            dim.scaled_font_size.y = overmap_tilecontext->get_tile_height();
        } else {
            dim.scaled_font_size.x = overmap_font->width;
            dim.scaled_font_size.y = overmap_font->height;
        }
    } else {
        dim.scaled_font_size.x = fontwidth;
        dim.scaled_font_size.y = fontheight;
    }


    dim.scaled_font_size *= get_scaling_factor();

    if( win ) {
        cata_cursesport::WINDOW *const pwin = win.get<cata_cursesport::WINDOW>();
        dim.window_pos_cell = pwin->pos;
        dim.window_size_cell.x = pwin->width;
        dim.window_size_cell.y = pwin->height;
    } else {
        dim.window_pos_cell = pos;
        dim.window_size_cell = size;
    }


    dim.window_pos_pixel = point( dim.window_pos_cell.x * fontwidth,
                                  dim.window_pos_cell.y * fontheight );

    if( use_tiles && g && win == g->w_terrain ) {

        dim.window_size_pixel.x = TERRAIN_WINDOW_TERM_WIDTH * fontwidth;
        dim.window_size_pixel.y = TERRAIN_WINDOW_TERM_HEIGHT * fontheight;
    } else if( use_tiles && use_tiles_overmap && g && win == g->w_overmap ) {

        dim.window_size_pixel.x = OVERMAP_WINDOW_TERM_WIDTH * fontwidth;
        dim.window_size_pixel.y = OVERMAP_WINDOW_TERM_HEIGHT * fontheight;
    } else {

        dim.window_size_pixel.x = dim.window_size_cell.x * dim.scaled_font_size.x;
        dim.window_size_pixel.y = dim.window_size_cell.y * dim.scaled_font_size.y;
    }

    return dim;
}


int get_window_width()
{
    return WindowWidth;
}

int get_window_height()
{
    return WindowHeight;
}

window_dimensions get_window_dimensions( const catacurses::window &win )
{
    return get_window_dimensions( win, point::zero, point::zero );
}

window_dimensions get_window_dimensions( const point &pos, const point &size )
{
    return get_window_dimensions( {}, pos, size );
}

std::optional<tripoint_bub_ms> input_context::get_coordinates( const catacurses::window
        &capture_win_, const point &offset, const bool center_cursor ) const
{

    ( void ) center_cursor;

    if( !coordinate_input_received ) {
        return std::nullopt;
    }

    const catacurses::window &capture_win = capture_win_ ? capture_win_ : g->w_terrain;
    const window_dimensions dim = get_window_dimensions( capture_win );

    const point &win_min = dim.window_pos_pixel;
    point win_size = dim.window_size_pixel;
    point logical_coordinate = coordinate;
    const int scaling_factor = get_scaling_factor();


    if( scaling_factor > 1 ) {
        logical_coordinate.x /= scaling_factor;
        logical_coordinate.y /= scaling_factor;

        const bool is_terrain_or_overmap = ( use_tiles && g && capture_win == g->w_terrain ) ||
                                           ( use_tiles && use_tiles_overmap && g && capture_win == g->w_overmap );
        if( !is_terrain_or_overmap ) {
            win_size.x /= scaling_factor;
            win_size.y /= scaling_factor;
        }
    }

    const point win_max = win_min + win_size;



    const half_open_rectangle<point> win_bounds( win_min, win_max );
    if( !win_bounds.contains( logical_coordinate ) ) {
        return std::nullopt;
    }

    const point screen_pos = logical_coordinate - win_min;

    const bool use_isometric = g->w_overmap &&
                               capture_win == g->w_overmap ? false : g->is_tileset_isometric();


    point logical_tile_size;
    if( scaling_factor > 1 ) {
        const bool is_terrain = use_tiles && g && capture_win == g->w_terrain;
        const bool is_overmap = use_tiles && use_tiles_overmap && g && capture_win == g->w_overmap;

        if( is_terrain ) {
            logical_tile_size.x = tilecontext->get_tile_width();
            logical_tile_size.y = tilecontext->get_tile_height();
        } else if( is_overmap ) {
            logical_tile_size.x = overmap_tilecontext->get_tile_width();
            logical_tile_size.y = overmap_tilecontext->get_tile_height();
        } else {
            logical_tile_size = dim.scaled_font_size;
            logical_tile_size.x /= scaling_factor;
            logical_tile_size.y /= scaling_factor;
        }
    } else {
        logical_tile_size = dim.scaled_font_size;
    }

    const point_bub_ms p = cata_tiles::screen_to_player(
                               screen_pos, logical_tile_size, win_size,
                               point_bub_ms( offset ), use_isometric );

    return tripoint_bub_ms( p, get_map().get_abs_sub().z() );
}

int get_terminal_width()
{
    return TERMINAL_WIDTH;
}

int get_terminal_height()
{
    return TERMINAL_HEIGHT;
}

int get_scaling_factor()
{
    return scaling_factor;
}

static int map_font_width()
{
    if( use_tiles && tilecontext ) {
        return tilecontext->get_tile_width();
    }
    return ( map_font ? map_font.get() : font.get() )->width;
}

static int map_font_height()
{
    if( use_tiles && tilecontext ) {
        return tilecontext->get_tile_height();
    }
    return ( map_font ? map_font.get() : font.get() )->height;
}

static int overmap_font_width()
{
    if( use_tiles && overmap_tilecontext && use_tiles_overmap ) {
        return overmap_tilecontext->get_tile_width();
    }
    return ( overmap_font ? overmap_font.get() : font.get() )->width;
}

static int overmap_font_height()
{
    if( use_tiles && overmap_tilecontext && use_tiles_overmap ) {
        return overmap_tilecontext->get_tile_height();
    }
    return ( overmap_font ? overmap_font.get() : font.get() )->height;
}

void to_map_font_dim_width( int &w )
{
    w = ( w * fontwidth ) / map_font_width();
}

void to_map_font_dim_height( int &h )
{
    h = ( h * fontheight ) / map_font_height();
}

void to_map_font_dimension( int &w, int &h )
{
    to_map_font_dim_width( w );
    to_map_font_dim_height( h );
}

void from_map_font_dimension( int &w, int &h )
{
    w = ( w * map_font_width() + fontwidth - 1 ) / fontwidth;
    h = ( h * map_font_height() + fontheight - 1 ) / fontheight;
}

void to_overmap_font_dimension( int &w, int &h )
{
    w = ( w * fontwidth ) / overmap_font_width();
    h = ( h * fontheight ) / overmap_font_height();
}

bool is_draw_tiles_mode()
{
    return use_tiles;
}

bool catacurses::supports_256_colors()
{

    return false;
}





bool save_screenshot( const std::string &file_path )
{


    display_buffer_draw_scope draw_scope;
    if( !draw_scope.should_draw() ) {


        return false;
    }

    SDL_Rect viewport;


    RenderGetViewport( renderer, &viewport );


    SDL_Surface_Ptr surface = CreateRGBSurface( 0, viewport.w, viewport.h, 32, 0, 0, 0, 0 );


    if( printErrorIf( !RenderReadPixels( renderer, nullptr, GetSurfacePixelFormat( surface ),
                                         surface->pixels, surface->pitch ),
                      "save_screenshot: cannot read data from SDL_Renderer." ) ) {
        return false;
    }


    if( printErrorIf( IMG_SavePNG( surface.get(), file_path.c_str() ) != 0,
                      std::string( "save_screenshot: cannot save screenshot file: " + file_path ).c_str() ) ) {
        return false;
    }

    return true;
}

#ifdef _WIN32
HWND getWindowHandle()
{
    return static_cast<HWND>( SDL_GetPointerProperty(
                                  SDL_GetWindowProperties( ::window.get() ),
                                  SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr ) );
}
#endif

void set_title( const std::string &title )
{
    if( ::window ) {
        SetWindowTitle( ::window.get(), title.c_str() );
    }
}

const SDL_Renderer_Ptr &get_sdl_renderer()
{
    return renderer;
}

SDL_Window *get_sdl_window()
{
    return ::window.get();
}

#endif

bool window_contains_point_relative( const catacurses::window &win, const point &p )
{
    const point bound = point( catacurses::getmaxx( win ), catacurses::getmaxy( win ) );
    const half_open_rectangle<point> win_bounds( point::zero, bound );
    return win_bounds.contains( p );
}
