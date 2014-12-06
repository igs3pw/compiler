/* File: ast_expr.h
 * ----------------
 * The Expr class and its subclasses are used to represent
 * expressions in the parse tree.  For each expression in the
 * language (add, call, New, etc.) there is a corresponding
 * node class for that construct. 
 *
 * pp5: You will need to extend the Expr classes to implement 
 * code generation for expressions.
 */


#ifndef _H_ast_expr
#define _H_ast_expr

#include "ast.h"
#include "ast_stmt.h"
#include "list.h"
#include "tac.h"
#include "codegen.h"

class NamedType; // for new
class Type; // for NewArray


class Expr : public Stmt 
{
  private:
    Type *type;
    Location *dst;
    Instruction *instr;

  public:
    Expr(yyltype loc) : Stmt(loc) {}
    Expr() : Stmt() {}

    Type *GetType() { return type; }
    void SetType(Type *t) { type = t; }
    Location *GetVar() { return dst; }
    void SetVar(Location *l) { dst = l; }
};

/* This node type is used for those places where an expression is optional.
 * We could use a NULL pointer, but then it adds a lot of checking for
 * NULL. By using a valid, but no-op, node, we save that trouble */
class EmptyExpr : public Expr
{
  public:
    void Check();
};

class IntConstant : public Expr 
{
  protected:
    int value;
  
  public:
    IntConstant(yyltype loc, int val);

    void Emit(CodeGenerator *cg);
};

class DoubleConstant : public Expr 
{
  protected:
    double value;
    
  public:
    DoubleConstant(yyltype loc, double val);
};

class BoolConstant : public Expr 
{
  protected:
    bool value;
    
  public:
    BoolConstant(yyltype loc, bool val);

    void Emit(CodeGenerator *cg);
};

class StringConstant : public Expr 
{ 
  protected:
    char *value;
    
  public:
    StringConstant(yyltype loc, const char *val);

    void Emit(CodeGenerator *cg);
};

class NullConstant: public Expr 
{
  public: 
    NullConstant(yyltype loc);

    void Emit(CodeGenerator *cg);
};

class Operator : public Node 
{
  protected:
    char tokenString[4];
    
  public:
    Operator(yyltype loc, const char *tok);
    friend std::ostream& operator<<(std::ostream& out, Operator *o) { return out << o->tokenString; }

    char *GetToken() { return tokenString; }
 };
 
class CompoundExpr : public Expr
{
  protected:
    Operator *op;
    Expr *left, *right; // left will be NULL if unary
    
  public:
    CompoundExpr(Expr *lhs, Operator *op, Expr *rhs); // for binary
    CompoundExpr(Operator *op, Expr *rhs);             // for unary

    virtual void Emit(CodeGenerator *cg);
};

class ArithmeticExpr : public CompoundExpr 
{
  public:
    ArithmeticExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    ArithmeticExpr(Operator *op, Expr *rhs) : CompoundExpr(op,rhs) {}
    void Check();
};

class RelationalExpr : public CompoundExpr 
{
  public:
    RelationalExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    void Check();
};

class EqualityExpr : public CompoundExpr 
{
  public:
    EqualityExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    const char *GetPrintNameForNode() { return "EqualityExpr"; }
    void Check();
};

class LogicalExpr : public CompoundExpr 
{
  public:
    LogicalExpr(Expr *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    LogicalExpr(Operator *op, Expr *rhs) : CompoundExpr(op,rhs) {}
    const char *GetPrintNameForNode() { return "LogicalExpr"; }
    void Check();
};

class LValue : public Expr 
{
  public:
    LValue(yyltype loc) : Expr(loc) {}

    virtual void EmitStore(CodeGenerator *cg, Expr *src) {}
};

class AssignExpr : public CompoundExpr 
{
  public:
    AssignExpr(LValue *lhs, Operator *op, Expr *rhs) : CompoundExpr(lhs,op,rhs) {}
    const char *GetPrintNameForNode() { return "AssignExpr"; }
    void Check();
    void Emit(CodeGenerator *cg);
};

class This : public Expr 
{
  public:
    This(yyltype loc) : Expr(loc) {}
    void Check();
    void Emit(CodeGenerator *cg);
};

class ArrayAccess : public LValue 
{
  protected:
    Expr *base, *subscript;
    
  public:
    ArrayAccess(yyltype loc, Expr *base, Expr *subscript);
    void Check();
    void Emit(CodeGenerator *cg);
    void EmitStore(CodeGenerator *cg, Expr *src);
};

/* Note that field access is used both for qualified names
 * base.field and just field without qualification. We don't
 * know for sure whether there is an implicit "this." in
 * front until later on, so we use one node type for either
 * and sort it out later. */
class FieldAccess : public LValue 
{
  protected:
    Expr *base;	// will be NULL if no explicit base
    Identifier *field;
    /* The cached declaration we are accessing */
    VarDecl *vd;
    
  public:
    FieldAccess(Expr *base, Identifier *field); //ok to pass NULL base
    void Check();
    void Emit(CodeGenerator *cg);
    void EmitStore(CodeGenerator *cg, Expr *src);
};

class FnDecl;

/* Like field access, call is used both for qualified base.field()
 * and unqualified field().  We won't figure out until later
 * whether we need implicit "this." so we use one node type for either
 * and sort it out later. */
class Call : public Expr 
{
  protected:
    Expr *base;	// will be NULL if no explicit base
    Identifier *field;
    List<Expr*> *actuals;
    /* The cached declaration we are accessing */
    FnDecl *fd;
    
  public:
    Call(yyltype loc, Expr *base, Identifier *field, List<Expr*> *args);
    void Check();
    void Emit(CodeGenerator *cg);
};

class NewExpr : public Expr
{
  protected:
    NamedType *cType;
    
  public:
    NewExpr(yyltype loc, NamedType *clsType);
    void Check();
    void Emit(CodeGenerator *cg);
};

class NewArrayExpr : public Expr
{
  protected:
    Expr *size;
    Type *elemType;
    
  public:
    NewArrayExpr(yyltype loc, Expr *sizeExpr, Type *elemType);
    void Check();
    void Emit(CodeGenerator *cg);
};

class ReadIntegerExpr : public Expr
{
  public:
    ReadIntegerExpr(yyltype loc);

    void Emit(CodeGenerator *cg);
};

class ReadLineExpr : public Expr
{
  public:
    ReadLineExpr(yyltype loc);

    void Emit(CodeGenerator *cg);
};

    
#endif
