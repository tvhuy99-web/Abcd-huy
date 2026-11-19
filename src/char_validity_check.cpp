#include "char_validity_check.h"

#include <cctype>








bool is_char_allowed( int ch )
{
#if defined(_MSC_VER)












    if( ch == 9 ) {
        return false;
    }
#endif





    if( ch < 0 || ch >= 128 ) {
        return true;
    }
    if( !std::isprint( ch ) ) {
        return false;
    }
    if( ch == '\\' || ch == '/' || ch == ':' || ch == '*' || ch == '?' || ch == '"' || ch == '<' ||
        ch == '>' || ch == '|' ) {

        return false;
    }
    return true;
}
