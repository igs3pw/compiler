/* File: ast_type.cc
 * -----------------
 * Implementation of type node classes.
 */
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h>

#include "errors.h"
 
/* Class constants
 * ---------------
 * These are public constants for the built-in base types (int, double, etc.)
 * They can be accessed with the syntax Type::intType. This allows you to
 * directly access them and share the built-in types where needed rather that
 * creates lots of copies.
 */

Type *Type::intType    = new Type("int");
Type *Type::doubleType = new Type("double");
Type *Type::voidType   = new Type("void");
Type *Type::boolType   = new Type("bool");
Type *Type::nullType   = new Type("null");
Type *Type::stringType = new Type("string");
Type *Type::errorType  = new Type("error"); 

Type::Type(const char *n) {
    Assert(n);
    typeName = strdup(n);
}



	
NamedType::NamedType(Identifier *i) : Type(*i->GetLocation()) {
    Assert(i != NULL);
    (id=i)->SetParent(this);
} 

void NamedType::Check() {
    if (!GetDeclForType()) {
        isError = true;
        ReportError::IdentifierNotDeclared(id, LookingForType);
    }
}
Decl *NamedType::GetDeclForType() {
    if (!cachedDecl && !isError) {
        Decl *declForName = FindDecl(id);
        if (declForName && (dynamic_cast<ClassDecl *>(declForName) || dynamic_cast<InterfaceDecl *>(declForName))) 
            cachedDecl = declForName;
    }
    return cachedDecl;
}

bool NamedType::IsInterface() {
    Decl *d = GetDeclForType();
    return !!(dynamic_cast<InterfaceDecl *>(d));
}

bool NamedType::IsClass() {
    Decl *d = GetDeclForType();
    return !!(dynamic_cast<ClassDecl *>(d));
}

bool NamedType::IsEquivalentTo(Type *other) {
    NamedType *ot = dynamic_cast<NamedType*>(other);
    return ot && strcmp(id->GetName(), ot->id->GetName()) == 0;
}

bool NamedType::IsCompatibleTo(Type *other) {
    if (other == Type::errorType || other == Type::nullType)
        return true;

    if (IsEquivalentTo(other))
        return true;

    NamedType *nt = dynamic_cast<NamedType*>(other);
    if (!nt)
        return false;

    Decl *d_other = nt->GetDeclForType();
    Decl *d_this = GetDeclForType();

    ClassDecl *d = dynamic_cast<ClassDecl *>(d_other);
    if (!d)
        return false;

    ClassDecl *cd = dynamic_cast<ClassDecl *>(d_this);
    InterfaceDecl *id = dynamic_cast<InterfaceDecl *>(d_this);

    if (id) {
        return d->DoImplement(id);
    } else if (cd) {
        return d->DoExtend(cd);
    } else {
        Assert(false);
        return false;
    }
}


ArrayType::ArrayType(yyltype loc, Type *et) : Type(loc) {
    Assert(et != NULL);
    (elemType=et)->SetParent(this);
}

void ArrayType::Check() {
    elemType->Check();
}

bool ArrayType::IsEquivalentTo(Type *other) {
    ArrayType *o = dynamic_cast<ArrayType*>(other);
    return (o && elemType->IsEquivalentTo(o->elemType));
}
