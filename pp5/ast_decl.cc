/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
#include "scope.h"
#include "errors.h"
#include "codegen.h"
        
         
Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this); 
}

bool Decl::ConflictsWithPrevious(Decl *prev) {
    ReportError::DeclConflict(this, prev);
    return true;
}


VarDecl::VarDecl(Identifier *n, Type *t) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
}

void VarDecl::Check() {
    type->Check();
}

void VarDecl::Emit(CodeGenerator *cg) {
    SetVar(cg->GenVar(id->GetName()));
}
  

ClassDecl::ClassDecl(Identifier *n, NamedType *ex, List<NamedType*> *imp, List<Decl*> *m) : Decl(n) {
    // extends can be NULL, impl & mem may be empty lists but cannot be NULL
    Assert(n != NULL && imp != NULL && m != NULL);     
    extends = ex;
    if (extends) extends->SetParent(this);
    (implements=imp)->SetParentAll(this);
    (members=m)->SetParentAll(this);
    cType = new NamedType(n);
    cType->SetParent(this);
    convImp = NULL;
}

void ClassDecl::Check() {

    if (extends && !extends->IsClass()) {
        ReportError::IdentifierNotDeclared(extends->GetId(), LookingForClass);
        extends = NULL;
    }
    for (int i = 0; i < implements->NumElements(); i++) {
        NamedType *in = implements->Nth(i);
        if (!in->IsInterface()) {
            ReportError::IdentifierNotDeclared(in->GetId(), LookingForInterface);
            implements->RemoveAt(i--);
        }
    }
    PrepareScope();
    members->CheckAll();
}

void ClassDecl::Emit(CodeGenerator *cg) {
    //members->EmitAll(cg);
    for (int i = 0; i < members->NumElements(); i++) {
        FnDecl *fd = dynamic_cast<FnDecl *>(members->Nth(i));

        if (fd)
            fd->Emit(cg);
    }

    List<const char *> *labels = new List<const char *>;

    for (int i = 0; i < methods->NumElements(); i++) {
        FnDecl *fd = methods->Nth(i);
        ClassDecl *cd = dynamic_cast<ClassDecl *>(fd->GetParent());

        char tmp[128];
        sprintf(tmp, "_%s.%s", cd->GetId()->GetName(), fd->GetId()->GetName());

        labels->Append(strdup(tmp));
    }

    cg->GenVTable(id->GetName(), labels);
}

// This is not done very cleanly. I should sit down and sort this out. Right now
// I was using the copy-in strategy from the old compiler, but I think the link to
// parent may be the better way now.
Scope *ClassDecl::PrepareScope()
{
    if (nodeScope) return nodeScope;
    nodeScope = new Scope();  

    methods = new List<FnDecl *>;

    if (extends) {
        ClassDecl *ext = dynamic_cast<ClassDecl*>(parent->FindDecl(extends->GetId())); 
        if (ext) nodeScope->CopyFromScope(ext->PrepareScope(), this);

        fields = ext->fields;

        for (int i = 0; i < ext->methods->NumElements(); i++) {
            methods->Append(ext->methods->Nth(i));
        }
    } else {
        fields = 0;
    }
    convImp = new List<InterfaceDecl*>;
    for (int i = 0; i < implements->NumElements(); i++) {
        NamedType *in = implements->Nth(i);
        InterfaceDecl *id = dynamic_cast<InterfaceDecl*>(in->FindDecl(in->GetId()));
        if (id) {
	    nodeScope->CopyFromScope(id->PrepareScope(), NULL);
            convImp->Append(id);

            List<FnDecl *> *other = id->GetMethods();
            for (int i = 0; i < other->NumElements(); i++) {
                methods->Append(other->Nth(i));
            }
	}
    }

    /* Count number of fields and methods */
    for (int i = 0; i < members->NumElements(); i++) {
        VarDecl *vd = dynamic_cast<VarDecl *>(members->Nth(i));
        FnDecl *fd = dynamic_cast<FnDecl *>(members->Nth(i));

        if (vd) {
            fields++;
        } else if (fd) {
            FnDecl *d = dynamic_cast<FnDecl *>(nodeScope->Lookup(fd->GetId()));
            if (!d) {
                /* Not overriden, this is a new method */
                fd->SetOff(methods->NumElements());
                methods->Append(fd);
            } else {
                /* Otherwise copy that method's location in the VTable */
                /* TODO: does not work with interfaces */
                unsigned int idx = d->GetOff();

                fd->SetOff(idx);

                methods->RemoveAt(idx);
                methods->InsertAt(fd, idx);
            }
        }
    }

    members->DeclareAll(nodeScope);

    /* Check if something isn't implemented */
    for (int i = 0; i < implements->NumElements(); i++) {
        NamedType *in = implements->Nth(i);
        InterfaceDecl *id = dynamic_cast<InterfaceDecl*>(in->FindDecl(in->GetId()));
        if (id) {
            Scope *s = id->PrepareScope();
            Iterator<Decl*> iter = s->GetIterator();
            Decl *decl;
            while ((decl = iter.GetNextValue()) != NULL) {
                FnDecl *fd = dynamic_cast<FnDecl *>(nodeScope->Lookup(decl->GetId()));
                if (!fd)
                    continue;

                if (fd->IsEmpty()) {
                    ReportError::InterfaceNotImplemented(this, new NamedType(in->GetId()));
                }
            }
        }
    }

    return nodeScope;
}

