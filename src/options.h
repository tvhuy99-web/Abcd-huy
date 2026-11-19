#pragma once
#ifndef CATA_SRC_OPTIONS_H
#define CATA_SRC_OPTIONS_H

#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "translation.h"
#include "type_id.h"

class JsonArray;
class JsonObject;
class JsonOut;
class cata_path;

class options_manager
{
    public:

        using id_and_option = std::pair<std::string, translation>;
        using int_and_option = std::pair<int, translation>;
        static std::vector<id_and_option> get_lang_options();
    private:








        static void search_resource(
            std::map<std::string, std::string> &storage, std::vector<id_and_option> &option_list,
            const std::vector<std::string> &search_paths, const std::string &resource_name,
            const std::string &resource_filename );
        static void search_resource(
            std::map<std::string, cata_path> &storage, std::vector<id_and_option> &option_list,
            const std::vector<cata_path> &search_paths, const std::string &resource_name,
            const std::string &resource_filename );
        static std::vector<id_and_option> build_tilesets_list( bool only_portraits = false );
        static std::vector<id_and_option> build_soundpacks_list();
        static std::unordered_set<std::string> get_langs_with_translation_files();

        void enable_json( const std::string &var );
        void add_retry( const std::string &var, const std::string &val );

        void update_global_locale();

        std::map<std::string, std::string> post_json_verify; // NOLINT(cata-serialize)

        friend options_manager &get_options();
        options_manager();

        void addOptionToPage( const std::string &name, const std::string &page );

    public:
        enum copt_hide_t {

            COPT_NO_HIDE,

            COPT_SDL_HIDE,

            COPT_CURSES_HIDE,

            COPT_POSIX_CURSES_HIDE,

            COPT_NO_SOUND_HIDE,

            COPT_ALWAYS_HIDE
        };

        class cOpt
        {
                friend class options_manager;
            public:
                cOpt();





                bool is_hidden() const;

                std::string getName() const;
                std::string getPage() const;

                std::string getGroupName() const;

                std::string getMenuText() const;

                std::string getTooltip() const;
                std::string getType() const;

                std::string getValue( bool classis_locale = false ) const;

                std::string getValueName() const;
                std::string getDefaultText( bool bTranslated = true ) const;

                const std::string &getDefaultValue() const {
                    return sDefault;
                }

                int getItemPos( const std::string &sSearch ) const;
                std::vector<id_and_option> getItems() const;

                int getIntPos( int iSearch ) const;
                std::optional<int_and_option> findInt( int iSearch ) const;

                int getMaxLength() const;





                double getNumericMin() const {
                    return eType == CVT_FLOAT ? static_cast<double>( fMin ) :
                           eType == CVT_INT ? static_cast<double>( iMin ) : 0.0;
                }
                double getNumericMax() const {
                    return eType == CVT_FLOAT ? static_cast<double>( fMax ) :
                           eType == CVT_INT ? static_cast<double>( iMax ) : 0.0;
                }
                double getNumericStep() const {
                    return eType == CVT_FLOAT ? static_cast<double>( fStep ) :
                           eType == CVT_INT ? 1.0 : 0.0;
                }


                void setNext();

                void setPrev();

                void setValue( const std::string &sSetIn );
                void setValue( float fSetIn );
                void setValue( int iSetIn );

                template<typename T>
                T value_as( bool convert = false ) const;

                bool operator==( const cOpt &rhs ) const;
                bool operator!=( const cOpt &rhs ) const {
                    return !operator==( rhs );
                }

                static std::vector<std::string> getPrerequisiteSupportedTypes() {
                    return { "bool", "string", "string_select", "string_input" };
                }

                void setPrerequisites( const std::string &sOption, const std::vector<std::string> &sAllowedValues );
                void setPrerequisite( const std::string &sOption, const std::string &sAllowedValue = "true" ) {
                    setPrerequisites( sOption, { sAllowedValue } );
                }
                std::string getPrerequisite() const;
                bool hasPrerequisite() const;
                bool checkPrerequisite() const;

