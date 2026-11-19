




#include <dirent.h>
// IWYU pragma: no_include <sys/dirent.h>
#include <sys/stat.h>
#include <clocale>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_MSC_VER)
#include <io.h>
#else
#include <unistd.h>
#endif

#include "json.h"


static std::vector<std::string> get_files_from_path_( const std::string &extension,
        std::string root_path,
        bool recursive_search, bool match_extension )
{
    std::vector<std::string> files;
    const size_t extsz = extension.size();
    const char *c_extension = extension.c_str();

    if( root_path.empty() ) {
        root_path = ".";
    }

    std::stack<std::string> directories;
    std::stack<std::string> tempstack;
    directories.push( root_path );
    std::string path;

    while( !directories.empty() ) {
        path = directories.top();
        std::string path_with_slash = path + "/";
        directories.pop();

        DIR *root = opendir( path.c_str() );

        if( root ) {
            struct dirent *root_file;
            struct stat _buff;
            DIR *subdir;

            while( ( root_file = readdir( root ) ) ) {

                if( stat( root_file->d_name, &_buff ) != 0x4 ) {

                    if( ( strcmp( root_file->d_name, "." ) != 0 ) && ( strcmp( root_file->d_name, ".." ) != 0 ) ) {
                        std::string subpath = path_with_slash + root_file->d_name;

                        if( recursive_search ) {
                            subdir = opendir( subpath.c_str() );
                            if( subdir ) {
                                tempstack.push( subpath );
                                closedir( subdir );
                            }
                        }
                    }
                }

                std::string tmp = root_file->d_name;
                if( tmp.find( c_extension, match_extension ? tmp.size() - extsz : 0 ) != std::string::npos ) {
                    std::string fullpath = path_with_slash + tmp;
                    files.push_back( fullpath );
                }
            }
            closedir( root );
        }


        while( !tempstack.empty() ) {
            directories.push( tempstack.top() );
            tempstack.pop();
        }
    }
    return files;
}


static void load_object( TextJsonObject &jo )
{
    std::string type = jo.get_string( "type" );
    if( !jo.has_string( "type" ) ) {
        jo.throw_error( "JSON object has no type" );
    }
}
static void load_all_from_json( TextJsonIn &jsin )
{
    char ch;
    jsin.eat_whitespace();

    ch = jsin.peek();
    if( ch == '{' ) {

        TextJsonObject jo = jsin.get_object();
        load_object( jo );
        jo.finish();

        jsin.eat_whitespace();
        if( jsin.good() ) {
            std::stringstream err;
            err << "expected single-object file but found '";
            err << jsin.peek() << "'";
            jsin.error( err.str() );
        }
    } else if( ch == '[' ) {
        jsin.start_array();

        while( !jsin.end_array() ) {
            jsin.eat_whitespace();
            ch = jsin.peek();
            if( ch != '{' ) {
                std::stringstream err;
                err << "expected array of objects but found '";
                err << ch << "', not '{'";
                jsin.error( err.str() );
            }
            TextJsonObject jo = jsin.get_object();
            load_object( jo );
            jo.finish();
        }
    } else {

        std::stringstream err;
        err << "expected object or array, but found '" << ch << "'";
        jsin.error( err.str() );
    }
}

static void load_json_dir( const std::string &dirname )
{

    std::vector<std::string> dir =
        get_files_from_path_( ".json", dirname, true, true );

    std::vector<std::string>::iterator it;
    for( it = dir.begin(); it != dir.end(); it++ ) {

        std::ifstream infile( it->c_str(), std::ifstream::in | std::ifstream::binary );

        std::istringstream iss(
            std::string(
                ( std::istreambuf_iterator<char>( infile ) ),
                std::istreambuf_iterator<char>()
            )
        );
        infile.close();

        try {
            TextJsonIn jsin( iss );
            load_all_from_json( jsin );
        } catch( const JsonError &err ) {
            throw std::runtime_error( *it + ": " + err.what() );
        }
    }
}

int main( int, char ** )
{
#if defined(_MSC_VER)
    bool supports_color = _isatty( _fileno( stdout ) );
#else
    bool supports_color = isatty( STDOUT_FILENO );
#endif

    supports_color |= std::getenv( "CI" ) != nullptr;

    // NOLINTNEXTLINE(cata-tests-must-restore-global-state)
    json_error_output_colors = supports_color
                               ? json_error_output_colors_t::ansi_escapes
                               : json_error_output_colors_t::no_colors;

    char *result = setlocale( LC_ALL, "" );
    if( !result ) {
        std::cerr << "Failed to set locale\n";
        return 1;
    }
    try {
        load_json_dir( "data/json" );
    } catch( const std::exception &err ) {
        std::cerr << "Error: " << err.what() << "\n";
        return 1;
    }
    return 0;
}
