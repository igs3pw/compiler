/* File: ast.h
 * ----------- 
 * This file defines the abstract base class Node and the concrete 
 * Identifier and Error node subclasses that are used through the tree as 
 * leaf nodes. A parse tree is a hierarchical collection of ast nodes (or, 
 * more correctly, of instances of concrete subclassses such as VarDecl,
 * ForStmt, and AssignExpr).
 * 
 * Location: Each node maintains its lexical location (line and columns in 
 * file), that location can be NULL for those nodes that don't care/use 
 * locations. The location is typcially set by the node constructor.  The 
 * location is used to provide the context when reporting semantic errors.
 *
 * Parent: Each node has a pointer to its parent. For a Program node, the 
 * parent is NULL, for all other nodes it is the pointer to the node one level
 * up in the parse tree.  The parent is not set in the constructor (during a 
 * bottom-up parse we don't know the parent at the time of construction) but 
 * instead we wait until assigning the children into the parent node and then 
 * set up links in both directions. The parent link is typically not used 
 * during parsing, but is more important in later phases.
 *
 * Semantic analysis: For pp3 you are adding "Check" behavior to the ast
 * node classes. Your semantic analyzer should do an inorder walk on the
 * parse tree, and when visiting each node, verify the particular
 * semantic rules that apply to that construct.

 */

#ifndef _H_ast
#define _H_ast

#include <stdlib.h>   // for NULL
#include "location.h"
#include "hashtable.h"
#include <iostream>

class Decl;
class Type;
class Identifier;

class Symbol {
  protected:
    Identifier *id;
    Decl *decl;
    Type *type;

    bool func;
    bool override;

  public:
    Symbol(Identifier *i, Decl *d, Type *t, bool f) : id(i), decl(d), type(t), func(f), override(false) {};

    Identifier *GetID() { return id; }
    Decl *GetDecl() { return decl; }
    void SetDecl(Decl *d) { decl = d; }
    Type *GetType() { return type; }
    void SetType(Type *t) { type = t; }
    bool IsFn() { return func; }
    bool CanOverride() { return override; }
    void SetOverride(bool o) { override = o; }
};

class Node 
{
  protected:
    yyltype *location;
    Node *parent;

    Hashtable<Symbol *> *sym_tab;

  public:
    Node(yyltype loc);
    Node();
    
    yyltype *GetLocation()   { return location; }
    void SetParent(Node *p)  { parent = p; }
    Node *GetParent()        { return parent; }
    virtual void Check()     {
      Hashtable<Symbol *> *sym_tab = GetParent()->GetSymTab();
      SetSymTab(sym_tab);
    }
    void SetSymTab(Hashtable<Symbol *> *st) { sym_tab = st; }
    Hashtable<Symbol *> *GetSymTab() { return sym_tab; }

    Symbol *LookupSymbol(const char *);
};
   

class Identifier : public Node 
{
  protected:
    char *name;
    
  public:
    Identifier(yyltype loc, const char *name);
    friend std::ostream& operator<<(std::ostream& out, Identifier *id) { return out << id->name; }

    char *GetName() { return name; }
};


// This node class is designed to represent a portion of the tree that 
// encountered syntax errors during parsing. The partial completed tree
// is discarded along with the states being popped, and an instance of
// the Error class can stand in as the placeholder in the parse tree
// when your parser can continue after an error.
class Error : public Node
{
  public:
    Error() : Node() {}
};



#endif
