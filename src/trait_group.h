#pragma once
#ifndef CATA_SRC_TRAIT_GROUP_H
#define CATA_SRC_TRAIT_GROUP_H

#include <memory>
#include <string>
#include <vector>

#include "type_id.h"

class JsonObject;
class JsonValue;
class Trait_group;
struct trait_and_var;

namespace trait_group
{

using Trait_group_tag = string_id<Trait_group>;
using Trait_list = std::vector<trait_and_var>;




Trait_list traits_from( const Trait_group_tag &gid );




bool group_contains_trait( const Trait_group_tag &gid, const trait_id &tid );




void load_trait_group( const JsonObject &jsobj, const Trait_group_tag &gid,
                       const std::string &subtype );



















Trait_group_tag load_trait_group( const JsonValue &value, const std::string &default_subtype );




void debug_spawn();

}





class Trait_creation_data
{
    public:
        using RecursionList = std::vector<trait_group::Trait_group_tag>;

        explicit Trait_creation_data( int _probability ) : probability( _probability ) {}
        virtual ~Trait_creation_data() = default;
        Trait_creation_data( const Trait_creation_data & ) = delete;
        Trait_creation_data &operator=( const Trait_creation_data & ) = delete;





        virtual trait_group::Trait_list create( RecursionList &rec ) const = 0;
        trait_group::Trait_list create() const;




        virtual void check_consistency() const = 0;





        virtual bool remove_trait( const trait_id &tid ) = 0;




        virtual bool has_trait( const trait_id &tid ) const = 0;


        int probability;
};





class Single_trait_creator : public Trait_creation_data
{
    public:
        Single_trait_creator( const trait_id &id, const std::string &var, int probability );
        ~Single_trait_creator() override = default;

        trait_id id;
        std::string variant;

        trait_group::Trait_list create( RecursionList &rec ) const override;
        void check_consistency() const override;
        bool remove_trait( const trait_id &tid ) override;
        bool has_trait( const trait_id &tid ) const override;
};





class Trait_group_creator : public Trait_creation_data
{
    public:
        Trait_group_creator( const trait_group::Trait_group_tag &id, int probability );
        ~Trait_group_creator() override = default;

        trait_group::Trait_group_tag id;

        trait_group::Trait_list create( RecursionList &rec ) const override;
        void check_consistency() const override;
        bool remove_trait( const trait_id &tid ) override;
        bool has_trait( const trait_id &tid ) const override;
};




class Trait_group : public Trait_creation_data
{
    public:
        using CreatorList = std::vector<std::unique_ptr<Trait_creation_data> >;

        explicit Trait_group( int probability );
        ~Trait_group() override = default;

        void add_trait_entry( const trait_id &tid, const std::string &var, int probability );
        void add_group_entry( const trait_group::Trait_group_tag &gid, int probability );






        virtual void add_entry( std::unique_ptr<Trait_creation_data> ptr ) = 0;

        void check_consistency() const override;
        bool remove_trait( const trait_id &tid ) override;
        bool has_trait( const trait_id &tid ) const override;

    protected:

        CreatorList creators;

        int sum_prob;
};





class Trait_group_collection : public Trait_group
{
    public:
        explicit Trait_group_collection( int probability );
        ~Trait_group_collection() override = default;

        trait_group::Trait_list create( RecursionList &rec ) const override;
        void add_entry( std::unique_ptr<Trait_creation_data> ptr ) override;
};






class Trait_group_distribution : public Trait_group
{
    public:
        explicit Trait_group_distribution( int probability ) :
            Trait_group( probability ) {}
        ~Trait_group_distribution() override = default;

        trait_group::Trait_list create( RecursionList &rec ) const override;
        void add_entry( std::unique_ptr<Trait_creation_data> ptr ) override;
};

#endif
