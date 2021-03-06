/* File: parser.y
 * --------------
 * Yacc input file to generate the parser for the compiler.
 *
 * pp5: add parser rules and tree construction from your past projects. 
 *      You should not need to make any significant changes in the parser itself. 
 *      After parsing completes, if no errors were found, the parser calls
 *      program->Emit() to kick off the code generation pass. The
 *      interesting work happens during the tree traversal.
 */

%{

#include "scanner.h" // for yylex
#include "parser.h"
#include "errors.h"

void yyerror(const char *msg); // standard error-handling routine

%}

/* "The operators for all expressions are evaluated left to right" */
%left '='
%left T_Or
%left T_And
%left T_Equal T_NotEqual
%left '<' T_LessEqual '>' T_GreaterEqual
%left '+' '-'
%left '*' '/' '%'
%left '!' T_Inc T_Dec
%left '[' '.'

/* Hack for dangling else */
%right ')' T_Else
 
/* yylval 
 * ------
 */
%union {
    int integerConstant;
    bool boolConstant;
    char *stringConstant;
    double doubleConstant;
    char identifier[MaxIdentLen+1]; // +1 for terminating null
    Decl *decl;
    VarDecl *var;
    FnDecl *fDecl;
    ClassDecl *cDecl;
    InterfaceDecl *iDecl;
    Type *type;
    Stmt *stmt;
    NamedType *ntype;
    Expr *expr;
    LValue *lvalue;
    //Case *kase;
    //Default *def;
    List<Stmt*> *stmtList;
    List<VarDecl*> *varList;
    List<Decl*> *declList;
    List<FnDecl*> *fnDeclList;
    List<NamedType*> *ntList;
    List<Expr*> *exprList;
    //List<Case*> *caseList;
}


/* Tokens
 * ------
 */
%token   T_Void T_Bool T_Int T_Double T_String T_Class 
%token   T_LessEqual T_GreaterEqual T_Equal T_NotEqual T_Dims
%token   T_And T_Or T_Null T_Extends T_This T_Interface T_Implements
%token   T_While T_For T_If T_Else T_Return T_Break
%token   T_New T_NewArray T_Print T_ReadInteger T_ReadLine
%token   T_Inc T_Dec T_Switch T_Case T_Default

%token   <identifier> T_Identifier
%token   <stringConstant> T_StringConstant 
%token   <integerConstant> T_IntConstant
%token   <doubleConstant> T_DoubleConstant
%token   <boolConstant> T_BoolConstant


/* Non-terminal types
 * ------------------
 */
%type <declList>  DeclList FieldList Fields
%type <fnDeclList> ProtoList Protos
%type <decl>      Decl
%type <type>      Type 
%type <var>       Variable VarDecl
%type <varList>   Formals FormalList VarDecls
%type <fDecl>     FnDecl FnHeader Prototype
%type <stmtList>  StmtList Stmts
%type <stmt>      StmtBlock Stmt IfStmt WhileStmt ForStmt BreakStmt RetStmt PrintStmt /*SwitchStmt*/
%type <cDecl>     ClassDecl
%type <iDecl>     IfaceDecl
%type <ntype>     Extends
%type <ntList>    Impls ImplList
%type <expr>      Expr Constant Call
%type <exprList>  ExprList Actuals
%type <lvalue>    LValue
//%type <caseList>  CaseList
//%type <kase>      Case
//%type <def>       DefCase

%%
/* Rules
 * -----
	 
 */
Program   :    DeclList            { 
                                      @1; 
                                      Program *program = new Program($1);
                                      // if no errors, advance to next phase
                                      if (ReportError::NumErrors() == 0) 
                                          program->Check(); 
                                      if (ReportError::NumErrors() == 0) 
                                          program->Emit();
                                    }
          ;

DeclList  :    DeclList Decl        { ($$=$1)->Append($2); }
          |    Decl                 { ($$ = new List<Decl*>)->Append($1); };

