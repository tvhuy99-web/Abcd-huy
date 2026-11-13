#include "input_popup.h"
#include "a11y_semantics.h"
#include "a11y_runtime.h"
#include "a11y_android.h"
#include "a11y_quantity_selection.h"
#include "a11y_text.h"

#include <cstddef>
#include <cmath>
#include <locale>
#if defined(__ANDROID__)
#include <limits>
#endif
#include <optional>
#include <sstream>
#include <type_traits>

#include "coordinates.h"
#if defined(__ANDROID__)
#include "catacharset.h"
#endif
#include "output.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "input_enums.h"
#include "ret_val.h"
#include "text.h"
#include "translations.h"
#include "try_parse_integer.h"
#include "uilist.h"
#include "ui_manager.h"
#include "uistate.h"

#if defined(__ANDROID__)
#include <jni.h>
#include "sdltiles.h"

// Reuse the standard popup pipeline for every ordinary text/number field.
// The native Android EditText is accessible to TalkBack, while the original
// ImGui editor remains in place for non-reader mode and custom callbacks.
// The optional is empty on Cancel; false means JNI is unavailable and the
// existing game editor should handle the request instead.
static bool android_accessible_input( const std::string &title, const std::string &initial,
                                      int kind, int limit, double minimum, double maximum,
                                      std::optional<std::string> &result )
{
    if( !a11y::enabled() ) {
        return false;
    }
    JNIEnv *env = static_cast<JNIEnv *>( GetAndroidJNIEnv() );
    jobject activity = static_cast<jobject>( GetAndroidActivity() );
    if( env == nullptr || activity == nullptr ) {
        return false;
    }
    jclass activity_class = env->GetObjectClass( activity );
    jmethodID get_native_ui = env->GetMethodID( activity_class, "getNativeUI",
                              "()Lcom/cleverraven/cataclysmdda/NativeUI;" );
    if( get_native_ui == nullptr ) {
        env->ExceptionClear();
        env->DeleteLocalRef( activity_class );
        env->DeleteLocalRef( activity );
        return false;
    }
    jobject native_ui = env->CallObjectMethod( activity, get_native_ui );
    if( native_ui == nullptr || env->ExceptionCheck() ) {
        env->ExceptionClear();
        env->DeleteLocalRef( activity_class );
        env->DeleteLocalRef( activity );
        return false;
    }
    jclass native_class = env->GetObjectClass( native_ui );
    jmethodID input_text = env->GetMethodID( native_class, "inputText",
                           "(Ljava/lang/String;Ljava/lang/String;IIDD)Ljava/lang/String;" );
    if( input_text == nullptr ) {
        env->ExceptionClear();
        env->DeleteLocalRef( native_class );
        env->DeleteLocalRef( native_ui );
        env->DeleteLocalRef( activity_class );
        env->DeleteLocalRef( activity );
        return false;
    }
    jstring jtitle = env->NewStringUTF( title.c_str() );
    jstring jinitial = env->NewStringUTF( initial.c_str() );
    jstring submitted = static_cast<jstring>( env->CallObjectMethod( native_ui, input_text,
                        jtitle, jinitial, kind, limit, minimum, maximum ) );
    const bool success = !env->ExceptionCheck();
    if( !success ) {
        env->ExceptionClear();
    } else if( submitted != nullptr ) {
        const char *utf8 = env->GetStringUTFChars( submitted, nullptr );
        if( utf8 != nullptr ) {
            result = std::string( utf8 );
            env->ReleaseStringUTFChars( submitted, utf8 );
        }
    }
    if( submitted != nullptr ) {
        env->DeleteLocalRef( submitted );
    }
    env->DeleteLocalRef( jinitial );
    env->DeleteLocalRef( jtitle );
    env->DeleteLocalRef( native_class );
    env->DeleteLocalRef( native_ui );
    env->DeleteLocalRef( activity_class );
    env->DeleteLocalRef( activity );
    return success;
}
#endif

