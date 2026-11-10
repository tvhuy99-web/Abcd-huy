#include "a11y_android.h"
#include "a11y_world_input_gate.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <limits>
#include <mutex>
#include <utility>

#if defined(__ANDROID__)
#include <jni.h>
#include "catacharset.h"
#include "options.h"
#include "sdltiles.h"
#include "sdl_wrappers.h"
#endif

namespace a11y::android
{
namespace
{
std::atomic<bool> reader_enabled{ false };
std::atomic<bool> reader_suspended{ true };
// Game-thread check is one atomic read; JNI only changes the selected modes.
std::atomic<bool> no_world_graphics{ false };
std::atomic<bool> performance_profile_active{ false };
std::atomic<unsigned int> profile_configuration_epoch{ 0 };
std::atomic<unsigned int> profile_busy_epoch{ 0 };
// Only accessed by the native game thread, never by Java callbacks.
unsigned int last_profile_configuration_epoch = 0;
unsigned int last_profile_busy_epoch = 0;
long long last_profile_draw_ms = -1;
long long last_profile_sim_ms = -1;
long long last_profile_vision_ms = -1;
long long last_profile_reader_ms = -1;
bool shifted_since_last_step = false;
bool pending_walking_step = false;
std::chrono::steady_clock::time_point pending_step_completed_at{};
std::atomic<std::uint64_t> epoch{ 1 };
std::mutex inbox_mutex;
std::deque<semantic_intent> inbox;
// Protected by inbox_mutex, never read from Java without holding it.
context_token last_world_token;
bool has_world_token = false;
bool world_step_ready = false;
std::atomic<int> active_travel_state{ 0 };
std::atomic<int> pending_travel_control{ 0 };
std::atomic<std::uint64_t> travel_map_return_deadline_ms{ 0 };

bool matches_last_world( const context_token &token )
{
    return has_world_token && token.session == last_world_token.session &&
           token.frame == last_world_token.frame &&
           token.generation == last_world_token.generation;
}

std::atomic<int> debug_prompt_response{ 0 };
#if defined(__ANDROID__)
// UI thread only queues updates. C++ options and audio are modified on the
// native game thread, never from a Java callback.
struct pending_sound_settings {
    int music;
    int effects;
    bool enabled;
};
std::mutex sound_settings_mutex;
std::optional<pending_sound_settings> pending_sound;
#endif
} // namespace

bool enabled() { return reader_enabled.load( std::memory_order_acquire ); }
bool suspended() { return reader_suspended.load( std::memory_order_acquire ); }
std::uint64_t lifecycle_epoch() { return epoch.load( std::memory_order_acquire ); }
bool suppress_world_visuals()
{
    return enabled() && !suspended() && no_world_graphics.load( std::memory_order_relaxed );
}
bool performance_profiling_enabled()
{
    return enabled() && !suspended() &&
           performance_profile_active.load( std::memory_order_relaxed );
}
void configure_performance( const bool skip_visuals, const bool profile )
{
    no_world_graphics.store( skip_visuals, std::memory_order_release );
    const bool previous = performance_profile_active.exchange( profile, std::memory_order_acq_rel );
    if( profile != previous ) {
        profile_configuration_epoch.fetch_add( 1, std::memory_order_release );
    }
}

// This is diagnostic-only state. It is touched exclusively by the native
// game thread. The Java switch only changes the atomic profile flag/epoch.
static void reset_profile_if_reconfigured()
{
    const unsigned int config_epoch = profile_configuration_epoch.load( std::memory_order_acquire );
    if( config_epoch == last_profile_configuration_epoch ) {
        return;
    }
    last_profile_configuration_epoch = config_epoch;
    last_profile_draw_ms = -1;
    last_profile_sim_ms = -1;
    last_profile_vision_ms = -1;
    last_profile_reader_ms = -1;
    shifted_since_last_step = false;
    pending_walking_step = false;
    last_profile_busy_epoch = profile_busy_epoch.load( std::memory_order_acquire );
}

void performance_draw_sample( const long long ms )
{
    if( performance_profiling_enabled() ) {
        reset_profile_if_reconfigured();
        last_profile_draw_ms = ms;
    }
}
void performance_sim_sample( const long long ms )
{
    if( performance_profiling_enabled() ) {
        reset_profile_if_reconfigured();
        // Some moves consume several simulated turns. Record the slowest
        // one until the result is published to the reader.
        last_profile_sim_ms = std::max( last_profile_sim_ms, ms );
    }
}
void performance_vision_sample( const long long ms )
{
    if( performance_profiling_enabled() ) {
        reset_profile_if_reconfigured();
        last_profile_vision_ms = ms;
    }
}
void performance_reader_sample( const long long ms )
{
    if( !performance_profiling_enabled() ) {
        return;
    }
    reset_profile_if_reconfigured();
    last_profile_reader_ms = ms;
    if( !pending_walking_step ) {
        // In logging mode every event is recorded, but a non-walking turn
        // must not reuse timing samples for the next walking action.
        last_profile_draw_ms = -1;
        last_profile_sim_ms = -1;
        last_profile_vision_ms = -1;
        last_profile_reader_ms = -1;
        shifted_since_last_step = false;
        last_profile_busy_epoch = profile_busy_epoch.load( std::memory_order_acquire );
        return;
    }

    const auto update_after_action_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                            std::chrono::steady_clock::now() -
                                            pending_step_completed_at ).count();
    const unsigned int busy_epoch = profile_busy_epoch.load( std::memory_order_acquire );
#if defined(__ANDROID__)
    // ON is a complete diagnostic capture. Do not sample, skip fast
    // footsteps or stop after any number of stable turns.
    diagnostic_event( "WORLD_STEP_PIPELINE",
                      "after_action_to_reader_ms=" + std::to_string( update_after_action_ms ) +
                      " draw_ms=" + std::to_string( last_profile_draw_ms ) +
                      " sim_max_ms=" + std::to_string( last_profile_sim_ms ) +
                      " vision_ms=" + std::to_string( last_profile_vision_ms ) +
                      " reader_ms=" + std::to_string( last_profile_reader_ms ) +
                      " shifted=" + ( shifted_since_last_step ? "true" : "false" ) +
                      " busy=" + ( busy_epoch != last_profile_busy_epoch ? "true" : "false" ) );
#endif