                enum COPT_VALUE_TYPE {
                    CVT_UNKNOWN = 0,
                    CVT_BOOL = 1,
                    CVT_STRING = 2,
                    CVT_FLOAT = 3,
                    CVT_INT = 4,
                    CVT_VOID = 5
                };

            private:
                std::string sName;
                std::string sPage;

                translation sMenuText;

                translation sTooltip;
                std::string sType;
                bool verbose = false;

                std::string format;

                std::string sPrerequisite;
                std::vector<std::string> sPrerequisiteAllowedValues;

                copt_hide_t hide;

                COPT_VALUE_TYPE eType;


                std::string sSet;
                std::vector<id_and_option> vItems;
                std::string sDefault;

                int iMaxLength = 0;


                bool bSet = false;
                bool bDefault = false;


                int iSet = 0;
                int iMin = 0;
                int iMax = 0;
                int iDefault = 0;
                std::vector<int_and_option> mIntValues;


                float fSet = 0.0f;
                float fMin = 0.0f;
                float fMax = 0.0f;
                float fDefault = 0.0f;
                float fStep = 0.0f;

                template<typename T>
                std::optional<T> _convert() const;
        };

        using options_container = std::unordered_map<std::string, cOpt>;

        void init();
        void add_options_general();
        void add_options_interface();
        void add_options_graphics();
        void add_options_world_default();
        void add_options_debug();
        void add_options_android();
        void load();
        bool save() const;
        std::string show( bool ingame = false, bool world_options_only = false, bool with_tabs = true );

        void add_value( const std::string &lvar, const std::string &lval,
                        const translation &lvalname );

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonArray &ja );

        std::string migrateOptionName( const std::string &name ) const;
        std::string migrateOptionValue( const std::string &name, const std::string &val ) const;


        static void update_options_cache();





        options_container get_world_defaults() const;


        options_container get_raw_options();

        void set_world_options( options_container *options );


        bool has_option( const std::string &name ) const;

        cOpt &get_option( const std::string &name );


        void add_external( const std::string &sNameIn, const std::string &sPageIn,
                           const std::string &sType );


        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const translation &sMenuTextIn, const translation &sTooltipIn,

                  const std::vector<id_and_option> &sItemsIn, std::string sDefaultIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE );


        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const translation &sMenuTextIn, const translation &sTooltipIn,
                  const std::string &sDefaultIn, int iMaxLengthIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE );


        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const translation &sMenuTextIn, const translation &sTooltipIn,
                  bool bDefaultIn, copt_hide_t opt_hide = COPT_NO_HIDE );


        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const translation &sMenuTextIn, const translation &sTooltipIn,
                  int iMinIn, int iMaxIn, int iDefaultIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE,
                  const std::string &format = "%i" );


        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const translation &sMenuTextIn, const translation &sTooltipIn,
                  const std::vector<int_and_option> &mIntValuesIn,
                  int iInitialIn, int iDefaultIn, copt_hide_t opt_hide = COPT_NO_HIDE,
                  bool verbose = false );


        void add( const std::string &sNameIn, const std::string &sPageIn,
                  const translation &sMenuTextIn, const translation &sTooltipIn,
                  float fMinIn, float fMaxIn,
                  float fDefaultIn, float fStepIn,
                  copt_hide_t opt_hide = COPT_NO_HIDE,
                  const std::string &format = "%.2f" );

    private:
        options_container options;
        std::optional<options_container *> world_options; // NOLINT(cata-serialize)


        class Group
        {
            public:

                std::string id_;

                translation name_;

                translation tooltip_;

                Group() = default;
                Group( const std::string &id, const translation &name, const translation &tooltip )
                    : id_( id ), name_( name ), tooltip_( tooltip ) { }
        };


        enum class ItemType {
            BlankLine,
            GroupHeader,
            Option,
        };


        class PageItem
        {
            public:
                ItemType type;
                std::string data;

                std::string group;

                PageItem() : type( ItemType::BlankLine ) { }
                PageItem( ItemType type, const std::string &data, const std::string &group )
                    : type( type ), data( data ), group( group ) { }

                std::string fmt_tooltip( const std::string &group_id, const options_container &cont ) const;
        };









        class Page
        {
            public:

                std::string id_;

                translation name_;

                std::vector<PageItem> items_;

                void removeRepeatedEmptyLines();

                Page( const std::string &id, const translation &name ) : id_( id ), name_( name ) { }
        };

        std::vector<Page> pages_; // NOLINT(cata-serialize)
        std::string adding_to_group_; // NOLINT(cata-serialize)
        std::vector<Group> groups_; // NOLINT(cata-serialize)












        void add_option_group( const std::string &page_id, const Group &group,
                               const std::function<void( const std::string & )> &entries );


        void add_empty_line( const std::string &sPageIn );


        Page &find_page( const std::string &id );


        const Group &find_group( const std::string &id ) const;
};

