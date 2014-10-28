/* File: ast.cc
 * ------------
 */

#include "ast.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h> // strdup
#include <stdio.h>  // printf

Node::Node(yyltype loc) {
    location = new yyltype(loc);
    parent = NULL;
}

Node::Node() {
    location = NULL;
    parent = NULL;
}

Symbol *Node::LookupSymbol(const char *n) {
    Node *node = this;

    while (1) {
        Hashtable<Symbol *> *sym_tab = node->GetSymTab();
        Symbol *s = sym_tab->Lookup(n);
        if (s)
            return s;

        /* Find the parent symbol table */
        while (node->GetSymTab() == sym_tab) {
            if (node->GetParent() == NULL)
                return NULL;

            node = node->GetParent();
        }
    }
}
	 
Identifier::Identifier(yyltype loc, const char *n) : Node(loc) {
    name = strdup(n);
} 