    pending_walking_step = false;
    shifted_since_last_step = false;
    last_profile_draw_ms = -1;
    last_profile_sim_ms = -1;
    last_profile_vision_ms = -1;
    last_profile_reader_ms = -1;
    last_profile_busy_epoch = busy_epoch;
}
void performance_world_shift()
{
    if( performance_profiling_enabled() ) {
        reset_profile_if_reconfigured();
        shifted_since_last_step = true;
    }
}
void performance_step_sample( const bool moved )
{
    if( !performance_profiling_enabled() ) {
        return;
    }
    reset_profile_if_reconfigured();
    pending_walking_step = moved;
    if( moved ) {
        pending_step_completed_at = std::chrono::steady_clock::now();
    }
}

void configure( const bool enable, const bool suspend )
{
    const std::lock_guard<std::mutex> lock( inbox_mutex );
    reader_enabled.store( enable, std::memory_order_release );
    reader_suspended.store( suspend, std::memory_order_release );
    inbox.clear();
    world_step_ready = false;
    has_world_token = false;
    // An intent from the former Android Activity must never control a
    // new owner. The route state itself survives a lifecycle rebind.
    pending_travel_control.store( 0, std::memory_order_release );
    travel_map_return_deadline_ms.store( 0, std::memory_order_release );
    epoch.fetch_add( 1, std::memory_order_release );
#if defined(__ANDROID__)
    PushWakeEvent();
#endif
}

bool enqueue( semantic_intent intent )
{
    const std::lock_guard<std::mutex> lock( inbox_mutex );
    if( !enabled() || suspended() || inbox.size() >= 32 || intent.request == 0 ||
        intent.node.size() > 512 || intent.action.size() > 80 || intent.value.size() > 16384 ) {
        return false;
    }
    // Only the world walking channel is single-flight. Menu navigation
    // retains the existing mailbox because it uses a different token.
    if( intent.action == "activate" && is_world_step_node( intent.node ) &&
        matches_last_world( intent.token ) ) {
        if( !world_step_ready ||
            std::any_of( inbox.begin(), inbox.end(), []( const semantic_intent &pending ) {
            return pending.action == "activate" && is_world_step_node( pending.node );
        } ) ) {
            if( performance_profile_active.load( std::memory_order_relaxed ) ) {
                profile_busy_epoch.fetch_add( 1, std::memory_order_relaxed );
            }
            return false;
        }
        // Reserve the one movement input immediately. Subsequent swipes
        // during a long game turn are refused now, never executed later.
        world_step_ready = false;
    }
    intent.received_at_ms = static_cast<std::uint64_t>(
                                std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::steady_clock::now().time_since_epoch() ).count() );
    inbox.push_back( std::move( intent ) );
