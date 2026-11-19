#include "TextStyleCheck.h"

#include <clang-tidy/ClangTidy.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <cstddef>
#include <string>

#include "../../src/text_style_check.h"
#include "StringLiteralIterator.h"

namespace clang
{
class LangOptions;
class TargetInfo;
}

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

TextStyleCheck::TextStyleCheck( StringRef Name, ClangTidyContext *Context )
    : ClangTidyCheck( Name, Context ),
      EscapeUnicode( Options.get( "EscapeUnicode", 0 ) != 0 ) {}

void TextStyleCheck::storeOptions( ClangTidyOptions::OptionMap &Opts )
{
    Options.store( Opts, "EscapeUnicode", EscapeUnicode );
}

void TextStyleCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        stringLiteral(
            anyOf(


                hasAncestor(
                    cxxOperatorCallExpr(
                        anyOf(
                            hasOverloadedOperatorName( "+" ),
                            hasOverloadedOperatorName( "<<" ),
                            hasOverloadedOperatorName( "+=" )
                        )
                    ).bind( "concat" )
                ),
                anything()
            ),

            unless(
                hasAncestor(
                    callExpr(
                        callee(
                            functionDecl(
                                hasAnyName( "formatted_set_simple", "ter_bind", "furn_bind" )
                            )
                        )
                    )
                )
            ),

            unless( hasAncestor( predefinedExpr() ) )
        ).bind( "str" ),
        this
    );
}

void TextStyleCheck::check( const MatchFinder::MatchResult &Result )
{
    const StringLiteral *str = Result.Nodes.getNodeAs<StringLiteral>( "str" );
    if( !str ) {
        return;
    }
    const StringLiteral &text = *str;

    const SourceManager &SrcMgr = *Result.SourceManager;
    for( size_t i = 0; i < text.getNumConcatenated(); ++i ) {
        const SourceLocation &loc = text.getStrTokenLoc( i );
        if( loc.isInvalid() ) {
            return;
        }
        if( StringRef( SrcMgr.getPresumedLoc( SrcMgr.getSpellingLoc(
                loc ) ).getFilename() ) == "<scratch space>" ) {
            return;
        }
    }


    if( ( text.isWide() || text.isUTF16() || text.isUTF32() ) || text.getCharByteWidth() != 1 ) {
        return;
    }


    bool fixit = !text.isUTF8();
    for( size_t i = 0; fixit && i < text.getNumConcatenated(); ++i ) {
        const SourceLocation &loc = text.getStrTokenLoc( i );
        if( !loc.isMacroID() && SrcMgr.getCharacterData( loc )[0] == 'R' ) {

            fixit = false;
        }
    }

    const LangOptions &LangOpts = Result.Context->getLangOpts();
    const TargetInfo &Info = Result.Context->getTargetInfo();
    const auto location = [&SrcMgr, &LangOpts, &Info]
    ( const StringLiteralIterator & it ) -> SourceLocation {
        return SrcMgr.getSpellingLoc( it.toSourceLocation( SrcMgr, LangOpts, Info ) );
    };

    const auto text_style_check_callback =
        [this, fixit, location]
        ( const text_style_fix type, const std::string & msg,
          const StringLiteralIterator &, const StringLiteralIterator &,
          const StringLiteralIterator & at,
          const StringLiteralIterator & from, const StringLiteralIterator & to,
          const std::string & fix
    ) {
        clang::DiagnosticBuilder diags = diag( location( at ), msg );
        if( fixit ) {
            switch( type ) {
                case text_style_fix::removal: {
                    const CharSourceRange range = CharSourceRange::getCharRange(
                                                      location( from ), location( to ) );
                    diags << FixItHint::CreateRemoval( range );
                }
                break;
                case text_style_fix::insertion:
                    diags << FixItHint::CreateInsertion( location( from ), fix );
                    break;
                case text_style_fix::replacement: {
                    const CharSourceRange range = CharSourceRange::getCharRange(
                                                      location( from ), location( to ) );
                    diags << FixItHint::CreateReplacement( range, fix );
                }
                break;
            }
        }
    };

    const auto beg = StringLiteralIterator::begin( text );
    const auto end = StringLiteralIterator::end( text );
    const bool in_concat_expr = Result.Nodes.getNodeAs<CXXOperatorCallExpr>( "concat" ) != nullptr;
    text_style_check<StringLiteralIterator>( beg, end,

            in_concat_expr ? fix_end_of_string_spaces::no : fix_end_of_string_spaces::yes,
            EscapeUnicode ? escape_unicode::yes : escape_unicode::no,
            text_style_check_callback );
}

}