bool ClassDecl::DoImplement(InterfaceDecl *d) {
    for (int i = 0; i < implements->NumElements(); i++) {
        NamedType *in = implements->Nth(i);
        InterfaceDecl *id = dynamic_cast<InterfaceDecl*>(in->FindDecl(in->GetId()));
        if (id == d)
            return true;
    }

    /* Maybe a subclass implements it */
    if (extends) {
        ClassDecl *ext = dynamic_cast<ClassDecl*>(parent->FindDecl(extends->GetId())); 
        if (ext)
            return ext->DoImplement(d);
    }

    return false;
}

bool ClassDecl::DoExtend(ClassDecl *d) {
    if (extends) {
        ClassDecl *ext = dynamic_cast<ClassDecl*>(parent->FindDecl(extends->GetId())); 
        if (d == ext)
            return true;
        else if (ext)
            return ext->DoExtend(d);
    }

    return false;
}

int ClassDecl::VarDeclOffset(VarDecl *find) {
    int off = 0;
    if (extends) {
        ClassDecl *ext = dynamic_cast<ClassDecl*>(parent->FindDecl(extends->GetId())); 
        if (ext)
            off = ext->fields;
    }

    for (int i = 0; i < members->NumElements(); i++) {
        VarDecl *vd = dynamic_cast<VarDecl *>(members->Nth(i));

        if (!vd)
            continue;

        off++;

        if (vd != find)
            continue;

        return off;
    }

    return -1;
}


InterfaceDecl::InterfaceDecl(Identifier *n, List<FnDecl*> *m) : Decl(n) {
    Assert(n != NULL && m != NULL);
    (members=m)->SetParentAll(this);
}

void InterfaceDecl::Check() {
    PrepareScope();
    members->CheckAll();
}
  
Scope *InterfaceDecl::PrepareScope() {
    if (nodeScope) return nodeScope;
    nodeScope = new Scope();  
    members->DeclareAll(nodeScope);
    return nodeScope;
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

void FnDecl::Check() {
    returnType->Check();
    if (body) {
        nodeScope = new Scope();
        formals->DeclareAll(nodeScope);
        formals->CheckAll();
	body->Check();
    }
}

bool FnDecl::ConflictsWithPrevious(Decl *prev) {
 // special case error for method override
    FnDecl *fn_prev = dynamic_cast<FnDecl*>(prev);
    if (fn_prev && IsMethodDecl() && fn_prev->IsMethodDecl() && parent != prev->GetParent()) { 
        if (!MatchesPrototype(fn_prev)) {
            ReportError::OverrideMismatch(this);
            return true;
        }
        return false;
    }
    ReportError::DeclConflict(this, prev);
    return true;
}

bool FnDecl::IsMethodDecl() 
  { return dynamic_cast<ClassDecl*>(parent) != NULL || dynamic_cast<InterfaceDecl*>(parent) != NULL; }

bool FnDecl::MatchesPrototype(FnDecl *other) {
    if (!returnType->IsEquivalentTo(other->returnType)) return false;
    if (formals->NumElements() != other->formals->NumElements())
        return false;
    for (int i = 0; i < formals->NumElements(); i++)
        if (!formals->Nth(i)->GetDeclaredType()->IsEquivalentTo(other->formals->Nth(i)->GetDeclaredType()))
            return false;
    return true;
}

void FnDecl::Emit(CodeGenerator *cg) {
    int haveThis = 0;

    if (IsMethodDecl()) {
        haveThis = 1;

        Decl *d = dynamic_cast<ClassDecl*>(parent);

        char tmp[128];
        sprintf(tmp, "_%s.%s", d->GetId()->GetName(), id->GetName());
        cg->GenLabel(tmp);
    } else if (!strcmp("main", id->GetName())) {
        cg->GenLabel("main");
    } else {
        char tmp[128];
        sprintf(tmp, "_%s", id->GetName());
        cg->GenLabel(tmp);
    }

    cg->GenBeginFunc();

    for (int i = 0; i < formals->NumElements(); i++) {
        VarDecl *decl = formals->Nth(i);

        decl->SetVar(new Location(fpRelative, cg->OffsetToFirstParam + (haveThis + i) * cg->VarSize, decl->GetId()->GetName()));
    }

    if (body)
        body->Emit(cg);
    cg->GenEndFunc();
}