#if defined(__ANDROID__)
    PushWakeEvent();
#endif
    return true;
}

std::optional<semantic_intent> take()
{
    const std::lock_guard<std::mutex> lock( inbox_mutex );
    if( inbox.empty() ) {
        return std::nullopt;
    }
    semantic_intent result = std::move( inbox.front() );
    inbox.pop_front();
    return result;
}

void clear()
{
    const std::lock_guard<std::mutex> lock( inbox_mutex );
    inbox.clear();
    world_step_ready = false;
}

void set_world_step_ready( const std::optional<context_token> token, const bool ready )
{
    const std::lock_guard<std::mutex> lock( inbox_mutex );
    if( token ) {
        last_world_token = *token;
        has_world_token = true;
    }
    // A repeated game-thread poll must not reopen the gate while an
    // accepted step is still waiting in the native mailbox.
    const bool pending_step = std::any_of( inbox.begin(), inbox.end(),
    []( const semantic_intent &pending ) {
        return pending.action == "activate" && is_world_step_node( pending.node );
    } );
    world_step_ready = ready && token.has_value() && enabled() && !suspended() &&
                       !pending_step;
}

bool world_step_can_accept()
{
    // JNI read-only readiness probe. The game thread is the only writer.
    // This is not permission to bypass gameplay: enqueue() repeats the
    // exact same one-step gate under this mutex.
    const std::lock_guard<std::mutex> lock( inbox_mutex );
    return world_step_ready && has_world_token && enabled() && !suspended() &&
           std::none_of( inbox.begin(), inbox.end(), []( const semantic_intent &pending ) {
        return pending.action == "activate" && is_world_step_node( pending.node );
    } );
}

int travel_state()
{
    return active_travel_state.load( std::memory_order_acquire );
}

void set_travel_state( const int state )
{
    active_travel_state.store( state >= 0 && state <= 3 ? state : 0,
                               std::memory_order_release );
}

bool request_travel_control( const int control )
{
    const int current = travel_state();
    if( !enabled() || suspended() ||
        ( control == 1 && current != 1 ) ||
        ( control == 2 && current != 2 && current != 3 ) ||
        ( control == 3 && current == 0 ) ||
        ( control == 4 && current != 3 ) ||
        control < 1 || control > 4 ) {
        return false;
    }
    // Only the latest explicit control is retained. The game thread checks
    // the actual route before applying it; no async operation touches game.
    pending_travel_control.store( control, std::memory_order_release );
#if defined(__ANDROID__)
    PushWakeEvent();
#endif
    return true;
}

int take_travel_control()
{
    return pending_travel_control.exchange( 0, std::memory_order_acq_rel );
}

void request_travel_map_return()
{
    const auto now = std::chrono::steady_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch() ).count();
    travel_map_return_deadline_ms.store( static_cast<std::uint64_t>( ms + 2000 ),
                                         std::memory_order_release );
}

bool take_travel_map_return()
{
    const std::uint64_t deadline = travel_map_return_deadline_ms.exchange(
                                       0, std::memory_order_acq_rel );
    const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::steady_clock::now().time_since_epoch() ).count();
    return deadline != 0 && static_cast<std::uint64_t>( now ) <= deadline;
}

void reset_travel_control()
{
    pending_travel_control.store( 0, std::memory_order_release );
    active_travel_state.store( 0, std::memory_order_release );
    travel_map_return_deadline_ms.store( 0, std::memory_order_release );
}

