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

std::map<std::string, std::string> macro_table;
std::string define_name;

void MacroAdd(const std::string &name, const std::string &replacement) {
	macro_table[name] = replacement;
}

bool MacroExists(const std::string &name) {
	return macro_table.find(name) != macro_table.end();
}

std::string &MacroGet(const std::string &name) {
	return macro_table[name];
}
%}

%x DEFINE_NAME DEFINE_REPLACE BLOCK_COMMENT ERROR

UPPER	[A-Z]

%%

  /* Always copy the newlines so that the scanner knows the correct line number */
<INITIAL,BLOCK_COMMENT>"\n"	{putchar('\n'); lineno++;}

  /* Error occurred, eat rest of line */
<ERROR>[^\n]*"\n"		{BEGIN(INITIAL); putchar('\n'); lineno++;}

  /* Don't touch strings */
"\""[^\n"]*["]?			{fputs(yytext, stdout);}

"//"[^\n]*			{}

"/*"				{BEGIN(BLOCK_COMMENT);}
  /* Can't do .* as it matches as much as possible */
<BLOCK_COMMENT>"*/"		{BEGIN(INITIAL);}
<BLOCK_COMMENT>.		{/* Eat comment */}
<BLOCK_COMMENT><<EOF>>		{ReportError::UntermComment(); BEGIN(INITIAL);}

<DEFINE_NAME>" "{UPPER}+	{define_name = yytext + 1; BEGIN(DEFINE_REPLACE);}
<DEFINE_NAME>[^\n]		{ReportError::InvalidDirective(lineno); BEGIN(ERROR);}
<DEFINE_REPLACE>" "[^\n]*	{MacroAdd(define_name, yytext + 1); BEGIN(INITIAL);}
<DEFINE_REPLACE>[^\n]*		{ReportError::InvalidDirective(lineno); BEGIN(INITIAL);}
  /* Handle '#define' and '#define NAME' */
<DEFINE_NAME,DEFINE_REPLACE>"\n" {ReportError::InvalidDirective(lineno); BEGIN(INITIAL); putchar('\n'); lineno++;}

"#define"			{BEGIN(DEFINE_NAME);}

"#"{UPPER}*			{
					if (MacroExists(yytext + 1)) {
						std::string &tmp = MacroGet(yytext + 1);
						const char * replace = tmp.c_str();
						/* Puts adds a newline, but this doesn't */
						fputs(replace, stdout);
					} else {
						ReportError::InvalidDirective(lineno);
					}
				}

.				{putchar(*yytext);}
