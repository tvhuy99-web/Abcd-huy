#pragma once
#ifndef CATA_SRC_CATA_UTILITY_H
#define CATA_SRC_CATA_UTILITY_H

#include <algorithm>
#include <array>
#include <ctime>
#include <filesystem>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <string> // IWYU pragma: keep
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "enums.h"

class JsonOut;
class JsonValue;
class cata_path;
class translation;
class zzip;
class zzip_stack;




struct pair_greater_cmp_first {
    template< class T, class U >
    bool operator()( const std::pair<T, U> &a, const std::pair<T, U> &b ) const {
        return a.first > b.first;
    }

};





struct null_deleter {
    template<typename T>
    void operator()( T * ) const {}
};





enum units_type {
    VU_VEHICLE,
    VU_WIND
};

constexpr bool float_equals( double l, double r )
{
    constexpr double epsilon = std::numeric_limits<double>::epsilon() * 100;
    return l + epsilon >= r && r + epsilon >= l;
}








double round_up( double val, unsigned int dp );





template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
T divide_round_up( T num, T den )
{
    return ( num + den - 1 ) / den;
}

int divide_round_down( int a, int b );











bool isBetween( int test, int down, int up );















bool lcmatch( std::string_view str, std::string_view qry );
bool lcmatch( const translation &str, std::string_view qry );














bool match_include_exclude( std::string_view text, std::string filter );










double logarithmic( double t );













double logarithmic_range( int min, int max, int pos );


















int bound_mod_to_vals( int val, int mod, int max, int min );












const char *velocity_units( units_type vel_units );










double convert_velocity( int velocity, units_type vel_units );






double kelvin_to_fahrenheit( double kelvin );








double clamp_to_width( double value, int width, int &scale );
double clamp_to_width( double value, int width, int &scale, bool *out_truncated );






template<typename T>
constexpr T clamp( const T &val, const T &min, const T &max )
{
    return std::max( min, std::min( max, val ) );
}





template<typename T>
constexpr T lerp( const T &min, const T &max, float t )
{
    return ( 1.0f - t ) * min + t * max;
}


template<typename T>
constexpr T lerp_clamped( const T &min, const T &max, float t )
{
    return lerp( min, max, clamp( t, 0.0f, 1.0f ) );
}



template<typename T>
constexpr float inverse_lerp( const T &min, const T &max, const T &value )
{
    if( max == min ) {
        return 0.0f;
    }
    return ( value - min ) / ( max - min );
}




template<typename Tin, typename Tout>
constexpr Tout linear_remap( const Tin &i_min, const Tin &i_max,
                             const Tout &o_min, const Tout &o_max, Tin value )
{
    const float t = inverse_lerp( i_min, i_max, value );
    return lerp( o_min, o_max, t );
}







float multi_lerp( const std::vector<std::pair<float, float>> &points, float x );








template<typename T>
class list_circularizer
{
    private:
        unsigned int _index = 0;
        std::vector<T> *_list;
    public:

        explicit list_circularizer( std::vector<T> &_list ) : _list( &_list ) {
        }


        void next() {
            _index = ( _index == _list->size() - 1 ? 0 : _index + 1 );
        }


        void prev() {
            _index = ( _index == 0 ? _list->size() - 1 : _index - 1 );
        }


        T &cur() const {

            return ( *_list )[_index];
        }
};













bool write_to_file( const std::string &path, const std::function<void( std::ostream & )> &writer,
                    const char *fail_message );
void write_to_file( const std::string &path, const std::function<void( std::ostream & )> &writer );
bool write_to_file( const cata_path &path, const std::function<void( std::ostream & )> &writer,
                    const char *fail_message );
void write_to_file( const cata_path &path, const std::function<void( std::ostream & )> &writer );





















bool read_from_file( const std::string &path, const std::function<void( std::istream & )> &reader );
bool read_from_file( const std::filesystem::path &path,
                     const std::function<void( std::istream & )> &reader );
bool read_from_file( const cata_path &path, const std::function<void( std::istream & )> &reader );
bool read_from_file_json( const cata_path &path,
                          const std::function<void( const JsonValue & )> &reader );

bool read_from_file_optional( const std::string &path,
                              const std::function<void( std::istream & )> &reader );
bool read_from_file_optional( const std::filesystem::path &path,
                              const std::function<void( std::istream & )> &reader );