std::size_t cancel_pending_world_steps()
{
    // The Android-only command hub is not a native modal. Explicitly
    // discard walking inputs already waiting in the existing mailbox
    // when the player opens that hub, without touching other actions.
    const std::lock_guard<std::mutex> lock( inbox_mutex );
    const std::size_t before = inbox.size();
    inbox.erase( std::remove_if( inbox.begin(), inbox.end(),
    []( const semantic_intent & intent ) {
        return intent.action == "activate" && is_world_step_node( intent.node ) &&
               matches_last_world( intent.token );
    } ), inbox.end() );
    return before - inbox.size();
}

void publish( const std::string &json )
{
#if defined(__ANDROID__)
    JNIEnv *env = static_cast<JNIEnv *>( GetAndroidJNIEnv() );
    if( env == nullptr || env->PushLocalFrame( 8 ) < 0 ) {
        return;
    }
    jobject activity = static_cast<jobject>( GetAndroidActivity() );
    if( activity != nullptr ) {
        jclass cls = env->GetObjectClass( activity );
        jmethodID method = env->GetMethodID( cls, "onAccessibilitySnapshot", "(Ljava/lang/String;)V" );
        if( method != nullptr ) {
            // JNI NewStringUTF uses modified UTF-8. Use UTF-16 explicitly for
            // Vietnamese combining marks and characters outside the BMP.
            std::u16string utf16;
            for( const char32_t cp : utf8_to_utf32( json ) ) {
                if( cp > 0x10ffff || ( cp >= 0xd800 && cp <= 0xdfff ) ) {
                    utf16.push_back( 0xfffd );
                } else if( cp <= 0xffff ) {
                    utf16.push_back( static_cast<char16_t>( cp ) );
                } else {
                    utf16.push_back( static_cast<char16_t>( 0xd800 + ( ( cp - 0x10000 ) >> 10 ) ) );
                    utf16.push_back( static_cast<char16_t>( 0xdc00 + ( ( cp - 0x10000 ) & 0x3ff ) ) );
                }
            }
            jstring text = env->NewString( reinterpret_cast<const jchar *>( utf16.data() ),
                                          static_cast<jsize>( utf16.size() ) );
            if( text != nullptr ) {
                env->CallVoidMethod( activity, method, text );
            }
        }
    }
    if( env->ExceptionCheck() ) {
        // The Java recovery panel remains available. Never unwind a pending
        // Java exception through C++ gameplay or retry the selected action.
        env->ExceptionClear();
    }
    env->PopLocalFrame( nullptr );
#else
    ( void )json;
#endif
}

void announce( const std::string &text, const int priority )
{
#if defined(__ANDROID__)
    if( text.empty() || text.size() > 16384 || ( priority != 1 && priority != 3 ) ) {
        return;
    }
    JNIEnv *env = static_cast<JNIEnv *>( GetAndroidJNIEnv() );
    if( env == nullptr ) {
        return;
    }
    if( env->PushLocalFrame( 8 ) < 0 ) {
        if( env->ExceptionCheck() ) {
            env->ExceptionClear();
        }
        return;
    }
    jobject activity = static_cast<jobject>( GetAndroidActivity() );
    if( activity != nullptr ) {
        jclass cls = env->GetObjectClass( activity );
        if( cls != nullptr ) {
            jmethodID method = env->GetMethodID( cls, "onAccessibilityAnnouncement",
                                                "(Ljava/lang/String;I)V" );
            if( method != nullptr ) {
                // Use UTF-16 instead of JNI modified UTF-8 for Vietnamese and
                // codepoints outside the Basic Multilingual Plane.
                std::u16string utf16;
                for( const char32_t cp : utf8_to_utf32( text ) ) {
                    if( cp > 0x10ffff || ( cp >= 0xd800 && cp <= 0xdfff ) ) {
                        utf16.push_back( 0xfffd );
                    } else if( cp <= 0xffff ) {
                        utf16.push_back( static_cast<char16_t>( cp ) );
                    } else {
                        utf16.push_back( static_cast<char16_t>( 0xd800 + ( ( cp - 0x10000 ) >> 10 ) ) );
                        utf16.push_back( static_cast<char16_t>( 0xdc00 + ( ( cp - 0x10000 ) & 0x3ff ) ) );
                    }
                }
                jstring value = env->NewString( reinterpret_cast<const jchar *>( utf16.data() ),
                                               static_cast<jsize>( utf16.size() ) );
                if( value != nullptr ) {
                    env->CallVoidMethod( activity, method, value, static_cast<jint>( priority ) );
                }
            }
        }
    }
    if( env->ExceptionCheck() ) {
        // Speech delivery must never unwind through the game loop.
        env->ExceptionClear();
    }
    env->PopLocalFrame( nullptr );
#else
    ( void )text;
    ( void )priority;
#endif
}