Decl      :    VarDecl              { $$=$1; }
          |    FnDecl               { $$=$1; }
          |    ClassDecl            { $$=$1; }
          |    IfaceDecl            { $$=$1; }
;

VarDecl   :    Variable ';'         { $$=$1; }
; 

Variable   :   Type T_Identifier    { $$ = new VarDecl(new Identifier(@2, $2), $1); }
;

Type      :    T_Int                { $$ = Type::intType; }
          |    T_Bool               { $$ = Type::boolType; }
          |    T_String             { $$ = Type::stringType; }
          |    T_Double             { $$ = Type::doubleType; }
          |    T_Identifier         { $$ = new NamedType(new Identifier(@1,$1)); }
          |    Type T_Dims          { $$ = new ArrayType(Join(@1, @2), $1); }
;

FnDecl    :    FnHeader StmtBlock   { ($$=$1)->SetFunctionBody($2); }
;

FnHeader  :    Type T_Identifier '(' Formals ')'  
                                    { $$ = new FnDecl(new Identifier(@2, $2), $1, $4); }
          |    T_Void T_Identifier '(' Formals ')' 
                                    { $$ = new FnDecl(new Identifier(@2, $2), Type::voidType, $4); }
;

Formals   :    FormalList           { $$ = $1; }
          |    /* empty */          { $$ = new List<VarDecl*>; }
;

FormalList:    FormalList ',' Variable  
                                    { ($$=$1)->Append($3); }
          |    Variable             { ($$ = new List<VarDecl*>)->Append($1); }
;

ClassDecl :    T_Class T_Identifier Extends Impls '{' Fields '}'
                                    { $$ = new ClassDecl(new Identifier(@2, $2), $3, $4, $6); }
;

Extends   :    T_Extends T_Identifier
                                    { $$ = new NamedType(new Identifier(@2, $2)); }
          |                         { $$ = NULL; }
;

Impls     :    ImplList             { $$ = $1; }
          |                         { $$ = new List<NamedType*>; }
;

ImplList  :    ImplList ',' T_Identifier
                                    { ($$ = $1)->Append(new NamedType(new Identifier(@3, $3))); }
          |    T_Implements T_Identifier
                                    { ($$ = new List<NamedType*>)->Append(new NamedType(new Identifier(@2, $2))); }
;

Fields    :    FieldList             { $$ = $1; }
          |                          { $$ = new List<Decl*>; }
;

FieldList :    FieldList VarDecl     { ($$ = $1)->Append($2); }
          |    FieldList FnDecl      { ($$ = $1)->Append($2); }
          |    VarDecl               { ($$ = new List<Decl*>)->Append($1); }
          |    FnDecl                { ($$ = new List<Decl*>)->Append($1); }
;

IfaceDecl :    T_Interface T_Identifier '{' Protos '}'
                                    { $$ = new InterfaceDecl(new Identifier(@2, $2), $4); }
;

Protos    :    ProtoList            { $$ = $1; }
          |                         { $$ = new List<FnDecl*>; }
;

ProtoList :    ProtoList Prototype
                                    { ($$ = $1)->Append($2); }
          |    Prototype
                                    { ($$ = new List<FnDecl*>)->Append($1); }
;

Prototype :    Type T_Identifier '(' Formals ')' ';'
                                    { $$ = new FnDecl(new Identifier(@2, $2), $1, $4); }
          |    T_Void T_Identifier '(' Formals ')' ';'
                                    { $$ = new FnDecl(new Identifier(@2, $2), Type::voidType, $4); }
;

StmtBlock :    '{' VarDecls Stmts '}' 
                                    { $$ = new StmtBlock($2, $3); }
;

VarDecls  :    VarDecls VarDecl     { ($$=$1)->Append($2); }
          |    /* empty*/           { $$ = new List<VarDecl*>; }
;

/* It's possible to combine Stmts and StmtList, but that causes a conflict */
Stmts     :    StmtList             { $$ = $1; }
          |                         { $$ = new List<Stmt*>; }
