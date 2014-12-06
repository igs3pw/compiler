/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "errors.h"
#include <string.h>


void EmptyExpr::Check() {
    SetType(Type::voidType);
}

IntConstant::IntConstant(yyltype loc, int val) : Expr(loc) {
    value = val;

    SetType(Type::intType);
}

void IntConstant::Emit(CodeGenerator *cg) {
    SetVar(cg->GenLoadConstant(value));
}

DoubleConstant::DoubleConstant(yyltype loc, double val) : Expr(loc) {
    value = val;
}

BoolConstant::BoolConstant(yyltype loc, bool val) : Expr(loc) {
    value = val;

    SetType(Type::boolType);
}

void BoolConstant::Emit(CodeGenerator *cg) {
    SetVar(cg->GenLoadConstant((value)?1:0));
}

StringConstant::StringConstant(yyltype loc, const char *val) : Expr(loc) {
    Assert(val != NULL);
    value = strdup(val);

    SetType(Type::stringType);
}

void StringConstant::Emit(CodeGenerator *cg) {
    SetVar(cg->GenLoadConstant(value));
}

NullConstant::NullConstant(yyltype loc) : Expr(loc) {
    SetType(Type::nullType);
}

void NullConstant::Emit(CodeGenerator *cg) {
    SetVar(cg->GenLoadConstant(0));
}


