/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"
#include "scope.h"
#include "errors.h"


Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
}

void Program::Check() {
    nodeScope = new Scope();
    decls->DeclareAll(nodeScope);
    decls->CheckAll();
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}
void StmtBlock::Check() {
    nodeScope = new Scope();
    decls->DeclareAll(nodeScope);
    decls->CheckAll();
    stmts->CheckAll();
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) { 
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this); 
    (body=b)->SetParent(this);
}

void ConditionalStmt::Check() {
    test->Check();
    body->Check();

    Type *t = test->GetType();
    if (t != Type::boolType && t != Type::errorType) {
        ReportError::TestNotBoolean(test);
    }
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) { 
    Assert(i != NULL && t != NULL && s != NULL && b != NULL);
    (init=i)->SetParent(this);
    (step=s)->SetParent(this);
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}
void IfStmt::Check() {
    ConditionalStmt::Check();
    if (elseBody) elseBody->Check();
}


ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    Assert(e != NULL);
    (expr=e)->SetParent(this);
}
  
PrintStmt::PrintStmt(List<Expr*> *a) {    
    Assert(a != NULL);
    (args=a)->SetParentAll(this);
}

void PrintStmt::Check() {
    args->CheckAll();

    int params = args->NumElements();
    for (int i = 0; i < params; i++) {
        Expr *e = args->Nth(i);

        Type *t = e->GetType();

        if (t != Type::errorType && t != Type::intType && t != Type::boolType && t != Type::stringType) {
            ReportError::ArgMismatch(e, i + 1, t, PrintType::printType);
        }
    }
}

void BreakStmt::Check() {
    /* Walk up until we find the loop, very inefficient */
    Node *n = this;
    while (n) {
        LoopStmt *ls = dynamic_cast<LoopStmt *>(n);
        if (!ls) {
            n = n->GetParent();
            continue;
        }

        return;
    }

    ReportError::BreakOutsideLoop(this);
}

void ReturnStmt::Check() {
    expr->Check();

    /* Walk up until we find the loop, very inefficient */
    Node *n = this;
    while (n) {
        FnDecl *fd = dynamic_cast<FnDecl *>(n);
        if (!fd) {
            n = n->GetParent();
            continue;
        }

        Type *expect = fd->GetReturnType();
        Type *got = expr->GetType();

        if (!expect->IsCompatibleTo(got)) {
            ReportError::ReturnMismatch(this, got, expect);
        }

        return;
    }
}

SwitchStmt::SwitchStmt(Expr *e, List<Case*> *c, Default *d) {    
    Assert(e != NULL && c != NULL);
    (expr=e)->SetParent(this);
    (cases=c)->SetParentAll(this);
    if (d) (def = d)->SetParent(this);
}

Case::Case(IntConstant *l, List<Stmt*> *s) {    
    Assert(l != NULL && s != NULL);
    (stmts=s)->SetParentAll(this);
    (label = l)->SetParent(this);
}

Default::Default(List<Stmt*> *s) {    
    Assert(s != NULL);
    (stmts=s)->SetParentAll(this);
}