struct option_slider {
    private:
        struct option_slider_level {
            private:
                struct opt_slider_option {
                    std::string _opt;
                    std::string _type;
                    std::string _val;
                    opt_slider_option( const std::string &opt, const std::string &type, const std::string &val ) :
                        _opt( opt ), _type( type ), _val( val ) {}
                };
                translation _name;
                translation _desc;
                int _level = 0;
                std::vector<opt_slider_option> _opts;

            public:
                option_slider_level() = default;
                option_slider_level( const translation &name, const translation &desc, int level ) :
                    _name( name ), _desc( desc ), _level( level ) {}

                const translation &name() const {
                    return _name;
                }

                const translation &desc() const {
                    return _desc;
                }

                int level() const {
                    return _level;
                }

                void add( const std::string &opt, const std::string &type, const std::string &val ) {
                    _opts.emplace_back( opt, type, val );
                }

                bool remove( const std::string &opt );
                void apply_opts( options_manager::options_container &OPTIONS ) const;
                void deserialize( const JsonObject &jo );
        };

        translation _name;
        std::vector<option_slider_level> _levels;
        std::string _context;
        int _default_level = 0;

    public:
        option_slider_id id;
        bool was_loaded = false;

        static void load_option_sliders( const JsonObject &jo, const std::string &src );
        static void reset();
        static void finalize_all();
        static void check_consistency();
        void load( const JsonObject &jo, std::string_view src );
        void finalize();
        void check() const;
        static const std::vector<option_slider> &get_all();

        void reorder_opts() {
            std::sort( _levels.begin(), _levels.end(),
            []( const option_slider_level & a, const option_slider_level & b ) {
                return a.level() < b.level();
            } );
        }

        const translation &name() const {
            return _name;
        }

        const std::string &context() const {
            return _context;
        }

        int default_level() const {
            return _default_level;
        }

        int count() const {
            return _levels.size();
        }

        void apply_opts( int level, options_manager::options_container &OPTIONS ) const {
            if( level >= 0 && level < static_cast<int>( _levels.size() ) ) {
                _levels[level].apply_opts( OPTIONS );
            }
        }

        const translation &level_name( int level ) const {
            static const translation none;
            if( level >= 0 && level < static_cast<int>( _levels.size() ) ) {
                return _levels[level].name();
            }
            return none;
        }

        const translation &level_desc( int level ) const {
            static const translation none;
            if( level >= 0 && level < static_cast<int>( _levels.size() ) ) {
                return _levels[level].desc();
            }
            return none;
        }

        int random_level() const;
};





extern std::map<std::string, cata_path> TILESETS;




extern std::map<std::string, cata_path> SOUNDPACKS;

options_manager &get_options();

inline bool has_option( const std::string &name )
{
    return get_options().has_option( name );
}

template<typename T>
inline T get_option( const std::string &name, bool convert = false )
{
    return get_options().get_option( name ).value_as<T>( convert );
}

#endif
