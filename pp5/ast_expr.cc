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
    if (!strcmp(token, "<=")) {
        Location *lt = cg->GenBinaryOp("<", left->GetVar(), right->GetVar());
        Location *eq = cg->GenBinaryOp("==", left->GetVar(), right->GetVar());
        SetVar(cg->GenBinaryOp("||", lt, eq));
    } else if (!strcmp(token, ">=")) {
        Location *gt = cg->GenBinaryOp(">", left->GetVar(), right->GetVar());
        Location *eq = cg->GenBinaryOp("==", left->GetVar(), right->GetVar());
        SetVar(cg->GenBinaryOp("||", gt, eq));
    } else if (!strcmp(token, "!=")) {
        Location *eq = cg->GenBinaryOp("==", left->GetVar(), right->GetVar());
        SetVar(cg->GenBinaryOp("-", cg->GenLoadConstant(1), eq));
    } else if (!strcmp(token, "!")) {
        SetVar(cg->GenBinaryOp("-", cg->GenLoadConstant(1), right->GetVar()));
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
    left->Emit(cg);
    right->Emit(cg);

    cg->GenAssign(left->GetVar(), right->GetVar());
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

    Location *off = cg->GenBinaryOp("+", base->GetVar(), subscript->GetVar());

    SetVar(cg->GenLoad(off));
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
        SetVar(cg->GenLoad(cg->ThisPtr, 0/*FIXME*/));
    } else {
        SetVar(vd->GetVar());
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
    FnDecl *fd;

    actuals->CheckAll();

    if (base) {
        base->Check();

        Type *tb = base->GetType();
        NamedType *nt = dynamic_cast<NamedType *>(tb);
        if (tb == Type::errorType || tb == Type::nullType) {
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
    if (!base) {
        int need = actuals->NumElements();
        for (int i = 0; i < need; i++) {
            Expr *arg = actuals->Nth(i);
            arg->Emit(cg);
        }

        for (int i = need - 1; i >= 0; i--) {
            Expr *arg = actuals->Nth(i);
            cg->GenPushParam(arg->GetVar());
        }

        Location *out = cg->GenLCall(field->GetName(), true);
        cg->GenPopParams(4 * actuals->NumElements());

        SetVar(out);
    } else {
        base->Emit(cg);

        Type *tb = base->GetType();
        NamedType *nt = dynamic_cast<NamedType *>(tb);

        if (!nt) {
            /* Array.length() */
            Location *addr = cg->GenBinaryOp("-", base->GetVar(), cg->GenLoadConstant(cg->VarSize));
            SetVar(cg->GenLoad(addr));
        } else {

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
    /* FIXME */
    Location *cnt = cg->GenLoadConstant(cg->VarSize);

    SetVar(cg->GenBuiltInCall(Alloc, cnt));
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

    Location *cnt = cg->GenBinaryOp("*", size->GetVar(), cg->GenLoadConstant(cg->VarSize));

    SetVar(cg->GenBuiltInCall(Alloc, cnt));
} 


ReadIntegerExpr::ReadIntegerExpr(yyltype loc) : Expr(loc) {
    SetType(Type::intType);
}

void ReadIntegerExpr::Emit(CodeGenerator *cg) {
}


ReadLineExpr::ReadLineExpr(yyltype loc) : Expr (loc) {
    SetType(Type::stringType);
}

void ReadLineExpr::Emit(CodeGenerator *cg) {
}