bool read_from_file_optional( const cata_path &path,
                              const std::function<void( std::istream & )> &reader );
bool read_from_file_optional_json( const cata_path &path,
                                   const std::function<void( const JsonValue & )> &reader );







bool read_from_zzip_optional( const zzip &z,
                              const std::filesystem::path &file,
                              const std::function<void( std::string_view )> &reader );
bool read_from_zzip_optional( const std::shared_ptr<zzip_stack> &z,
                              const std::filesystem::path &file,
                              const std::function<void( std::string_view )> &reader );













std::unique_ptr<std::istream> read_maybe_compressed_file( const std::string &path );
std::unique_ptr<std::istream> read_maybe_compressed_file( const std::filesystem::path &path );
std::unique_ptr<std::istream> read_maybe_compressed_file( const cata_path &path );











std::optional<std::string> read_whole_file( const std::string &path );
std::optional<std::string> read_whole_file( const std::filesystem::path &path );
std::optional<std::string> read_whole_file( const cata_path &path );


std::istream &safe_getline( std::istream &ins, std::string &str );
















struct do_not_replace_character {};
struct replace_character_with {
    char ch;
};
struct replace_character_randomly {};
using obscure_message_action =
    std::variant<do_not_replace_character, replace_character_with, replace_character_randomly>;

std::string obscure_message( std::string_view str,
                             const std::function<obscure_message_action()> &f );
















std::string serialize_wrapper( const std::function<void( JsonOut & )> &callback );
void deserialize_wrapper( const std::function<void( const JsonValue & )> &callback,
                          const std::string &data );

template<typename T>
inline std::string serialize( const T &obj )
{
    return serialize_wrapper( [&obj]( JsonOut & jsout ) {
        obj.serialize( jsout );
    } );
}

template<typename T>
inline void deserialize_from_string( T &obj, const std::string &data )
{
    deserialize_wrapper( [&obj]( const JsonValue & jsin ) {
        obj.deserialize( jsin );
    }, data );
}





inline bool string_starts_with( std::string_view s1, std::string_view s2 )
{
    return s1.compare( 0, s2.size(), s2 ) == 0;
}




inline bool string_ends_with( std::string_view s1, std::string_view s2 )
{
    return s1.size() >= s2.size() &&
           s1.compare( s1.size() - s2.size(), s2.size(), s2 ) == 0;
}

bool string_empty_or_whitespace( std::string_view s );



int string_view_cmp( std::string_view, std::string_view );

template<typename Integer>
Integer svto( std::string_view );

extern template int svto<int>( std::string_view );


template<typename T>
bool return_true( const T & )
{
    return true;
}

template<typename T>
bool return_false( const T & )
{
    return false;
}







template<typename Container>
std::string string_join( const Container &iterable, const std::string &joiner )
{
    std::ostringstream buffer;

    for( auto a = iterable.begin(); a != iterable.end(); ++a ) {
        if( a != iterable.begin() ) {
            buffer << joiner;
        }
        buffer << *a;
    }
    return buffer.str();
}






std::vector<std::string> string_split( std::string_view string, char delim );








template<typename... T>
std::string &str_append( std::string &root, T &&...a )
{

    static_cast<void>(
    std::array<bool, sizeof...( T )> { {
            ( root.append( std::forward<T>( a ) ), false )...
        }
    } );
    return root;
}





template<typename T0, typename... T>
std::string str_cat( T0 &&a0, T &&...a )
{
    std::string result( std::forward<T0>( a0 ) );
    return str_append( result, std::forward<T>( a )... );
}







template<typename Col, class Pred>
bool erase_if( Col &set, Pred predicate )
{
    bool ret = false;
    auto iter = set.begin();
    for( ; iter != set.end(); ) {
        if( predicate( *iter ) ) {
            iter = set.erase( iter );
            ret = true;
        } else {
            ++iter;
        }
    }
    return ret;
}










