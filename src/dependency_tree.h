#pragma once
#ifndef CATA_SRC_DEPENDENCY_TREE_H
#define CATA_SRC_DEPENDENCY_TREE_H

#include <map>
#include <stack>
#include <string>
#include <vector>

#include "type_id.h"

enum NODE_ERROR_TYPE {
    DEPENDENCY,
    CYCLIC,
    OTHER
};

class dependency_node
{
    public:
        std::vector<dependency_node *> parents, children;
        std::map<NODE_ERROR_TYPE, std::vector<std::string> > all_errors;
        mod_id key;
        bool availability;


        int index;
        int lowlink;
        bool on_stack;

        dependency_node();
        explicit dependency_node( const mod_id &key );

        void add_parent( dependency_node *parent );
        void add_child( dependency_node *child );
        bool is_available() const;
        bool has_errors();
        std::map<NODE_ERROR_TYPE, std::vector<std::string > > errors() const;
        std::string s_errors();



        std::vector<mod_id> get_dependencies_as_strings();
        std::vector<dependency_node * > get_dependencies_as_nodes();

        std::vector<mod_id> get_dependents_as_strings();
        std::vector< dependency_node * > get_dependents_as_nodes();

        void inherit_errors();
        void check_cyclicity();
};

class dependency_tree
{
    public:
        dependency_tree();

        void init( const std::map<mod_id, std::vector<mod_id> > &key_dependency_map );

        void clear();



        std::vector<mod_id > get_dependencies_of_X_as_strings( const mod_id &key );
        std::vector<dependency_node * > get_dependencies_of_X_as_nodes( const mod_id &key );

        std::vector< mod_id > get_dependents_of_X_as_strings( const mod_id &key );
        std::vector< dependency_node * > get_dependents_of_X_as_nodes( const mod_id &key );

        bool is_available( const mod_id &key );
        dependency_node *get_node( const mod_id &key );

        std::map<mod_id, dependency_node> master_node_map;
    private:

        void build_node_map( const std::map<mod_id, std::vector<mod_id > > &key_dependency_map );
        void build_connections( const std::map<mod_id, std::vector<mod_id > > &key_dependency_map );








        void check_for_strongly_connected_components();
        void strong_connect( dependency_node *dnode );

        std::vector<std::vector<dependency_node * > > strongly_connected_components;
        std::stack<dependency_node *> connection_stack;
        std::vector<dependency_node *> nodes_on_stack;
        int open_index;

};

#endif
