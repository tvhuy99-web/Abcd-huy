#include "subbodypart.h"

#include <unordered_map>
#include <vector>

#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "type_id.h"

namespace
{

generic_factory<sub_body_part_type> sub_body_part_factory( "sub body part" );


std::unordered_map<sub_bodypart_str_id, std::vector<sub_bodypart_str_id>>
        combined_similar_sub_bodyparts;

}


template<>
bool string_id<sub_body_part_type>::is_valid() const
{
    return sub_body_part_factory.is_valid( *this );
}


template<>
bool int_id<sub_body_part_type>::is_valid() const
{
    return sub_body_part_factory.is_valid( *this );
}


template<>
const sub_body_part_type &string_id<sub_body_part_type>::obj() const
{
    return sub_body_part_factory.obj( *this );
}


template<>
const sub_body_part_type &int_id<sub_body_part_type>::obj() const
{
    return sub_body_part_factory.obj( *this );
}


template<>
const sub_bodypart_str_id &int_id<sub_body_part_type>::id() const
{
    return sub_body_part_factory.convert( *this );
}


template<>
sub_bodypart_id string_id<sub_body_part_type>::id() const
{
    return sub_body_part_factory.convert( *this, sub_bodypart_id( 0 ) );
}


template<>
int_id<sub_body_part_type>::int_id( const string_id<sub_body_part_type> &id ) : _id( id.id() ) {}

void sub_body_part_type::load_bp( const JsonObject &jo, const std::string &src )
{
    sub_body_part_factory.load( jo, src );
}

void sub_body_part_type::load( const JsonObject &jo, std::string_view )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "name", name );
    mandatory( jo, was_loaded, "parent", parent );
    optional( jo, was_loaded, "secondary", secondary );
    optional( jo, was_loaded, "max_coverage", max_coverage );
    optional( jo, was_loaded, "side", part_side );
    optional( jo, was_loaded, "name_multiple", name_multiple );


    if( jo.has_string( "opposite" ) ) {
        mandatory( jo, was_loaded, "opposite", opposite );
    } else {
        opposite = id;
    }


    optional( jo, was_loaded, "locations_under", locations_under, { id } );
    optional( jo, was_loaded, "similar_bodypart", similar_bodypart );
    optional( jo, was_loaded, "unarmed_damage", unarmed_damage );
}

void sub_body_part_type::reset()
{
    sub_body_part_factory.reset();
}

void sub_body_part_type::finalize_all()
{
    sub_body_part_factory.finalize();
}

void sub_body_part_type::finalize()
{

    if( similar_bodypart.has_value() ) {

        combined_similar_sub_bodyparts[similar_bodypart.value()].emplace_back( id );

        combined_similar_sub_bodyparts[id].emplace_back( similar_bodypart.value() );
    }
}

std::vector<sub_bodypart_str_id> sub_body_part_type::get_all_combined_similar_sub_bodyparts() const
{
    const auto found = combined_similar_sub_bodyparts.find( id );

    if( found != combined_similar_sub_bodyparts.end() ) {
        return found->second;
    }

    return std::vector<sub_bodypart_str_id>();
}