void present_debug_prompt( const std::string &report )
{
#if defined(__ANDROID__)
    debug_prompt_response.store( 0, std::memory_order_release );
    if( report.empty() ) {
        return;
    }
    JNIEnv *env = static_cast<JNIEnv *>( GetAndroidJNIEnv() );
    if( env == nullptr ) {
        return;
    }
    if( env->PushLocalFrame( 8 ) < 0 ) {
        if( env->ExceptionCheck() ) {
            env->ExceptionClear();
        }
        return;
    }
    jobject activity = static_cast<jobject>( GetAndroidActivity() );
    if( activity != nullptr ) {
        jclass cls = env->GetObjectClass( activity );
        jmethodID method = cls == nullptr ? nullptr :
                           env->GetMethodID( cls, "onAccessibilityDebugPrompt",
                                             "(Ljava/lang/String;)V" );
        if( method != nullptr ) {
            std::u16string utf16;
            for( const char32_t cp : utf8_to_utf32( report ) ) {
                if( cp > 0x10ffff || ( cp >= 0xd800 && cp <= 0xdfff ) ) {
                    utf16.push_back( 0xfffd );
                } else if( cp <= 0xffff ) {
                    utf16.push_back( static_cast<char16_t>( cp ) );
                } else {
                    utf16.push_back( static_cast<char16_t>( 0xd800 + ( ( cp - 0x10000 ) >> 10 ) ) );
                    utf16.push_back( static_cast<char16_t>( 0xdc00 + ( ( cp - 0x10000 ) & 0x3ff ) ) );
                }
            }
            jstring text = env->NewString( reinterpret_cast<const jchar *>( utf16.data() ),
                                          static_cast<jsize>( utf16.size() ) );
            if( text != nullptr ) {
                env->CallVoidMethod( activity, method, text );
            }
        }
    }
    if( env->ExceptionCheck() ) {
        env->ExceptionClear();
    }
    env->PopLocalFrame( nullptr );
#else
    ( void )report;
#endif
}

int take_debug_prompt_response()
{
    return debug_prompt_response.exchange( 0, std::memory_order_acq_rel );
}

void dismiss_debug_prompt()
{
#if defined(__ANDROID__)
    JNIEnv *env = static_cast<JNIEnv *>( GetAndroidJNIEnv() );
    if( env == nullptr ) {
        return;
    }
    if( env->PushLocalFrame( 4 ) < 0 ) {
        if( env->ExceptionCheck() ) {
            env->ExceptionClear();
        }
        return;
    }
    jobject activity = static_cast<jobject>( GetAndroidActivity() );
    if( activity != nullptr ) {
        jclass cls = env->GetObjectClass( activity );
        jmethodID method = cls == nullptr ? nullptr :
                           env->GetMethodID( cls, "onAccessibilityDebugPromptDismissed", "()V" );
        if( method != nullptr ) {
            env->CallVoidMethod( activity, method );
        }
    }
    if( env->ExceptionCheck() ) {
        env->ExceptionClear();
    }
    env->PopLocalFrame( nullptr );
#endif
}

#if defined(__ANDROID__)
void queue_sound_settings( const int music, const int effects, const bool enabled )
{
    if( music < 0 || music > 128 || effects < 0 || effects > 128 ) {
        return;
    }
    {
        const std::lock_guard<std::mutex> lock( sound_settings_mutex );
        pending_sound = pending_sound_settings{ music, effects, enabled };
    }
    PushWakeEvent();
}

void apply_pending_sound_settings()
{
    std::optional<pending_sound_settings> update;
    {
        const std::lock_guard<std::mutex> lock( sound_settings_mutex );
        update = pending_sound;
        pending_sound.reset();
    }
    if( !update ) {
        return;
    }
    get_options().get_option( "SOUND_ENABLED" ).setValue(
        std::string( update->enabled ? "true" : "false" ) );
    get_options().get_option( "MUSIC_VOLUME" ).setValue( update->music );
    get_options().get_option( "SOUND_EFFECT_VOLUME" ).setValue( update->effects );
    // This also updates SDL music output and the global sound-enabled flag.
    get_options().save();
}