;

StmtList  :    StmtList Stmt        { ($$ = $1)->Append($2); }
          |    Stmt                 { ($$ = new List<Stmt*>)->Append($1); }
;

Stmt      :    ';'                  { $$ = new EmptyExpr; }
          |    Expr ';'             { $$ = $1; }
          |    IfStmt               { $$ = $1; }
          |    WhileStmt            { $$ = $1; }
          |    ForStmt              { $$ = $1; }
          |    BreakStmt            { $$ = $1; }
          |    RetStmt              { $$ = $1; }
          |    PrintStmt            { $$ = $1; }
          |    StmtBlock            { $$ = $1; }
/*          |    SwitchStmt           { $$ = $1; }*/
;

IfStmt    :    T_If '(' Expr ')' Stmt
                                    { $$ = new IfStmt($3, $5, NULL); }
          |    T_If '(' Expr ')' Stmt T_Else Stmt
                                    { $$ = new IfStmt($3, $5, $7); }
;

WhileStmt :    T_While '(' Expr ')' Stmt
                                    { $$ = new WhileStmt($3, $5); }
;

ForStmt   :    T_For '(' ';' Expr ';' ')' Stmt
                                    { $$ = new ForStmt(new EmptyExpr, $4, new EmptyExpr, $7); }
          |    T_For '(' Expr ';' Expr ';' ')' Stmt
                                    { $$ = new ForStmt($3, $5, new EmptyExpr, $8); }
          |    T_For '(' ';' Expr ';' Expr ')' Stmt
                                    { $$ = new ForStmt(new EmptyExpr, $4, $6, $8); }
          |    T_For '(' Expr ';' Expr ';' Expr ')' Stmt
                                    { $$ = new ForStmt($3, $5, $7, $9); }
;

BreakStmt :    T_Break ';'          { $$ = new BreakStmt(@1); }
;

RetStmt   :    T_Return ';'         { $$ = new ReturnStmt(@1, new EmptyExpr); }
          |    T_Return Expr ';'    { $$ = new ReturnStmt(@2, $2); }
;

PrintStmt :    T_Print '(' ExprList ')' ';'
                                    { $$ = new PrintStmt($3); }
;

/*

SwitchStmt:    T_Switch '(' Expr ')' '{' CaseList '}'
                                    { $$ = new SwitchStmt($3, $6, NULL); }
          |    T_Switch '(' Expr ')' '{' CaseList DefCase '}'
                                    { $$ = new SwitchStmt($3, $6, $7); }
;

CaseList  :    CaseList Case        { ($$ = $1)->Append($2); }
          |    Case                 { ($$ = new List<Case*>)->Append($1); }
;

Case      :    T_Case T_IntConstant ':' Stmts
                                    { $$ = new Case(new IntConstant(@2, $2), $4); }
;

DefCase   :    T_Default ':' Stmts
                                    { $$ = new Default($3); }
;

*/

ExprList  :    ExprList ',' Expr    { ($$ = $1)->Append($3); }
          |    Expr                 { ($$ = new List<Expr*>)->Append($1); }
;

