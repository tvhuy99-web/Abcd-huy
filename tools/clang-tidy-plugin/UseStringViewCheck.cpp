#include "UseStringViewCheck.h"

#include <algorithm>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclBase.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/OperatorKinds.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <climits>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/Casting.h>
#include <string>

#include "Utils.h"

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

static auto isConstStringRef()
{
    using namespace clang::ast_matchers;
    return qualType(
               hasCanonicalType(
                   references(
                       qualType(
                           allOf(
                               isConstQualified(),
                               hasDeclaration(
                                   namedDecl( hasAnyName( "std::string", "std::basic_string" ) )
                               )
                           )
                       )
                   )
               )
           );
}

static auto refToStringRefParam()
{
    return declRefExpr( to( parmVarDecl( hasType( isConstStringRef() ) ).bind( "param" ) ) );
}

void UseStringViewCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        functionDecl(
            hasAnyParameter( parmVarDecl( hasType( isConstStringRef() ) ).bind( "param" ) )
        ).bind( "function" ),
        this
    );


    Finder->addMatcher(
        callExpr(
            forEachArgumentWithParamType(
                refToStringRefParam(),
                isConstStringRef()
            ),
            callee( functionDecl().bind( "callee" ) )
        ),
        this
    );


    Finder->addMatcher(
        callExpr(
            hasAnyArgument( refToStringRefParam() ),
            isTypeDependent()
        ).bind( "badExpr" ),
        this
    );

    Finder->addMatcher(
        cxxConstructExpr(
            forEachArgumentWithParamType(
                refToStringRefParam(),
                isConstStringRef()
            ),
            hasDeclaration( functionDecl().bind( "callee" ) )
        ),
        this
    );

    Finder->addMatcher(
        cxxUnresolvedConstructExpr( hasAnyArgument( refToStringRefParam() ) ).bind( "badExpr" ),
        this
    );

    Finder->addMatcher(
        cxxMemberCallExpr(
            on( refToStringRefParam() ),
            hasDeclaration( functionDecl().bind( "callee" ) )
        ).bind( "membercall" ),
        this
    );

    Finder->addMatcher(
        cxxCtorInitializer(
            withInitializer( refToStringRefParam() ),
            forField( fieldDecl( hasType( isConstStringRef() ) ).bind( "field" ) )
        ),
        this
    );









    Finder->addMatcher(
        declRefExpr(
            refToStringRefParam(),
            hasParent( parenListExpr( hasParent( cxxConstructorDecl() ) ) )
        ).bind( "badExpr" ),
        this
    );

    Finder->addMatcher(
        returnStmt( hasReturnValue( refToStringRefParam().bind( "badExpr" ) ) ),
        this
    );

    Finder->addMatcher(
        unaryOperator(
            hasOperatorName( "&" ),
            hasUnaryOperand( refToStringRefParam().bind( "badExpr" ) )
        ),
        this
    );
}

void UseStringViewCheck::check( const MatchFinder::MatchResult &Result )
{
    const ParmVarDecl *Param = Result.Nodes.getNodeAs<ParmVarDecl>( "param" );
    const FunctionDecl *Callee = Result.Nodes.getNodeAs<FunctionDecl>( "callee" );
    const Expr *BadExpr = Result.Nodes.getNodeAs<Expr>( "badExpr" );
    const FieldDecl *Field = Result.Nodes.getNodeAs<FieldDecl>( "field" );
    if( Callee || Field || BadExpr ) {


        if( Result.Nodes.getNodeAs<CXXMemberCallExpr>( "membercall" ) ) {
            if( const CXXMethodDecl *MethodCallee = dyn_cast<CXXMethodDecl>( Callee ) ) {
                if( !MethodCallee->getDeclName().isIdentifier() ||
                    MethodCallee->getName() != "c_str" ) {



                    return;
                }
            }
        }
        auto func_it = function_from_param_.find( Param );
        if( func_it != function_from_param_.end() ) {
            found_defns_.erase( func_it->second->getFirstDecl() );
        }
        return;
    }

    const FunctionDecl *Func = Result.Nodes.getNodeAs<FunctionDecl>( "function" );

    if( !Param || !Func ) {
        return;
    }

    if( Func->isTemplateInstantiation() ) {
        return;
    }

    if( Func->isImplicit() ) {
        return;
    }

    if( const CXXMethodDecl *Method = dyn_cast<CXXMethodDecl>( Func ) ) {
        if( Method->isVirtual() ) {
            return;
        }
    }

    function_from_param_.emplace( Param, Func );

    std::string DeclarationReplacementText = "std::string_view";
    if( !Param->getName().empty() ) {
        DeclarationReplacementText += " ";
        DeclarationReplacementText += Param->getName();
    }

    const bool IsDefinition = Func->getDefinition() == Func;
    if( IsDefinition ) {


        found_defns_.emplace(
            Func->getFirstDecl(),
            FoundStringViewDefn{ Param, Func, DeclarationReplacementText } );
        return;
    } else {


        found_decls_.emplace(
            Func->getFirstDecl(),
            FoundStringViewDecl{ Param->getSourceRange(), DeclarationReplacementText } );
        return;
    }
}

void UseStringViewCheck::onEndOfTranslationUnit()
{
    for( const std::pair<const FunctionDecl *const, FoundStringViewDefn> &p : found_defns_ ) {
        const ParmVarDecl *Param = p.second.param;
        const FunctionDecl *Func = p.second.func;


        std::vector<FixItHint> fixits;

        auto prior_matches = found_decls_.equal_range( Func->getFirstDecl() );
        for( auto it = prior_matches.first; it != prior_matches.second; ++it ) {
            const FoundStringViewDecl &prior = it->second;
            fixits.push_back( FixItHint::CreateReplacement( prior.range, prior.text ) );
        }

        const std::string DefinitionReplacementText = "const " + p.second.text;
        diag( Param->getBeginLoc(), "Parameter %0 of %1 can be std::string_view." )
                << Param << Func <<
                FixItHint::CreateReplacement( Param->getSourceRange(), DefinitionReplacementText )
                << fixits;
    }
}

}
