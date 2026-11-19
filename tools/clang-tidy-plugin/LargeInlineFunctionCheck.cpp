#include "LargeInlineFunctionCheck.h"
#include "Utils.h"

#include "clang/AST/RecursiveASTVisitor.h"

using namespace clang::ast_matchers;

namespace clang::tidy::cata
{

static constexpr unsigned DefaultMaxStatements = 2;

LargeInlineFunctionCheck::LargeInlineFunctionCheck( StringRef Name, ClangTidyContext *Context )
    : ClangTidyCheck( Name, Context )
    , MaxStatements( Options.get( "MaxStatements", DefaultMaxStatements ) )
{}

void LargeInlineFunctionCheck::storeOptions( ClangTidyOptions::OptionMap &Opts )
{
    Options.store( Opts, "MaxStatements", MaxStatements );
}

void LargeInlineFunctionCheck::registerMatchers( MatchFinder *Finder )
{
    Finder->addMatcher(
        functionDecl(
            anyOf( isInline(), cxxMethodDecl() ),
            hasAnyBody( compoundStmt().bind( "body" ) )
        ).bind( "decl" ),
        this
    );
}

struct CountStmtsASTVisitor final
    : public RecursiveASTVisitor<CountStmtsASTVisitor> {
    using Base = RecursiveASTVisitor<CountStmtsASTVisitor>;

    int Count = 0;

    bool TraverseStmt( Stmt *Node ) {
        if( Node && !isa<Expr>( Node ) ) {
            ++Count;
        }

        return Base::TraverseStmt( Node );
    }
};

static void CheckDecl( LargeInlineFunctionCheck &Check,
                       const MatchFinder::MatchResult &Result )
{
    const FunctionDecl *ThisDecl = Result.Nodes.getNodeAs<FunctionDecl>( "decl" );
    if( !ThisDecl ) {
        return;
    }

    const SourceManager &SM = *Result.SourceManager;


    if( ThisDecl->getPreviousDecl() ) {
        return;
    }


    if( ThisDecl->isImplicit() ) {
        return;
    }



    const DeclContext *Parent = ThisDecl->getParent();
    while( const CXXRecordDecl *ParentAsRecord = dyn_cast_or_null<CXXRecordDecl>( Parent ) ) {
        if( ParentAsRecord->isTemplated() || ParentAsRecord->isLambda() ) {
            return;
        }
        Parent = ParentAsRecord->getParent();
    }


    if( ThisDecl->getTemplatedKind() != FunctionDecl::TK_NonTemplate ) {
        return;
    }


    if( getContainingFunction( Result, ThisDecl ) ) {
        return;
    }

    SourceLocation ExpansionBegin = SM.getFileLoc( ThisDecl->getBeginLoc() );
    if( ExpansionBegin != ThisDecl->getBeginLoc() ) {

        return;
    }


    if( !isInHeader( ThisDecl->getBeginLoc(), SM ) ) {

        if( ThisDecl->isInlineSpecified() ) {
            Check.diag(
                ThisDecl->getBeginLoc(),
                "Function %0 declared inline in a cpp file.  'inline' should only be used in "
                "header files."
            ) << ThisDecl;
        }
        return;
    }

    const CompoundStmt *Body = Result.Nodes.getNodeAs<CompoundStmt>( "body" );
    if( !Body ) {
        return;
    }

    CountStmtsASTVisitor Visitor;
    Visitor.TraverseDecl( const_cast<FunctionDecl *>( ThisDecl ) );
    unsigned NumLines = Visitor.Count;
    if( NumLines <= Check.getMaxStatements() + 1 ) {
        return;
    }

    unsigned MaxStatements = Check.getMaxStatements();

    Check.diag(
        ThisDecl->getBeginLoc(),
        "Function %0 declared inline but contains more than %1 statements.  Consider moving the "
        "definition to a cpp file."
    ) << ThisDecl << MaxStatements;
}

void LargeInlineFunctionCheck::check( const MatchFinder::MatchResult &Result )
{
    CheckDecl( *this, Result );
}

}
