/* File: ast_decl.h
 * ----------------
 * In our parse tree, Decl nodes are used to represent and
 * manage declarations. There are 4 subclasses of the base class,
 * specialized for declarations of variables, functions, classes,
 * and interfaces.
 *
 * pp5: You will need to extend the Decl classes to implement 
 * code generation for declarations.
 */

#ifndef _H_ast_decl
#define _H_ast_decl

#include "ast.h"
#include "list.h"

class Type;
class NamedType;
class Identifier;
class Stmt;
class FnDecl;
class InterfaceDecl;
class Location;
class CodeGenerator;

class Decl : public Node 
{
  protected:
    Identifier *id;
  
  public:
    Decl(Identifier *name);
    friend std::ostream& operator<<(std::ostream& out, Decl *d) { return out << d->id; }
    Identifier *GetId() { return id; }
    const char *GetName() { return id->GetName(); }
    
    virtual bool ConflictsWithPrevious(Decl *prev);
};

class VarDecl : public Decl 
{
  protected:
    Type *type;
    Location *src;
    
  public:
    VarDecl(Identifier *name, Type *type);
    void Check();
    void Emit(CodeGenerator *cg);
    Type *GetDeclaredType() { return type; }
    Location *GetVar() { return src; }
    void SetVar(Location *l) { src = l; }
};

class ClassDecl : public Decl 
{
  protected:
    List<Decl*> *members;
    NamedType *extends;
    List<NamedType*> *implements;
    Type *cType;
    List<InterfaceDecl*> *convImp;
    /* Includes extended class's stuff */
    unsigned int fields;
    List<FnDecl *> *methods;

  public:
    ClassDecl(Identifier *name, NamedType *extends, 
              List<NamedType*> *implements, List<Decl*> *members);
    void Check();
    void Emit(CodeGenerator *cg);
    Scope *PrepareScope();

    bool DoExtend(ClassDecl *d);
    bool DoImplement(InterfaceDecl *d);
    Type *GetType() { return cType; }

    unsigned int NumFields() { return fields; }
    int VarDeclOffset(VarDecl *find);
};

class InterfaceDecl : public Decl 
{
  protected:
    List<FnDecl*> *members;
    
  public:
    InterfaceDecl(Identifier *name, List<FnDecl*> *members);
    void Check();
    Scope *PrepareScope();

    List<FnDecl *> *GetMethods() { return members; }
    int NumMethods() { return members->NumElements(); }
};

class FnDecl : public Decl 
{
  protected:
    List<VarDecl*> *formals;
    Type *returnType;
    Stmt *body;

    unsigned int off;
    
  public:
    FnDecl(Identifier *name, Type *returnType, List<VarDecl*> *formals);
    void SetFunctionBody(Stmt *b);
    void Check();
    bool IsMethodDecl();
    bool ConflictsWithPrevious(Decl *prev);
    bool MatchesPrototype(FnDecl *other);

    Type *GetReturnType() { return returnType; };
    List<VarDecl *> *GetArgumentTypes() { return formals; }
    bool IsEmpty() { return body == NULL; }

    void Emit(CodeGenerator *cg);
    unsigned int GetOff() { return off; }
    void SetOff(unsigned int o) { off = o; }
};

#endif