Operator::Operator(yyltype loc, const char *tok) : Node(loc) {
    Assert(tok != NULL);
    strncpy(tokenString, tok, sizeof(tokenString));
}
CompoundExpr::CompoundExpr(Expr *l, Operator *o, Expr *r) 
  : Expr(Join(l->GetLocation(), r->GetLocation())) {
    Assert(l != NULL && o != NULL && r != NULL);
    (op=o)->SetParent(this);
    (left=l)->SetParent(this); 
    (right=r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Operator *o, Expr *r) 
  : Expr(Join(o->GetLocation(), r->GetLocation())) {
    Assert(o != NULL && r != NULL);
    left = NULL; 
    (op=o)->SetParent(this);
    (right=r)->SetParent(this);
}

void CompoundExpr::Emit(CodeGenerator *cg) {
    if (left)
        left->Emit(cg);
    right->Emit(cg);

    char *token = op->GetToken();
    if (!strcmp(token, "==") && left->GetType() == Type::stringType) {
        SetVar(cg->GenBuiltInCall(StringEqual, left->GetVar(), right->GetVar()));
    } else if (!strcmp(token, "!=") && left->GetType() == Type::stringType) {
        Location *eq = cg->GenBuiltInCall(StringEqual, left->GetVar(), right->GetVar());
        SetVar(cg->GenBinaryOp("==", eq, cg->GenLoadConstant(0)));
    } else if (!strcmp(token, "<=")) {
        Location *lt = cg->GenBinaryOp("<", left->GetVar(), right->GetVar());
        Location *eq = cg->GenBinaryOp("==", left->GetVar(), right->GetVar());
        SetVar(cg->GenBinaryOp("||", lt, eq));
    } else if (!strcmp(token, ">=")) {
        Location *gt = cg->GenBinaryOp("<", right->GetVar(), left->GetVar());
        Location *eq = cg->GenBinaryOp("==", right->GetVar(), left->GetVar());
        SetVar(cg->GenBinaryOp("||", gt, eq));
    } else if (!strcmp(token, ">")) {
        SetVar(cg->GenBinaryOp("<", right->GetVar(), left->GetVar()));
    } else if (!strcmp(token, "!=")) {
        Location *eq = cg->GenBinaryOp("==", left->GetVar(), right->GetVar());
        SetVar(cg->GenBinaryOp("==", eq, cg->GenLoadConstant(0)));
    } else if (!strcmp(token, "!")) {
        SetVar(cg->GenBinaryOp("==", right->GetVar(), cg->GenLoadConstant(0)));
    } else if (!strcmp(token, "-") && !left) {
        SetVar(cg->GenBinaryOp("-", cg->GenLoadConstant(0), right->GetVar()));
    } else {
        SetVar(cg->GenBinaryOp(op->GetToken(), left->GetVar(), right->GetVar()));
    }
}


void ArithmeticExpr::Check() {
    if (left) {
        left->Check();
        right->Check();

        Type *lt = left->GetType();
        Type *rt = right->GetType();

        if (lt == Type::errorType || rt == Type::errorType) {
            SetType(Type::errorType);
        } else if (lt != rt || (lt != Type::intType && lt != Type::doubleType)
                            || (rt != Type::intType && rt != Type::doubleType)) {
            ReportError::IncompatibleOperands(op, lt, rt);
            SetType(Type::errorType);
        } else {
            SetType(right->GetType());
        }
    } else {
        right->Check();

        Type *rt = right->GetType();

        if (rt == Type::errorType) {
            SetType(Type::errorType);
        } else if (rt != Type::intType && rt != Type::doubleType) {
            ReportError::IncompatibleOperand(op, rt);
            SetType(Type::errorType);
        } else {
            SetType(right->GetType());
        }
    }
}

void RelationalExpr::Check() {
    left->Check();
    right->Check();

    Type *lt = left->GetType();
    Type *rt = right->GetType();

    if (lt == Type::errorType || rt == Type::errorType) {
        SetType(Type::boolType);
    } else if (lt != rt || (lt != Type::intType && lt != Type::doubleType)
                        || (rt != Type::intType && rt != Type::doubleType)) {
        ReportError::IncompatibleOperands(op, lt, rt);
        SetType(Type::errorType);
    } else {
        SetType(Type::boolType);
    }
}

void EqualityExpr::Check() {
    left->Check();
    right->Check();

    Type *lt = left->GetType();
    Type *rt = right->GetType();

    if (lt == Type::errorType || rt == Type::errorType) {
        SetType(Type::boolType);
    } else if (!lt->IsCompatibleTo(rt)) {
        ReportError::IncompatibleOperands(op, lt, rt);
        SetType(Type::errorType);
    } else {
        SetType(Type::boolType);
    }
}

void LogicalExpr::Check() {
    if (!left) {
        right->Check();

        Type *rt = right->GetType();

        if (rt == Type::errorType) {
            SetType(Type::boolType);
        } else if (rt != Type::boolType) {
            ReportError::IncompatibleOperand(op, right->GetType());
            SetType(Type::errorType);
        } else {
            SetType(Type::boolType);
        }
    } else {
        left->Check();
        right->Check();

        Type *lt = left->GetType();
        Type *rt = right->GetType();

        if (lt == Type::errorType || rt == Type::errorType) {
            SetType(Type::boolType);
        } else if (lt != Type::boolType || rt != Type::boolType) {
            ReportError::IncompatibleOperands(op, left->GetType(), right->GetType());
            SetType(Type::errorType);
        } else {
            SetType(Type::boolType);
        }
    }
}

void AssignExpr::Check() {
    left->Check();
    right->Check();

    Type *lt = left->GetType();
    Type *rt = right->GetType();

    NamedType *lnt = dynamic_cast<NamedType *>(lt);
    NamedType *rnt = dynamic_cast<NamedType *>(rt);

    if (lt == Type::errorType || rt == Type::errorType) {
        SetType(Type::errorType);
    } else if (!lt->IsCompatibleTo(rt)) {
        ReportError::IncompatibleOperands(op, lt, rt);
        SetType(Type::errorType);
    } else {
        SetType(lt);
    }
}

void AssignExpr::Emit(CodeGenerator *cg) {
    LValue *lval = dynamic_cast<LValue *>(left);
    lval->EmitStore(cg, right);

    SetVar(left->GetVar());
}


void This::Check() {
    /* Walk up until we find the class, very inefficient */
    Node *n = this;
    while (n) {
        ClassDecl *cd = dynamic_cast<ClassDecl *>(n);
        if (!cd) {
            n = n->GetParent();
            continue;
        }

        SetType(cd->GetType());

        return;
    }

    ReportError::ThisOutsideClassScope(this);

    SetType(Type::errorType);
}

void This::Emit(CodeGenerator *cg) {
    SetVar(cg->ThisPtr);
}
   
  
ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}

void ArrayAccess::Check() {
    base->Check();
    subscript->Check();

    Type *st = subscript->GetType();

    if (st != Type::intType && st != Type::errorType) {
        ReportError::SubscriptNotInteger(subscript);
    }

    ArrayType *type = dynamic_cast<ArrayType *>(base->GetType());
    if (base->GetType() == Type::errorType) {
        SetType(Type::errorType);
    } else if (!type) {
        ReportError::BracketsOnNonArray(base);
        SetType(Type::errorType);
    } else {
        SetType(type->GetElemType());
    }
}

