/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
#include "errors.h"
         
Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this); 
}


VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
}
 

void VarDecl::Define() {
    Hashtable<Symbol *> *sym_tab = GetParent()->GetSymTab();
    SetSymTab(sym_tab);

    Symbol *prev = sym_tab->Lookup(id->GetName());

    if (prev)
        ReportError::DeclConflict(this, prev->GetDecl());

    Symbol *next = new Symbol(id, this, type, false);
    sym_tab->Enter(id->GetName(), next, false);
}


void VarDecl::Check() {
    type->Check();
}
 

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);     
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);

    ready = false;
}


void ClassDecl::Define() {
    /* A class begins a new scope that contains the definitions */
    SetSymTab(new Hashtable<Symbol *>);

    Hashtable<Symbol *> *sym_tab = GetParent()->GetSymTab();

    Symbol *prev = sym_tab->Lookup(id->GetName());

    if (prev)
        ReportError::DeclConflict(this, prev->GetDecl());

    Symbol *next = new Symbol(id, this, NULL, false);
    sym_tab->Enter(id->GetName(), next, false);
}


/**
 * This function fills the symbol table with the declarations in the base class and interfaces.
 * If the class base is declared after the function, it will also need to be filled.
 */
void ClassDecl::Fill() {
    if (ready) return;
    ready = true;

    if (extends) {
        Identifier *id = extends->GetID();
        /* Find the symbol for the class we are extending */
        Symbol *s_extend = GetParent()->LookupSymbol(id->GetName());

        ClassDecl *klass;

        if (!s_extend || !(klass = dynamic_cast<ClassDecl *>(s_extend->GetDecl()))) {
            ReportError::IdentifierNotDeclared(id, LookingForClass);
        } else {
            /* Make sure we have the class fields */
            klass->Fill();

            /* Copy over all of the declarations */
            Iterator<Symbol *> iter = s_extend->GetDecl()->GetSymTab()->GetIterator();

            while (1) {
                Symbol *s = iter.GetNextValue();
                if (!s) break;

                Symbol *dup = new Symbol(s->GetID(), s->GetDecl(), s->GetType(), s->IsFn());
                dup->SetOverride(true);

                sym_tab->Enter(s->GetID()->GetName(), dup, false);
            }
        }
    }

    for (int i = 0; i < implements->NumElements(); i++) {
        Identifier *id = implements->Nth(i)->GetID();
        /* Find the symbol for the interface we are implementing */
        Symbol *s_impl = GetParent()->LookupSymbol(id->GetName());
        InterfaceDecl *iface;

        if (!s_impl || !(iface = dynamic_cast<InterfaceDecl *>(s_impl->GetDecl()))) {
            ReportError::IdentifierNotDeclared(id, LookingForInterface);
        } else {
            /* Make sure we have the interface prototypes */
            iface->Fill();

            /* Copy over all of the prototypes */
            Iterator<Symbol *> iter = s_impl->GetDecl()->GetSymTab()->GetIterator();

            while (1) {
                Symbol *s = iter.GetNextValue();
                if (!s) break;

                Symbol *dup = new Symbol(s->GetID(), s->GetDecl(), s->GetType(), s->IsFn());
                dup->SetOverride(true);

                sym_tab->Enter(s->GetID()->GetName(), dup, false);
            }
        }
    }

    for (int i = 0; i < members->NumElements(); i++) {
        Decl *d = members->Nth(i);
        d->Define();
    }
}


void ClassDecl::Check() {
    Fill();

    for (int i = 0; i < members->NumElements(); i++) {
        Decl *d = members->Nth(i);
        d->Check();
    }
}


InterfaceDecl::InterfaceDecl(Identifier *n, List<Decl*> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members=m)->SetParentAll(this);

    ready = false;
}


void InterfaceDecl::Define() {
    /* An interface begins a new scope that contains the definitions */
    SetSymTab(new Hashtable<Symbol *>);

    Hashtable<Symbol *> *sym_tab = GetParent()->GetSymTab();

    Symbol *prev = sym_tab->Lookup(id->GetName());

    if (prev)
        ReportError::DeclConflict(this, prev->GetDecl());

    Symbol *next = new Symbol(id, this, NULL, false);
    sym_tab->Enter(id->GetName(), next, false);
}


void InterfaceDecl::Fill() {
    if (ready) return;
    ready = true;

    for (int i = 0; i < members->NumElements(); i++) {
        Decl *d = members->Nth(i);
        d->Define();
    }
}


void InterfaceDecl::Check() {
    Fill();

    for (int i = 0; i < members->NumElements(); i++) {
        Decl *d = members->Nth(i);
        d->Check();
    }
}

	
FnDecl::FnDecl(Identifier *n, Type *r, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r!= NULL && d != NULL);
    (returnType=r)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
}


void FnDecl::SetFunctionBody(Stmt *b) { 
    (body=b)->SetParent(this);
}


void FnDecl::Define() {
    /* A function begins a new scope that contains the function arguments */
    SetSymTab(new Hashtable<Symbol *>);

    Hashtable<Symbol *> *sym_tab = GetParent()->GetSymTab();

    Symbol *prev = sym_tab->Lookup(id->GetName());

    if (prev) {
        FnDecl *fn = dynamic_cast<FnDecl *>(prev->GetDecl());

        if (fn && prev->CanOverride()) {
            if (!fn->returnType->IsEquivalentTo(returnType)) {
                ReportError::OverrideMismatch(this);
            } else if (fn->formals->NumElements() != formals->NumElements()) {
                ReportError::OverrideMismatch(this);
            } else {
                for (int i = 0; i < formals->NumElements(); i++) {
                    VarDecl *d = formals->Nth(i);
                    VarDecl *other = fn->formals->Nth(i);

                    if (!d->GetType()->IsEquivalentTo(other->GetType())) {
                        ReportError::OverrideMismatch(this);
                        break;
                    }
                }

                prev->SetOverride(false);
                prev->SetDecl(this);
            }
        } else {
            ReportError::DeclConflict(this, prev->GetDecl());
        }
    } else {
        Symbol *next = new Symbol(id, this, NULL, true);
        sym_tab->Enter(id->GetName(), next, false);
    }
}


void FnDecl::Check() {
    for (int i = 0; i < formals->NumElements(); i++) {
        Decl *d = formals->Nth(i);
        d->Define();
    }

    for (int i = 0; i < formals->NumElements(); i++) {
        Decl *d = formals->Nth(i);
        d->Check();
    }

    returnType->Check();

    /* This is a STMT block that will start a new symbol table */
    if (body) body->Check();
}

