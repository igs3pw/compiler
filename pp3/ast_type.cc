/* File: ast_type.cc
 * -----------------
 * Implementation of type node classes.
 */
#include "ast_type.h"
#include "ast_decl.h"
#include "errors.h"
#include <string.h>

 
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
    SetSymTab(GetParent()->GetSymTab());

    Symbol *obj = GetParent()->LookupSymbol(id->GetName());

    if (!obj) {
        ReportError::IdentifierNotDeclared(id, LookingForType);
        /* Define type here? */
    }
}


bool NamedType::IsEquivalentTo(Type *other) {
    if (other == errorType || other == nullType)
        return true;

    NamedType *n = dynamic_cast<NamedType *>(other);

    if (!n)
        return false;

    return strcmp(n->id->GetName(), id->GetName()) == 0;
}


ArrayType::ArrayType(yyltype loc, Type *et) : Type(loc) {
    Assert(et != NULL);
    (elemType=et)->SetParent(this);
}


void ArrayType::Check() {
    SetSymTab(GetParent()->GetSymTab());

    elemType->Check();
}


bool ArrayType::IsEquivalentTo(Type *other) {
    if (other == errorType || other == nullType)
        return true;

    ArrayType *a = dynamic_cast<ArrayType *>(other);

    if (!a)
        return false;

    return a->elemType->IsEquivalentTo(elemType);
}