void diagnostic_event( const std::string &category, const std::string &detail )
{
    // Manual OFF disables the entire diagnostic bridge, including failures.
    // Java checks the same manual switch under its file lock before writing.
    if( !performance_profile_active.load( std::memory_order_acquire ) ) {
        return;
    }
    // Called on the native game thread; only ASCII internal action IDs are
    // sent here. User-entered text never crosses this diagnostic bridge.
    JNIEnv *env = static_cast<JNIEnv *>( GetAndroidJNIEnv() );
    if( env == nullptr || env->PushLocalFrame( 8 ) < 0 ) {
        if( env != nullptr && env->ExceptionCheck() ) { env->ExceptionClear(); }
        return;
    }
    jobject activity = static_cast<jobject>( GetAndroidActivity() );
    if( activity != nullptr ) {
        jclass cls = env->GetObjectClass( activity );
        jmethodID method = cls == nullptr ? nullptr :
                           env->GetMethodID( cls, "onAccessibilityNativeDiagnostic",
                                             "(Ljava/lang/String;Ljava/lang/String;)V" );
        if( method != nullptr ) {
            jstring kind = env->NewStringUTF( category.c_str() );
            jstring info = env->NewStringUTF( detail.c_str() );
            if( kind != nullptr && info != nullptr ) {
                env->CallVoidMethod( activity, method, kind, info );
            }
        }
    }
    if( env->ExceptionCheck() ) { env->ExceptionClear(); }
    env->PopLocalFrame( nullptr );
}

void open_debug_log()
{
    JNIEnv *env = static_cast<JNIEnv *>( GetAndroidJNIEnv() );
    if( env == nullptr || env->PushLocalFrame( 4 ) < 0 ) {
        if( env != nullptr && env->ExceptionCheck() ) { env->ExceptionClear(); }
        return;
    }
    jobject activity = static_cast<jobject>( GetAndroidActivity() );
    if( activity != nullptr ) {
        jclass cls = env->GetObjectClass( activity );
        jmethodID method = cls == nullptr ? nullptr :
                           env->GetMethodID( cls, "onAccessibilityOpenDebugLog", "()V" );
        if( method != nullptr ) { env->CallVoidMethod( activity, method ); }
    }
    if( env->ExceptionCheck() ) { env->ExceptionClear(); }
    env->PopLocalFrame( nullptr );
}

void normal_exit()
{
    JNIEnv *env = static_cast<JNIEnv *>( GetAndroidJNIEnv() );
    if( env == nullptr || env->PushLocalFrame( 4 ) < 0 ) {
        if( env != nullptr && env->ExceptionCheck() ) { env->ExceptionClear(); }
        return;
    }
    jobject activity = static_cast<jobject>( GetAndroidActivity() );
    if( activity != nullptr ) {
        jclass cls = env->GetObjectClass( activity );
        jmethodID method = cls == nullptr ? nullptr :
                           env->GetMethodID( cls, "onAccessibilityNormalExit", "()V" );
        if( method != nullptr ) { env->CallVoidMethod( activity, method ); }
    }
    if( env->ExceptionCheck() ) { env->ExceptionClear(); }
    env->PopLocalFrame( nullptr );
}

void open_sound_settings( const int music, const int effects, const bool enabled )
{
    JNIEnv *env = static_cast<JNIEnv *>( GetAndroidJNIEnv() );
    if( env == nullptr || env->PushLocalFrame( 4 ) < 0 ) {
        if( env != nullptr && env->ExceptionCheck() ) {
            env->ExceptionClear();
        }
        return;
    }
    jobject activity = static_cast<jobject>( GetAndroidActivity() );
    if( activity != nullptr ) {
        jclass cls = env->GetObjectClass( activity );
        jmethodID method = cls == nullptr ? nullptr :
                           env->GetMethodID( cls, "onAccessibilityOpenSoundSettings", "(IIZ)V" );
        if( method != nullptr ) {
            env->CallVoidMethod( activity, method, music, effects,
                                 enabled ? JNI_TRUE : JNI_FALSE );
        }
    }
    if( env->ExceptionCheck() ) {
        env->ExceptionClear();
    }
    env->PopLocalFrame( nullptr );
}
#endif