void ArrayAccess::Emit(CodeGenerator *cg) {
    base->Emit(cg);
    subscript->Emit(cg);

    /* Error checking */
    {
        char *skip = cg->NewLabel();
        Location *zero = cg->GenLoadConstant(0);
        Location *negative = cg->GenBinaryOp("<", subscript->GetVar(), zero);
        Location *size = cg->GenLoad(base->GetVar(), -cg->VarSize);
        Location *lt = cg->GenBinaryOp("<", subscript->GetVar(), size);
        Location *gteq = cg->GenBinaryOp("==", lt, zero);
        Location *cmp = cg->GenBinaryOp("||", negative, gteq);
        cg->GenIfZ(cmp, skip);

        /* Error occured */
        Location *str = cg->GenLoadConstant("Decaf runtime error: Array subscript out of bounds\\n");
        cg->GenBuiltInCall(PrintString, str);
        cg->GenBuiltInCall(Halt);

        cg->GenLabel(skip);
    }

    Location *rel = cg->GenBinaryOp("*", cg->GenLoadConstant(4), subscript->GetVar());
    Location *abs = cg->GenBinaryOp("+", base->GetVar(), rel);

    SetVar(cg->GenLoad(abs));
}

void ArrayAccess::EmitStore(CodeGenerator *cg, Expr *src) {
    base->Emit(cg);
    subscript->Emit(cg);

    /* Error checking */
    {
        char *skip = cg->NewLabel();
        Location *zero = cg->GenLoadConstant(0);
        Location *negative = cg->GenBinaryOp("<", subscript->GetVar(), zero);
        Location *size = cg->GenLoad(base->GetVar(), -cg->VarSize);
        Location *lt = cg->GenBinaryOp("<", subscript->GetVar(), size);
        Location *gteq = cg->GenBinaryOp("==", lt, zero);
        Location *cmp = cg->GenBinaryOp("||", negative, gteq);
        cg->GenIfZ(cmp, skip);

        /* Error occured */
        Location *str = cg->GenLoadConstant("Decaf runtime error: Array subscript out of bound...");
        cg->GenBuiltInCall(PrintString, str);
        cg->GenBuiltInCall(Halt);

        cg->GenLabel(skip);
    }

    Location *rel = cg->GenBinaryOp("*", cg->GenLoadConstant(4), subscript->GetVar());
    Location *abs = cg->GenBinaryOp("+", base->GetVar(), rel);

    /* The solution seems to do this after all of the arithmetic, so I'll follow it */
    src->Emit(cg);

    cg->GenStore(abs, src->GetVar());
    SetVar(src->GetVar());
}
     

FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
}

void FieldAccess::Check() {
    if (base) {
        base->Check();
        Type *tb = base->GetType();
        NamedType *nt = dynamic_cast<NamedType *>(tb);
        if (tb == Type::errorType || tb == Type::nullType) {
            /* Error/Null type: ignore */
            SetType(Type::errorType);
        } else if (!nt) {
            /* Not a named type */
            ReportError::FieldNotFoundInBase(field, tb);
            SetType(Type::errorType);
        } else {
            Decl *klass = nt->GetDeclForType();
            if (!klass)
                return;

            Decl *dfield = klass->FindDecl(field, Node::kShallow);
            vd = dynamic_cast<VarDecl *>(dfield);

            /* What class or interface are we calling */
            InterfaceDecl *id = dynamic_cast<InterfaceDecl *>(klass);
            ClassDecl *cd = dynamic_cast<ClassDecl *>(klass);

            /* Locate class we are calling this from */
            Node *n = this;
            ClassDecl *caller = 0;
            while (n) {
                caller = dynamic_cast<ClassDecl *>(n);
                if (!caller) {
                    n = n->GetParent();
                    continue;
                }

                break;
            }

            if (!vd) {
                /* Class does not contain field */
                ReportError::FieldNotFoundInBase(field, tb);
                SetType(Type::errorType);
            } else if (!caller || (id && !caller->DoImplement(id)) || (cd && caller->DoExtend(cd))) {
                /* We don't have permission to access the field */
                ReportError::InaccessibleField(field, nt);
                SetType(Type::errorType);
            } else  {
                /* Class contains field */
                SetType(vd->GetDeclaredType());
            }
        }
    } else {
        Decl *declForName = FindDecl(field);
        vd = dynamic_cast<VarDecl *>(declForName);
        if (!vd) {
            ReportError::IdentifierNotDeclared(field, LookingForVariable);
            SetType(Type::errorType);
        } else {
            SetType(vd->GetDeclaredType());
        }
    }
}

