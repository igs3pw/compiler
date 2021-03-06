/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"


Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
}

void Program::Check() {
    /* pp3: here is where the semantic analyzer is kicked off.
     *      The general idea is perform a tree traversal of the
     *      entire program, examining all constructs for compliance
     *      with the semantic rules.  Each node can have its own way of
     *      checking itself, which makes for a great use of inheritance
     *      and polymorphism in the node classes.
     */

    Hashtable<Symbol *> *sym_tab = new Hashtable<Symbol *>;
    SetSymTab(sym_tab);

    for (int i = 0; i < decls->NumElements(); i++) {
        Decl *d = decls->Nth(i);
        d->Define();
    }

    for (int i = 0; i < decls->NumElements(); i++) {
        Decl *d = decls->Nth(i);
        d->Check();
    }
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}

void StmtBlock::Check() {
    /* A statment block begins a new scope */
    Hashtable<Symbol *> *sym_tab = new Hashtable<Symbol *>;
    SetSymTab(sym_tab);

    for (int i = 0; i < decls->NumElements(); i++) {
        Decl *d = decls->Nth(i);
        d->Define();
    }

    for (int i = 0; i < decls->NumElements(); i++) {
        Decl *d = decls->Nth(i);
        d->Check();
    }

    stmts->Check();
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) { 
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this); 
    (body=b)->SetParent(this);
}

void ConditionalStmt::Check() {
    SetSymTab(GetParent()->GetSymTab());

    test->Check();
    body->Check();
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


ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    Assert(e != NULL);
    (expr=e)->SetParent(this);
}
  
PrintStmt::PrintStmt(List<Expr*> *a) {    
    Assert(a != NULL);
    (args=a)->SetParentAll(this);
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
