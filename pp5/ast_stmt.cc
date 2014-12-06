/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"
#include "errors.h"


Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
}

void Program::Check() {
    /* You can use your pp3 semantic analysis or leave it out if
     * you want to avoid the clutter.  We won't test pp5 against 
     * semantically-invalid programs.
     */
    nodeScope = new Scope();
    decls->DeclareAll(nodeScope);
    decls->CheckAll();
}
void Program::Emit() {
    /* pp5: here is where the code generation is kicked off.
     *      The general idea is perform a tree traversal of the
     *      entire program, generating instructions as you go.
     *      Each node can have its own way of translating itself,
     *      which makes for a great use of inheritance and
     *      polymorphism in the node classes.
     */

    CodeGenerator *cg = new CodeGenerator;

    Emit(cg);

    cg->DoFinalCodeGen();

    delete cg;
}

void Program::Emit(CodeGenerator *cg) {
    decls->EmitAll(cg);
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

void StmtBlock::Emit(CodeGenerator *cg) {
    decls->EmitAll(cg);
    stmts->EmitAll(cg);
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

void BreakStmt::Check() {
    /* Walk up until we find the loop, very inefficient */
    Node *n = this;
    while (n) {
        LoopStmt *ls = dynamic_cast<LoopStmt *>(n);
        if (!ls) {
            n = n->GetParent();
            continue;
        }

        stop = ls;

        return;
    }

    ReportError::BreakOutsideLoop(this);
}

void BreakStmt::Emit(CodeGenerator *cg) {
    cg->GenGoto(stop->GetStop());
}


void WhileStmt::Emit(CodeGenerator *cg) {
    const char *cont = cg->NewLabel();
    const char *stop = cg->NewLabel();

    cg->GenLabel(cont);
    /* while (test) */
    test->Emit(cg);
    cg->GenIfZ(test->GetVar(), stop);
    /* while (...) body */
    body->Emit(cg);
    cg->GenGoto(cont);
    cg->GenLabel(stop);
}


ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) { 
    Assert(i != NULL && t != NULL && s != NULL && b != NULL);
    (init=i)->SetParent(this);
    (step=s)->SetParent(this);
}

void ForStmt::Check() {
    ConditionalStmt::Check();
    init->Check();
    step->Check();
}

void ForStmt::Emit(CodeGenerator *cg) {
    const char *cont = cg->NewLabel();
    const char *stop = cg->NewLabel();

    init->Emit(cg);
    cg->GenLabel(cont);
    /* for(...;test;...) */
    test->Emit(cg);
    cg->GenIfZ(test->GetVar(), stop);
    /* for(...;...;...) body */
    body->Emit(cg);
    /* for(...;...;step) */
    step->Emit(cg);
    cg->GenGoto(cont);
    cg->GenLabel(stop);
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

void IfStmt::Emit(CodeGenerator *cg) {
    const char *skip = cg->NewLabel();
    const char *stop = cg->NewLabel();

    test->Emit(cg);
    cg->GenIfZ(test->GetVar(), skip);
    body->Emit(cg);
    if (elseBody) {
        cg->GenGoto(stop);
        cg->GenLabel(skip);
        elseBody->Emit(cg);
        cg->GenLabel(stop);
    } else {
        cg->GenLabel(skip);
    }
}


ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    Assert(e != NULL);
    (expr=e)->SetParent(this);
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

void ReturnStmt::Emit(CodeGenerator *cg) {
    expr->Emit(cg);
    cg->GenReturn(expr->GetVar());
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
            ReportError::PrintArgMismatch(e, i + 1, t);
        }
    }
}

void PrintStmt::Emit(CodeGenerator *cg) {
    for (int i = 0; i < args->NumElements(); i++) {
        Expr *e = args->Nth(i);
        Type *t = e->GetType();

        e->Emit(cg);

        if (t == Type::intType) {
            cg->GenBuiltInCall(PrintInt, e->GetVar());
        } else if (t == Type::boolType) {
            cg->GenBuiltInCall(PrintBool, e->GetVar());
        } else if (t == Type::stringType) {
            cg->GenBuiltInCall(PrintString, e->GetVar());
        }
    }
}
