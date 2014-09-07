/*
 * file:  dpp.l
 * ------------
 * Lex inupt file to generate the scanner for the preprocessor.
 * If you decide to use lex for the preprocesor, put your rules
 * here, otherwise the file can remain empty.
 */

%{
#include "errors.h"
#include <map>

int lineno = 1;

std::map<const char *, const char *> macro_table;
char *define_name = NULL;

void MacroAdd(const char *name, const char *replacement) {
	
}

bool MacroExists(const char *name) {
	return macro_table.find(name) == macro_table.end();
}

const char *MacroGet(const char *name) {
	if (!MacroExists(name))
		return NULL;
	return macro_table[name];
}
%}

%s DEFINE_NAME DEFINE_REPLACE

UPPER	[A-Z]

%%

"\n"				{putchar('\n'); lineno++;}

"//"[^\n]*			{}

"/*"([^*]*"*")+"/"		{}
"/*".*				{ReportError::UntermComment();}

"#define "			{BEGIN(DEFINE_NAME);}

  /* <DEFINE_NAME>{UPPER}+		{define_name = malloc(yyleng + 1); strcpy(define_name, yytext); BEGIN(DEFINE_REPLACE);} */

  /* "#define "{UPPER}*" "[^\n]*	{} */
  /* "#define "[^ ]*" "[^\n]*	{ReportError::InvalidDirective(lineno);} */
"#"{UPPER}*			{
					const char *replace = MacroGet(yytext + 1);
					if (replace)
						puts(replace);
					else
						ReportError::InvalidDirective(lineno);
				}

.				{putchar(*yytext);}