void FieldAccess::Emit(CodeGenerator *cg) {
    ClassDecl *cd = dynamic_cast<ClassDecl *>(vd->GetParent());
    if (cd) {
        /* We are accessing a field in a class */
        Location *klass;
        if (base) {
            base->Emit(cg);
            klass = base->GetVar();
        } else {
            klass = cg->ThisPtr;
        }

        int loc = cd->VarDeclOffset(vd) * cg->VarSize;

        SetVar(cg->GenLoad(klass, loc));
    } else {
        SetVar(vd->GetVar());
    }
}

void FieldAccess::EmitStore(CodeGenerator *cg, Expr *src) {
    src->Emit(cg);

    ClassDecl *cd = dynamic_cast<ClassDecl *>(vd->GetParent());
    if (cd) {
        /* We are accessing a field in a class */
        Location *klass;
        if (base) {
            base->Emit(cg);
            klass = base->GetVar();
        } else {
            klass = cg->ThisPtr;
        }

        int loc = cd->VarDeclOffset(vd) * cg->VarSize;

        cg->GenStore(klass, src->GetVar(), loc);
        SetVar(src->GetVar());
    } else {
        cg->GenAssign(vd->GetVar(), src->GetVar());
        SetVar(src->GetVar());
    }
}


Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}

void Call::Check() {
    actuals->CheckAll();

    if (base) {
        base->Check();

        Type *tb = base->GetType();
        NamedType *nt = dynamic_cast<NamedType *>(tb);
        if (tb == Type::errorType) {
            /* Error/Null type: ignore */
            SetType(Type::errorType);
            return;
        } else if (!nt) {
            /* Not a named type */
            ArrayType *at = dynamic_cast<ArrayType *>(tb);
            if (at && !strcmp(field->GetName(), "length")) {
                 /* array.length() */
                 SetType(Type::intType);
            } else {
                ReportError::FieldNotFoundInBase(field, tb);
                SetType(Type::errorType);
            }

            return;
        } else {
            Decl *klass = nt->GetDeclForType();
            if (!klass) {
                return;
            } else {
                Decl *dfield = klass->FindDecl(field, Node::kShallow);
                fd = dynamic_cast<FnDecl *>(dfield);

                if (!fd) {
                    /* Class does not contain field */
                    ReportError::FieldNotFoundInBase(field, tb);
                    SetType(Type::errorType);
                    return;
                }
            }
        }
    } else {
        Decl *declForName = FindDecl(field);
        fd = dynamic_cast<FnDecl *>(declForName);
        if (!fd) {
            ReportError::IdentifierNotDeclared(field, LookingForFunction);
            SetType(Type::errorType);
            return;
        }
    }

    SetType(fd->GetReturnType());

    List<VarDecl *> *formals = fd->GetArgumentTypes();

    int need = formals->NumElements();
    int have = actuals->NumElements();
    if (have != need) {
        ReportError::NumArgsMismatch(field, need, have);
        return;
    }

    for (int i = 0; i < need; i++) {
        Expr *arg = actuals->Nth(i);
        VarDecl *vd = formals->Nth(i);

        if (!vd->GetDeclaredType()->IsCompatibleTo(arg->GetType())) {
            ReportError::ArgMismatch(arg, i + 1, arg->GetType(), vd->GetDeclaredType());
        }
    }
}

