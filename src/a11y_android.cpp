#include "a11y_android.h"

#include <atomic>
#include <deque>
#include <limits>
#include <mutex>
#include <utility>

#if defined(__ANDROID__)
#include <jni.h>
#include "catacharset.h"
#include "sdltiles.h"
#include "sdl_wrappers.h"
#endif

namespace a11y::android
{
namespace
{
std::atomic<bool> reader_enabled{ false };
std::atomic<bool> reader_suspended{ true };
std::atomic<std::uint64_t> epoch{ 1 };
std::mutex inbox_mutex;
std::deque<semantic_intent> inbox;
} // namespace

bool enabled() { return reader_enabled.load( std::memory_order_acquire ); }
bool suspended() { return reader_suspended.load( std::memory_order_acquire ); }
std::uint64_t lifecycle_epoch() { return epoch.load( std::memory_order_acquire ); }

void configure( const bool enable, const bool suspend )
{
    const std::lock_guard<std::mutex> lock( inbox_mutex );
    reader_enabled.store( enable, std::memory_order_release );
    reader_suspended.store( suspend, std::memory_order_release );
    inbox.clear();
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
Java_com_cleverraven_cataclysmdda_CataclysmDDA_nativeConfigureReader(
    JNIEnv *, jclass, jboolean enable, jboolean suspend )
{
    a11y::android::configure( enable == JNI_TRUE, suspend == JNI_TRUE );
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