void open_tts_settings()
{
#if defined(__ANDROID__)
    JNIEnv *env = static_cast<JNIEnv *>( GetAndroidJNIEnv() );
    if( env == nullptr || env->PushLocalFrame( 4 ) < 0 ) {
        if( env != nullptr && env->ExceptionCheck() ) {
            env->ExceptionClear();
        }
        return;
    }
    jobject activity = static_cast<jobject>( GetAndroidActivity() );
    if( activity != nullptr ) {
        jclass cls = env->GetObjectClass( activity );
        if( cls != nullptr ) {
            jmethodID method = env->GetMethodID( cls, "onAccessibilityOpenTtsSettings", "()V" );
            if( method != nullptr ) {
                env->CallVoidMethod( activity, method );
            }
        }
    }
    if( env->ExceptionCheck() ) {
        env->ExceptionClear();
    }
    env->PopLocalFrame( nullptr );
#endif
}

void cue( const std::string &name )
{
    // Only established accessibility alerts may reach the Android cue mapping.
    if( name != "blocked" && name != "danger" && name != "pickup" &&
        name != "activity_done" ) {
        return;
    }
#if defined(__ANDROID__)
    JNIEnv *env = static_cast<JNIEnv *>( GetAndroidJNIEnv() );
    if( env == nullptr ) {
        return;
    }
    if( env->PushLocalFrame( 8 ) < 0 ) {
        if( env->ExceptionCheck() ) {
            env->ExceptionClear();
        }
        return;
    }
    jobject activity = static_cast<jobject>( GetAndroidActivity() );
    if( activity != nullptr ) {
        jclass cls = env->GetObjectClass( activity );
        if( cls != nullptr ) {
            jmethodID method = env->GetMethodID( cls, "onAccessibilityCue",
                                                "(Ljava/lang/String;)V" );
            if( method != nullptr ) {
                // The whitelist above is pure ASCII, so plain
                // NewStringUTF is exact here.
                jstring value = env->NewStringUTF( name.c_str() );
                if( value != nullptr ) {
                    env->CallVoidMethod( activity, method, value );
                }
            }
        }
    }
    if( env->ExceptionCheck() ) {
        // A cue failure must never unwind through the game loop.
        env->ExceptionClear();
    }
    env->PopLocalFrame( nullptr );
#endif
}

void alignment_distance( const int distance )
{
#if defined(__ANDROID__)
    if( !enabled() || suspended() ) {
        return;
    }
    JNIEnv *env = static_cast<JNIEnv *>( GetAndroidJNIEnv() );
    if( env == nullptr || env->PushLocalFrame( 4 ) < 0 ) {
        return;
    }
    jobject activity = static_cast<jobject>( GetAndroidActivity() );
    if( activity != nullptr ) {
        jclass cls = env->GetObjectClass( activity );
        if( cls != nullptr ) {
            jmethodID method = env->GetMethodID( cls, "onAccessibilityAlignment", "(I)V" );
            if( method != nullptr ) {
                env->CallVoidMethod( activity, method, static_cast<jint>( distance ) );
            }
        }
    }
    if( env->ExceptionCheck() ) {
        env->ExceptionClear();
    }
    env->PopLocalFrame( nullptr );
#else
    ( void )distance;
#endif
}

void status( const std::string &code, const semantic_intent *intent )
{
    // Codes are internal literals; translated user-facing text belongs to Java.
    if( code == "stale" || code == "disabled" || code == "unsupported" ||
        code == "invalid_value" || code == "text_applied" ) {
        std::string json = "{\"status\":\"" + code + "\"";
        if( intent != nullptr ) {
            json += ",\"request\":\"" + std::to_string( intent->request ) +
                    "\",\"session\":\"" + std::to_string( intent->token.session ) +
                    "\",\"frame\":\"" + std::to_string( intent->token.frame ) +
                    "\",\"generation\":\"" + std::to_string( intent->token.generation ) + "\"";
        }
        publish( json + '}' );
    }
}
} // namespace a11y::android