Expr      :    LValue '=' Expr      { $$ = new AssignExpr($1, new Operator(@2, "="), $3); }
          |    Constant             { $$ = $1; }
          |    LValue               { $$ = $1; }
          |    T_This               { $$ = new This(@1); }
          |    Call                 { $$ = $1; }
          |    '(' Expr ')'         { $$ = $2; }
          |    Expr '+' Expr        { $$ = new ArithmeticExpr($1, new Operator(@2, "+"), $3); }
          |    Expr '-' Expr        { $$ = new ArithmeticExpr($1, new Operator(@2, "-"), $3); }
          |    Expr '*' Expr        { $$ = new ArithmeticExpr($1, new Operator(@2, "*"), $3); }
          |    Expr '/' Expr        { $$ = new ArithmeticExpr($1, new Operator(@2, "/"), $3); }
          |    Expr '%' Expr        { $$ = new ArithmeticExpr($1, new Operator(@2, "%"), $3); }
          |    '-' Expr             { $$ = new ArithmeticExpr(new Operator(@1, "-"), $2); }
          |    Expr '<' Expr        { $$ = new RelationalExpr($1, new Operator(@2, "<"), $3); }
          |    Expr T_LessEqual Expr
                                    { $$ = new RelationalExpr($1, new Operator(@2, "<="), $3); }
          |    Expr '>' Expr        { $$ = new RelationalExpr($1, new Operator(@2, ">"), $3); }
          |    Expr T_GreaterEqual Expr
                                    { $$ = new RelationalExpr($1, new Operator(@2, ">="), $3); }
          |    Expr T_Equal Expr    { $$ = new EqualityExpr($1, new Operator(@2, "=="), $3); }
          |    Expr T_NotEqual Expr { $$ = new EqualityExpr($1, new Operator(@2, "!="), $3); }
          |    Expr T_And Expr      { $$ = new LogicalExpr($1, new Operator(@2, "&&"), $3); }
          |    Expr T_Or Expr       { $$ = new LogicalExpr($1, new Operator(@2, "||"), $3); }
          |    '!' Expr             { $$ = new LogicalExpr(new Operator(@2, "!"), $2); }
          |    T_ReadInteger '(' ')'
                                    { $$ = new ReadIntegerExpr(Join(@1, @3)); }
          |    T_ReadLine '(' ')'   { $$ = new ReadLineExpr(Join(@1, @3)); }
          |    T_New '(' T_Identifier ')'
                                    { $$ = new NewExpr(Join(@1, @4), new NamedType(new Identifier(@3, $3))); }
          |    T_NewArray '(' Expr ',' Type ')'
                                    { $$ = new NewArrayExpr(Join(@1, @6), $3, $5); }
          /* Needs to be assignable */
          //|    LValue T_Inc         { $$ = new PostfixExpr($1, new Operator(@2, "++")); }
          //|    LValue T_Dec         { $$ = new PostfixExpr($1, new Operator(@2, "--")); }
;

LValue    :    T_Identifier         { $$ = new FieldAccess(NULL, new Identifier(@1, $1)); }
          |    Expr '.' T_Identifier
                                    { $$ = new FieldAccess($1, new Identifier(@3, $3)); }
          |    Expr '[' Expr ']'    { $$ = new ArrayAccess(Join(@1, @4), $1, $3); }
;

Call      :    T_Identifier '(' Actuals ')'
                                    { $$ = new Call(Join(@1, @4), NULL, new Identifier(@1, $1), $3); }
          |    Expr '.' T_Identifier '(' Actuals ')'
                                    { $$ = new Call(Join(@1, @6), $1, new Identifier(@3, $3), $5); }
;

Actuals   :    ExprList             { $$ = $1; }
          |                         { $$ = new List<Expr*>; }
;

Constant  :    T_IntConstant        { $$ = new IntConstant(@1, $1); }
          |    T_DoubleConstant     { $$ = new DoubleConstant(@1, $1); }
          |    T_BoolConstant       { $$ = new BoolConstant(@1, $1); }
          |    T_StringConstant     { $$ = new StringConstant(@1, $1); }
          |    T_Null               { $$ = new NullConstant(@1); }
;

%%


/* Function: InitParser
 * --------------------
 * This function will be called before any calls to yyparse().  It is designed
 * to give you an opportunity to do anything that must be done to initialize
 * the parser (set global variables, configure starting state, etc.). One
 * thing it already does for you is assign the value of the global variable
 * yydebug that controls whether yacc prints debugging information about
 * parser actions (shift/reduce) and contents of state stack during parser.
 * If set to false, no information is printed. Setting it to true will give
 * you a running trail that might be helpful when debugging your parser.
 * Please be sure the variable is set to false when submitting your final
 * version.
 */
void InitParser()
{
   PrintDebug("parser", "Initializing parser");
   yydebug = false;
}
