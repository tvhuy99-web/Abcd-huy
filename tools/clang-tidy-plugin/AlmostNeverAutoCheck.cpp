#include "AlmostNeverAutoCheck.h"

#include <cassert>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchersInternal.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Lexer.h>
#include <llvm/ADT/APInt.h>
#include <llvm/Support/Casting.h>
#include <map>
#include <string>
#include <tuple>
#include <utility>

#include "Utils.h"
#include "clang/AST/OperationKinds.h"
#include "../../src/cata_assert.h"

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

void AlmostNeverAutoCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        varDecl(

            unless( hasParent( lambdaExpr() ) ),
            anyOf(
                varDecl( hasType( autoType() ) ),
                varDecl( hasType( references( autoType() ) ) )
            )
        ).bind( "decl" ),
        this
    );
}

static void CheckDecl( AlmostNeverAutoCheck &Check,
                       const MatchFinder::MatchResult &Result )
{
    const VarDecl *AutoVarDecl = Result.Nodes.getNodeAs<VarDecl>( "decl" );

    if( !AutoVarDecl ) {
        return;
    }

    if( AutoVarDecl->isImplicit() ) {


        return;
    }

    SourceRange RangeToReplace = AutoVarDecl->getTypeSourceInfo()->getTypeLoc().getSourceRange();

    const SourceManager *SM = Result.SourceManager;
    SourceLocation ExpansionBegin = SM->getFileLoc( RangeToReplace.getBegin() );
    if( ExpansionBegin != RangeToReplace.getBegin() ) {

        return;
    }

    const FunctionDecl *ContainingFunc = getContainingFunction( Result, AutoVarDecl );
    if( ContainingFunc &&
        ContainingFunc->getTemplateSpecializationKind() == TSK_ImplicitInstantiation ) {
        return;
    }

    QualType VarDeclType = AutoVarDecl->getType();
    const Expr *Initializer = AutoVarDecl->getAnyInitializer();

    if( !Initializer ) {


        if( VarDeclType.getTypePtr()->isDependentType() ) {

            return;
        }
        Check.diag(
            RangeToReplace.getBegin(),
            "Avoid auto in declaration of %0."
        ) << AutoVarDecl;
        return;
    }

    QualType AutoTp = Initializer->getType();
    bool WasRRef = VarDeclType.getTypePtr()->isRValueReferenceType();
    bool WasLRef = VarDeclType.getTypePtr()->isLValueReferenceType();
    VarDeclType = VarDeclType.getNonReferenceType();
    bool WasConst = VarDeclType.isLocalConstQualified();
    bool IsConstexpr = AutoVarDecl->isConstexpr();
    if( WasConst && !IsConstexpr ) {
        AutoTp.addConst();
    }

    if( VarDeclType.getTypePtr()->isArrayType() ) {
        return;
    }

    PrintingPolicy Policy( LangOptions{} );
    Policy.adjustForCPlusPlus();
    std::string TypeStr = AutoTp.getAsString( Policy );

    std::string DesugaredTypeStr;


    if( QualifierCollector().strip( AutoTp ) ) {
        QualType DesugaredAutoTp = AutoTp.getDesugaredType( *Result.Context );
        DesugaredTypeStr = DesugaredAutoTp.getAsString( Policy );
    }



    SourceLocation MaybeNewBegin = RangeToReplace.getBegin().getLocWithOffset( -6 );
    std::string TextBeforeBegin( SM->getCharacterData( MaybeNewBegin ), 10 );
    if( TextBeforeBegin == "const auto" ) {
        RangeToReplace.setBegin( MaybeNewBegin );
    }



    std::string TextFromEnd( SM->getCharacterData( RangeToReplace.getEnd() ), 10 );
    if( TextFromEnd == "auto const" ) {
        RangeToReplace.setEnd( RangeToReplace.getEnd().getLocWithOffset( 10 ) );
    }

    if( TypeStr == "auto" ) {



        return;
    }


    if( TypeStr.find_first_of( "(<" ) != std::string::npos ) {
        return;
    }




    for( std::string Fragment : {
             "terator", "element_type", "mapped_type", "value_type", "nternal", "__"
         } ) {
        if( std::search( TypeStr.begin(), TypeStr.end(), Fragment.begin(), Fragment.end() ) !=
            TypeStr.end() ) {
            return;
        }
        if( std::search( DesugaredTypeStr.begin(), DesugaredTypeStr.end(), Fragment.begin(),
                         Fragment.end() ) != DesugaredTypeStr.end() ) {
            return;
        }
    }

    if( WasLRef ) {
        TypeStr += " &";
    } else if( WasRRef ) {
        TypeStr += " &&";
    }

    Check.diag(
        RangeToReplace.getBegin(),
        "Avoid auto in declaration of %0."
    ) << AutoVarDecl <<
      FixItHint::CreateReplacement( RangeToReplace, TypeStr );
}

void AlmostNeverAutoCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckDecl( *this, Result );
}

}