input_popup::input_popup( int width, const std::string &title, const point &pos,
                          ImGuiWindowFlags flags ) :
    cataimgui::window( title, ImGuiWindowFlags_NoNavInputs | flags | ( title.empty() ?
                       ImGuiWindowFlags_NoTitleBar : 0 ) | ImGuiWindowFlags_AlwaysAutoResize ),
    reader_title( title ),
    pos( pos ),
    width( width )
{
    ctxt = input_context( "STRING_INPUT", keyboard_mode::keychar );
    ctxt.register_action( "TEXT.CONFIRM" );
    ctxt.register_action( "TEXT.QUIT" );
    ctxt.register_action( "TEXT.CLEAR" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.set_timeout( 10 );
}

void input_popup::set_description( const std::string &desc, const nc_color &default_color,
                                   bool monofont )
{
    description = desc;
    description_default_color = default_color;
    description_monofont = monofont;
}

void input_popup::set_label( const std::string &label, const nc_color &default_color )
{
    this->label = label;
    label_default_color = default_color;
}

void input_popup::set_max_input_length( int length )
{
    max_input_length = length;
}

int input_popup::get_max_input_length() const
{
    return max_input_length;
}

static void register_input( input_context &ctxt, const callback_input &input )
{
    if( input.description ) {
        ctxt.register_action( input.action, *input.description );
    } else {
        ctxt.register_action( input.action );
    }
}

void input_popup::add_callback( const callback_input &input,
                                const std::function<bool()> &callback_func )
{
    if( !input.action.empty() ) {
        register_input( ctxt, input );
    }
    callbacks.emplace_back( input, callback_func );
}

bool input_popup::cancelled() const
{
    return is_cancelled;
}

cataimgui::bounds input_popup::get_bounds()
{
    return {
        pos.x < 0 ? -1.f : str_width_to_pixels( pos.x ),
        pos.y < 0 ? -1.f : str_height_to_pixels( pos.y ),
        width <= 0 ? -1.f : str_width_to_pixels( width ),
        -1.f
    };
}

void input_popup::draw_controls()
{
    if( !description.empty() ) {
        if( description_monofont ) {
            cataimgui::PushMonoFont();
        }
        // todo: you should not have to manually split text to print it as paragraphs
        uint64_t offset = 0;
        uint64_t pos = 0;
        while( pos != std::string::npos ) {
            pos = description.find( '\n', offset );
            std::string str = description.substr( offset, pos - offset );
            cataimgui::TextColoredParagraph( description_default_color, str );
            ImGui::NewLine();
            offset = pos + 1;
        }
        if( description_monofont ) {
            ImGui::PopFont();
        }
    }

    if( !label.empty() ) {
        ImGui::AlignTextToFramePadding();
        cataimgui::draw_colored_text( label, c_light_red );
        ImGui::SameLine();
    }

    // make sure the cursor is in the input field when we regain focus
    if( !ImGui::IsWindowFocused() ) {
        set_focus = true;
    }

    if( set_focus && ImGui::IsWindowFocused() ) {
        ImGui::SetKeyboardFocusHere();
        set_focus = false;
    }
    draw_input_control();
}

bool input_popup::handle_custom_callbacks( const std::string &action )
{

    input_event ev = ctxt.get_raw_input();
    int key = ev.get_first_input();

    bool next_loop = false;
    for( const auto &cb : callbacks ) {
        if( cb.first == callback_input( action, key ) && cb.second() ) {
            next_loop = true;
            break;
        }
    }

    return next_loop;
}

string_input_popup_imgui::string_input_popup_imgui( int width, const std::string &old_input,
        const std::string &title, const point &pos, ImGuiWindowFlags flags ) :
    input_popup( width, title, pos, flags ),
    text( old_input ),
    old_input( old_input )
{
    // for opening uilist history
    // non-uilist history is handled by imgui itself and not customizable (yet?)
    ctxt.register_action( "HISTORY_UP" );
}

static void set_text( ImGuiInputTextCallbackData *data, const std::string &text )
{
    data->DeleteChars( 0, data->BufTextLen );
    data->InsertChars( 0, text.c_str() );
}

static int input_callback( ImGuiInputTextCallbackData *data )
{
    string_input_popup_imgui *popup = static_cast<string_input_popup_imgui *>( data->UserData );

    if( data->EventFlag == ImGuiInputTextFlags_CallbackHistory ) {
        popup->update_input_history( data );
    }

    if( data->EventFlag == ImGuiInputTextFlags_CallbackEdit ) {
        int max = popup->get_max_input_length();
        if( max > 0 && data->BufTextLen > max ) {
            data->BufTextLen = max;
            data->SelectionStart = data->BufTextLen;
            data->SelectionEnd = data->BufTextLen;
            data->CursorPos = data->BufTextLen;
            data->BufDirty = true;
        }
    }

    if( data->EventFlag == ImGuiInputTextFlags_CallbackAlways && popup->want_clear_text() ) {
        // Only called when popup->text is empty
        data->BufTextLen = 0;
        data->SelectionStart = 0;
        data->SelectionEnd = 0;
        data->CursorPos = 0;
        data->Buf[0] = '\000';
        data->BufDirty = true;
    }

    return 0;
}

bool string_input_popup_imgui::want_clear_text()
{
    if( do_clear_text ) {
        do_clear_text = false;
        return true;
    }
    return false;
}

void string_input_popup_imgui::draw_input_control()
{
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackEdit;

    if( !is_uilist_history ) {
        flags |= ImGuiInputTextFlags_CallbackHistory;
    }

    if( do_clear_text ) {
        flags |= ImGuiInputTextFlags_CallbackAlways;
    }

    // shrink width of input field if we only allow short inputs
    // todo: use full available width for unrestricted inputs in wider windows
    float input_width = str_width_to_pixels( max_input_length + 1 );
    if( max_input_length > 0 && input_width < ImGui::CalcItemWidth() ) {
        ImGui::SetNextItemWidth( input_width );
    }

    std::string input_label = "##string_input_" + label;
    ImGui::InputText( input_label.c_str(), &text, flags, input_callback, this );
}

void string_input_popup_imgui::set_identifier( const std::string &ident )
{
    identifier = ident;
}

void string_input_popup_imgui::set_text( const std::string &txt )
{
    text = txt;
}

void string_input_popup_imgui::show_history()
{
    if( identifier.empty() ) {
        return;
    }
    std::vector<std::string> &hist = uistate.gethistory( identifier );
    uilist hmenu;
    hmenu.title = _( "d: delete history" );
    hmenu.allow_anykey = true;
    for( size_t h = 0; h < hist.size(); h++ ) {
        hmenu.addentry( h, true, -2, hist[h] );
    }
    if( !text.empty() && ( hmenu.entries.empty() ||
                           hmenu.entries[hist.size() - 1].txt != text ) ) {
        hmenu.addentry( hist.size(), true, -2, text );
    }

    if( !hmenu.entries.empty() ) {
        hmenu.selected = hmenu.entries.size() - 1;

        bool finished = false;
        do {
            hmenu.query();
            if( hmenu.ret >= 0 && hmenu.entries[hmenu.ret].txt != text ) {
                set_text( hmenu.entries[hmenu.ret].txt );
                if( static_cast<size_t>( hmenu.ret ) < hist.size() ) {
                    hist.erase( hist.begin() + hmenu.ret );
                    add_to_history( text );
                }
                finished = true;
            } else if( hmenu.ret == UILIST_UNBOUND && hmenu.ret_evt.get_first_input() == 'd' ) {
                hist.clear();
                finished = true;
            } else if( hmenu.ret != UILIST_UNBOUND ) {
                finished = true;
            }
        } while( !finished );
    }
}

void string_input_popup_imgui::update_input_history( ImGuiInputTextCallbackData *data )
{
    if( data->EventKey != ImGuiKey_UpArrow && data->EventKey != ImGuiKey_DownArrow ) {
        return;
    }

    bool up = data->EventKey == ImGuiKey_UpArrow;

    if( identifier.empty() ) {
        return;
    }

    std::vector<std::string> &hist = uistate.gethistory( identifier );

    if( hist.empty() ) {
        return;
    }

    if( hist.size() >= max_history_size ) {
        hist.erase( hist.begin(), hist.begin() + ( hist.size() - max_history_size ) );
    }

    if( up ) {
        if( history_index >= static_cast<int>( hist.size() ) ) {
            return;
        } else if( history_index == 0 ) {
            session_input.assign( data->Buf, data->BufTextLen );

            //avoid showing the same result twice (after reopen filter window without reset)
            if( hist.size() > 1 && session_input == hist.back() ) {
                history_index += 1;
            }
        }
    } else {
        if( history_index == 1 ) {
            ::set_text( data, session_input );
            //show initial string entered and 'return'
            history_index = 0;
            return;
        } else if( history_index == 0 ) {
            return;
        }
    }

    history_index += up ? 1 : -1;
    const std::string &new_text = hist[hist.size() - history_index];
    ::set_text( data, new_text );
}

void string_input_popup_imgui::add_to_history( const std::string &value ) const
{
    if( !identifier.empty() && !value.empty() ) {
        std::vector<std::string> &hist = uistate.gethistory( identifier );
        if( hist.empty() || hist[hist.size() - 1] != value ) {
            hist.push_back( value );
        }
    }
}

std::string string_input_popup_imgui::query()
{
    is_cancelled = false;
#if defined(__ANDROID__)
    if( callbacks.empty() ) {
        std::optional<std::string> submitted;
        const std::string title = reader_title.empty() ?
                                  ( reader_description().empty() ? _( "Enter text" ) : remove_color_tags( reader_description() ) ) :
                                  remove_color_tags( reader_title );
        if( android_accessible_input( title, text, 0, max_input_length,
                                      std::numeric_limits<double>::quiet_NaN(),
                                      std::numeric_limits<double>::quiet_NaN(), submitted ) ) {
            is_cancelled = !submitted.has_value();
            if( !submitted ) {
                return old_input;
            }
            text = *submitted;
            add_to_history( text );
            return text;
        }
    }
#endif
    a11y::screen_scope reader( &ctxt, "STRING_INPUT",
    [&]( const a11y::semantic_intent & intent ) -> std::optional<std::string> {
        if( intent.action == "cancel" || ( intent.node == "cancel" && intent.action == "activate" ) ) {
            return "TEXT.QUIT";
        }
        if( intent.node == "confirm" && intent.action == "activate" ) { return "TEXT.CONFIRM"; }
        if( intent.node == "text" && intent.action == "set_text" ) {
            if( max_input_length > 0 && intent.value.size() > static_cast<std::size_t>( max_input_length ) ) {
                a11y::android::status( "invalid_value", &intent );
                return std::nullopt;
            }
            set_text( intent.value );
            a11y::android::status( "text_applied", &intent );
            return "TIMEOUT";
        }
        return std::nullopt;
    } );

    while( true ) {
        ui_manager::redraw_invalidated();
        reader.update( a11y::text_snapshot( remove_color_tags( reader_title ),
                       remove_color_tags( reader_description() ), text, false ) );

        std::string action = ctxt.handle_input();
        if( handle_custom_callbacks( action ) ) {
            continue;
        }

        if( action == "TEXT.CONFIRM" ) {
            add_to_history( text );
            return text;
        } else if( action == "TEXT.QUIT" ) {
            break;
        } else if( action == "HISTORY_UP" && is_uilist_history ) {
            // non-uilist history is handled inside input callback
            show_history();
        } else if( action == "TEXT.CLEAR" ) {
            do_clear_text = true;
        }

        // mouse click on x to close leads here
        if( !get_is_open() ) {
            break;
        }
    }

    is_cancelled = true;
    return old_input;
}

std::optional<tripoint_abs_omt> string_input_popup_imgui::query_coordinate( bool loop )
{
    do {
        const std::string &queried_string = query();
        ret_val<tripoint_abs_omt> result = try_parse_coordinate_abs( queried_string );
        if( cancelled() || queried_string.empty() ) {
            return std::nullopt;
        }
        if( result.success() ) {
            return result.value();
        }
    } while( loop );

    return tripoint_abs_omt::zero;
}

void string_input_popup_imgui::use_uilist_history( bool use_uilist )
{
    is_uilist_history = use_uilist;
}

template<typename T>
number_input_popup<T>::number_input_popup( int width, T old_value, const std::string &title,
        const point &pos, ImGuiWindowFlags flags ) :
    input_popup( width, title, pos, flags ),
    value( old_value ),
    old_value( old_value )
{
    // potentially register context keys
}

template<>
void number_input_popup<int>::draw_input_control()
{
    // todo: maybe set width of input field, default is fairly long
    // step size default values are imgui defaults
    ImGui::InputInt( "##number_input", &value, step_size.value_or( 1 ),
                     fast_step_size.value_or( 100 ) );
}

template<>
void number_input_popup<float>::draw_input_control()
{
    // todo: maybe set width of input field, default is fairly long
    // step size default values are imgui defaults
    cataimgui::InputFloat( "##number_input", &value, step_size.value_or( 0.f ),
                           fast_step_size.value_or( 0.f ) );
}

template<typename T>
T number_input_popup<T>::query()
{
    last_quantity_cancelled = false;
#if defined(__ANDROID__)
    if( callbacks.empty() && !accessible_quantity_max ) {
        std::ostringstream old_text;
        old_text.imbue( std::locale::classic() );
        old_text << value;
        std::optional<std::string> submitted;
        if( android_accessible_input( reader_title.empty() ? _( "Enter number" ) :
                                      remove_color_tags( reader_title ), old_text.str(),
                                      std::is_integral<T>::value ? 1 : 2, 0,
                                      value_min ? static_cast<double>( *value_min ) :
                                      std::numeric_limits<double>::quiet_NaN(),
                                      value_max ? static_cast<double>( *value_max ) :
                                      std::numeric_limits<double>::quiet_NaN(), submitted ) ) {
            last_quantity_cancelled = !submitted.has_value();
            if( !submitted ) {
                return old_value;
            }
            std::istringstream parsed( *submitted );
            parsed.imbue( std::locale::classic() );
            T next{};
            parsed >> next;
            if( !parsed.fail() ) {
                parsed >> std::ws;
                if( parsed.eof() && std::isfinite( static_cast<double>( next ) ) ) {
                    value = next;
                    return value;
                }
            }
            // The Java dialog already validates numeric syntax and bounds
            // (int / finite float), so this path is defensive only.
            return old_value;
        }
    }
#endif
    a11y::screen_scope reader( &ctxt, "NUMBER_INPUT",
    [&]( const a11y::semantic_intent & intent ) -> std::optional<std::string> {
        if( intent.action == "cancel" || ( intent.node == "cancel" && intent.action == "activate" ) ) {
            return "TEXT.QUIT";
        }
        if( intent.node == "confirm" && intent.action == "activate" ) { return "TEXT.CONFIRM"; }
        if( accessible_quantity_max && std::is_integral<T>::value &&
            intent.node == "quantity" && intent.action == "set_value" ) {
            const auto count = a11y::selected_quantity( intent.value,
                               static_cast<std::size_t>( *accessible_quantity_max ) );
            if( !count ) {
                a11y::android::status( "invalid_value", &intent );
                return std::nullopt;
            }
            value = static_cast<T>( *count );
            // Selecting one completed number closes the popup.
            return "TEXT.CONFIRM";
        }
        if( intent.node == "text" && intent.action == "set_text" ) {
            std::istringstream parser( intent.value );
            parser.imbue( std::locale::classic() );
            T parsed{};
            parser >> parsed;
            const bool parsed_number = !parser.fail();
            parser >> std::ws;
            if( !parsed_number || !parser.eof() || !std::isfinite( static_cast<double>( parsed ) ) ) {
                a11y::android::status( "invalid_value", &intent );
                return std::nullopt;
            }
            value = parsed;
            a11y::android::status( "text_applied", &intent );
            return "TIMEOUT";
        }
        return std::nullopt;
    } );

    while( true ) {
        ui_manager::redraw_invalidated();
        std::ostringstream reader_value;
        reader_value.imbue( std::locale::classic() );
        reader_value << value;
        if( accessible_quantity_max && std::is_integral<T>::value ) {
            a11y::semantic_snapshot snapshot;
            snapshot.title = remove_color_tags( reader_title );
            snapshot.summary = remove_color_tags( reader_description() );
            snapshot.focus = "quantity";
            a11y::semantic_node field;
            field.id = "quantity";
            field.role = "checkbox";
            field.label = _( "Quantity" );
            field.value = reader_value.str();
            field.field_type = "int";
            field.applied_value = reader_value.str();
            field.has_field_range = true;
            field.field_min = 0;
            field.field_max = static_cast<double>( *accessible_quantity_max );
            field.field_step = 1;
            field.actions = { "focus", "activate", "set_value" };
            snapshot.nodes.push_back( std::move( field ) );
            a11y::semantic_node cancel;
            cancel.id = "cancel";
            cancel.label = _( "Cancel" );
            cancel.actions = { "focus", "activate" };
            snapshot.nodes.push_back( std::move( cancel ) );
            reader.update( std::move( snapshot ) );
        } else {
            reader.update( a11y::text_snapshot( remove_color_tags( reader_title ),
                           remove_color_tags( reader_description() ), reader_value.str(), true,
                           step_size.has_value() ? static_cast<double>( *step_size ) : 0.0,
                           std::is_integral<T>::value ) );
        }
        std::string action = ctxt.handle_input();

        if( handle_custom_callbacks( action ) ) {
            continue;
        }

        if( action == "TEXT.CONFIRM" ) {
            return value;
        } else if( action == "TEXT.QUIT" ) {
            last_quantity_cancelled = true;
            break;
        }

        // mouse click on x to close leads here
        if( !get_is_open() ) {
            last_quantity_cancelled = true;
            break;
        }
    }

    return old_value;
}

template<typename T>
void number_input_popup<T>::set_accessible_quantity_max( const T available )
{
    if( std::is_integral<T>::value && available > 0 ) {
        accessible_quantity_max = available;
    }
}

template<typename T>
bool number_input_popup<T>::quantity_cancelled() const
{
    return last_quantity_cancelled;
}

template<typename T>
void number_input_popup<T>::set_value_range( T minimum, T maximum )
{
    value_min = minimum;
    value_max = maximum;
}

template<typename T>
void number_input_popup<T>::set_step_size( std::optional<T> step, std::optional<T> fast_step )
{
    step_size = step;
    fast_step_size = fast_step;
}

template class number_input_popup<int>;
template class number_input_popup<float>;