void Call::Emit(CodeGenerator *cg) {
    if (!fd) {
        /* Array.length() */
        base->Emit(cg);
        SetVar(cg->GenLoad(base->GetVar(), -cg->VarSize));
    } else {
        Location *fnptr = 0;
        Location *thiz = 0;

        int need = actuals->NumElements();
        for (int i = 0; i < need; i++) {
            Expr *arg = actuals->Nth(i);
            arg->Emit(cg);
        }

        if (base) {
            base->Emit(cg);

            unsigned int loc = fd->GetOff() * cg->VarSize;

            Location *vtable = cg->GenLoad(base->GetVar());
            fnptr = cg->GenLoad(vtable, loc);

            thiz = base->GetVar();
        } else if (fd->IsMethodDecl()) {
            unsigned int loc = fd->GetOff() * cg->VarSize;

            Location *vtable = cg->GenLoad(cg->ThisPtr);
            fnptr = cg->GenLoad(vtable, loc);

            thiz = cg->ThisPtr;
        }

        /* This ordering is stange, but it matches the solution's */
        for (int i = need - 1; i >= 0; i--) {
            Expr *arg = actuals->Nth(i);
            cg->GenPushParam(arg->GetVar());
        }

        if (fnptr) {
            cg->GenPushParam(thiz);

            Location *out = cg->GenACall(fnptr, fd->GetReturnType() != Type::voidType);
            cg->GenPopParams(cg->VarSize * actuals->NumElements() + cg->VarSize);

            SetVar(out);
        } else {
            char tmp[128];
            sprintf(tmp, "_%s", fd->GetId()->GetName());

            Location *out = cg->GenLCall(tmp, fd->GetReturnType() != Type::voidType);
            cg->GenPopParams(cg->VarSize * actuals->NumElements());

            SetVar(out);
        }
    }
}
 

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) { 
  Assert(c != NULL);
  (cType=c)->SetParent(this);
}

void NewExpr::Check() {
    Decl *d = cType->GetDeclForType();
    ClassDecl *cd = dynamic_cast<ClassDecl *>(d);
    if (!cd) {
        ReportError::IdentifierNotDeclared(cType->GetId(), LookingForClass);
        SetType(Type::errorType);
    } else {
        SetType(cType);
    }
}

void NewExpr::Emit(CodeGenerator *cg) {
    ClassDecl *cd = dynamic_cast<ClassDecl *>(cType->GetDeclForType());

    unsigned int size = (cd->NumFields() + 1) * cg->VarSize;

    Location *cnt = cg->GenLoadConstant(size);
    Location *addr = cg->GenBuiltInCall(Alloc, cnt);
    cg->GenStore(addr, cg->GenLoadLabel(cd->GetId()->GetName()));

    SetVar(addr);
}


NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this); 
    (elemType=et)->SetParent(this);
}

void NewArrayExpr::Check() {
    size->Check();
    elemType->Check();

    Type *st = size->GetType();

    if (st != Type::intType && st != Type::errorType) {
        ReportError::NewArraySizeNotInteger(size);
    }

    SetType(new ArrayType(*location, elemType));
}

void NewArrayExpr::Emit(CodeGenerator *cg) {
    size->Emit(cg);

    /* The provided solution does bounds checking, so included here for easy diffing */
    {
        char *skip = cg->NewLabel();
        Location *cmp = cg->GenBinaryOp("<", size->GetVar(), cg->GenLoadConstant(0));
        cg->GenIfZ(cmp, skip);

        /* Error occured */
        Location *str = cg->GenLoadConstant("Decaf runtime error: Array size is <= 0\\n");
        cg->GenBuiltInCall(PrintString, str);
        cg->GenBuiltInCall(Halt);

        cg->GenLabel(skip);
    }

    Location *cnt = cg->GenBinaryOp("+", cg->GenLoadConstant(1), size->GetVar());
    Location *four = cg->GenLoadConstant(cg->VarSize);
    Location *bytes = cg->GenBinaryOp("*", cnt, four);

    Location *arr = cg->GenBuiltInCall(Alloc, bytes);

    /* Store size */
    cg->GenStore(arr, size->GetVar());

    SetVar(cg->GenBinaryOp("+", arr, four));
} 


ReadIntegerExpr::ReadIntegerExpr(yyltype loc) : Expr(loc) {
    SetType(Type::intType);
}

void ReadIntegerExpr::Emit(CodeGenerator *cg) {
    SetVar(cg->GenBuiltInCall(ReadInteger));
}


ReadLineExpr::ReadLineExpr(yyltype loc) : Expr (loc) {
    SetType(Type::stringType);
}

void ReadLineExpr::Emit(CodeGenerator *cg) {
    SetVar(cg->GenBuiltInCall(ReadLine));
}