#if defined(__ANDROID__)
namespace
{
std::string from_java( JNIEnv *env, jstring value )
{
    if( value == nullptr ) {
        return {};
    }
    const jsize size = env->GetStringLength( value );
    if( size > 16384 ) {
        return {};
    }
    const jchar *chars = env->GetStringChars( value, nullptr );
    if( chars == nullptr ) {
        return {};
    }
    std::u32string codepoints;
    for( jsize i = 0; i < size; ++i ) {
        char32_t cp = chars[i];
        if( cp >= 0xd800 && cp <= 0xdbff && i + 1 < size &&
            chars[i + 1] >= 0xdc00 && chars[i + 1] <= 0xdfff ) {
            cp = 0x10000 + ( ( cp - 0xd800 ) << 10 ) + ( chars[++i] - 0xdc00 );
        } else if( cp >= 0xd800 && cp <= 0xdfff ) {
            cp = 0xfffd;
        }
        codepoints.push_back( cp );
    }
    env->ReleaseStringChars( value, chars );
    return utf32_to_utf8( codepoints );
}
} // namespace

extern "C" JNIEXPORT void JNICALL
Java_com_cleverraven_cataclysmdda_CataclysmDDA_nativeQueueSoundSettings(
    JNIEnv *, jclass, jint music, jint effects, jboolean enabled )
{
    a11y::android::queue_sound_settings( music, effects, enabled == JNI_TRUE );
}

extern "C" JNIEXPORT void JNICALL
Java_com_cleverraven_cataclysmdda_CataclysmDDA_nativeConfigureReader(
    JNIEnv *, jclass, jboolean enable, jboolean suspend )
{
    a11y::android::configure( enable == JNI_TRUE, suspend == JNI_TRUE );
}

extern "C" JNIEXPORT void JNICALL
Java_com_cleverraven_cataclysmdda_CataclysmDDA_nativeConfigureReaderPerformance(
    JNIEnv *, jclass, jboolean no_world_visuals, jboolean profile )
{
    a11y::android::configure_performance( no_world_visuals == JNI_TRUE, profile == JNI_TRUE );
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_cleverraven_cataclysmdda_CataclysmDDA_nativeWorldStepReady( JNIEnv *, jclass )
{
    return a11y::android::world_step_can_accept() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_cleverraven_cataclysmdda_CataclysmDDA_nativeCancelPendingWorldSteps(
    JNIEnv *, jclass )
{
    return static_cast<jint>( a11y::android::cancel_pending_world_steps() );
}

extern "C" JNIEXPORT jint JNICALL
Java_com_cleverraven_cataclysmdda_CataclysmDDA_nativeTravelState( JNIEnv *, jclass )
{
    return static_cast<jint>( a11y::android::travel_state() );
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_cleverraven_cataclysmdda_CataclysmDDA_nativeTravelControl(
    JNIEnv *, jclass, jint control )
{
    return a11y::android::request_travel_control( control ) ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_cleverraven_cataclysmdda_CataclysmDDA_nativeDebugPromptResponse(
    JNIEnv *, jclass, jint response )
{
    if( response >= 1 && response <= 3 ) {
        a11y::android::debug_prompt_response.store( response, std::memory_order_release );
        PushWakeEvent();
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_cleverraven_cataclysmdda_CataclysmDDA_nativeReaderIntent(
    JNIEnv *env, jclass, jlong session, jlong frame, jlong generation,
    jlong revision, jlong request, jstring node, jstring action, jstring value )
{
    if( session <= 0 || frame <= 0 || generation <= 0 || revision <= 0 || request <= 0 ) {
        return JNI_FALSE;
    }
    a11y::semantic_intent intent;
    intent.token = { static_cast<std::uint64_t>( session ), static_cast<std::uint64_t>( frame ),
                     static_cast<std::uint64_t>( generation ) };
    intent.revision = static_cast<std::uint64_t>( revision );
    intent.request = static_cast<std::uint64_t>( request );
    intent.node = from_java( env, node );
    intent.action = from_java( env, action );
    intent.value = from_java( env, value );
    return a11y::android::enqueue( std::move( intent ) ) ? JNI_TRUE : JNI_FALSE;
}
#endif