template<typename Set, typename T = std::decay_t<decltype( *std::declval<const Set &>().begin() )>>
bool equal_ignoring_elements( const Set &set, const Set &set2, const Set &ignored_elements )
{




    if( ignored_elements.empty() ) {
        return set == set2;
    }

    using Iter = typename Set::iterator;
    Iter end = ignored_elements.end();
    Iter cur = ignored_elements.begin();
    Iter prev = cur;
    cur++;


    if( !std::equal( set.begin(), set.lower_bound( *prev ),
                     set2.begin(), set2.lower_bound( *prev ) ) ) {
        return false;
    }


    while( cur != end ) {
        if( !std::equal( set.upper_bound( *prev ), set.lower_bound( *cur ),
                         set2.upper_bound( *prev ), set2.lower_bound( *cur ) ) ) {
            return false;
        }
        prev = cur;
        cur++;
    }


    return static_cast<bool>( std::equal( set.upper_bound( *prev ), set.end(),
                                          set2.upper_bound( *prev ), set2.end() ) );
}




template<typename K, typename V>
std::map<K, V> map_without_keys( const std::map<K, V> &original, const std::vector<K> &remove_keys )
{
    std::map<K, V> filtered( original );
    for( const K &key : remove_keys ) {
        filtered.erase( key );
    }
    return filtered;
}

namespace cata_detail
{
template<typename, typename = void>
struct is_unordered_map : std::false_type {};
template<typename Map>
struct is_unordered_map<Map, std::void_t<typename Map::hasher>> : std::true_type {};
}

template<typename Map, typename Set>
bool map_equal_ignoring_keys( const Map &lhs, const Map &rhs, const Set &ignore_keys )
{
    if( lhs.empty() && rhs.empty() ) {
        return true;
    }
    if( ignore_keys.empty() ) {
        return lhs == rhs;
    }

    if constexpr( cata_detail::is_unordered_map<Map>::value ) {

        std::size_t lhs_relevant = 0;
        for( const auto &kv : lhs ) {
            if( ignore_keys.count( kv.first ) ) {
                continue;
            }
            ++lhs_relevant;
            const auto it = rhs.find( kv.first );
            if( it == rhs.end() || !( it->second == kv.second ) ) {
                return false;
            }
        }
        std::size_t rhs_relevant = 0;
        for( const auto &kv : rhs ) {
            if( !ignore_keys.count( kv.first ) ) {
                ++rhs_relevant;
            }
        }
        return lhs_relevant == rhs_relevant;
    } else {

        auto li = lhs.begin();
        auto ri = rhs.begin();
        const auto le = lhs.end();
        const auto re = rhs.end();
        while( true ) {
            while( li != le && ignore_keys.count( li->first ) ) {
                ++li;
            }
            while( ri != re && ignore_keys.count( ri->first ) ) {
                ++ri;
            }
            if( li == le || ri == re ) {
                return li == le && ri == re;
            }
            if( !( li->first == ri->first ) || !( li->second == ri->second ) ) {
                return false;
            }
            ++li;
            ++ri;
        }
    }
}

int modulo( int v, int m );


template <typename T>
std::unordered_set<T> &operator<<( std::unordered_set<T> &lhv, const std::unordered_set<T> &rhv )
{
    lhv.insert( rhv.begin(), rhv.end() );
    return lhv;
}


template <typename T>
std::unordered_set<T> &operator<<( std::unordered_set<T> &lhv, std::unordered_set<T> &&rhv )
{
    for( const T &value : rhv ) {
        lhv.insert( std::move( value ) );
    }
    rhv.clear();
    return lhv;
}






holiday get_holiday_from_time( std::time_t time = 0, bool force_refresh = false );






int bucket_index_from_weight_list( const std::vector<int> &weights );





void set_title( const std::string &title );




template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
T aggregate( const std::vector<T> &values, aggregate_type agg_func )
{
    if( values.empty() ) {
        return T();
    }
    switch( agg_func ) {
        case aggregate_type::FIRST:
            return values.front();
        case aggregate_type::LAST:
            return *values.rbegin();
        case aggregate_type::MIN:
            return *std::min_element( values.begin(), values.end() );
        case aggregate_type::MAX:
            return *std::max_element( values.begin(), values.end() );
        case aggregate_type::AVERAGE:
        case aggregate_type::SUM: {
            T agg_sum = std::accumulate( values.begin(), values.end(), 0 );
            return agg_func == aggregate_type::SUM ? agg_sum : agg_sum / values.size();
        }
        default:
            return T();
    }
}


template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
explicit overloaded( Ts... ) -> overloaded<Ts...>;

std::optional<double> svtod( std::string_view token, bool debugmsg_on_fail = true );

#endif
