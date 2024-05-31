%{
/* Declaration Section */
#include <stdio.h>
#include "tokens.hpp"
%}

%option yylineno
%option noyywrap
%x STR
digit            ([0-9])
letter           ([a-zA-Z])
letterdigit      ([a-zA-Z0-9])
string           ([ !#-\[\]-~	])
escape           ([\\ntr\"0])
escape_not_n ([\\rt\"0])
hex_digit_top(x7[0-9a-eA-E])
hex_digit	 (x[2-6][0-9A-Fa-f])
whitespace       ([\t\n\r ])
special_hex (x(0a|09|0D|0A|0d))
point (..|.)


%%
void                                                                                return VOID;
int                                                                                 return INT;
byte                                                                                return BYTE;
b                                                                                   return B;
bool                                                                                return BOOL;
and                                                                                 return AND;
or                                                                                  return OR;
not                                                                                 return NOT;
true                                                                                return TRUE;
false                                                                               return FALSE;
return                                                                              return RETURN;
if                                                                                  return IF;
else                                                                                return ELSE;
while                                                                               return WHILE;
break                                                                               return BREAK;
continue                                                                            return CONTINUE;
;                                                                                   return SC;
,                                                                                   return COMMA;
\(                                                                                  return LPAREN;
\)                                                                                  return RPAREN;
\{                                                                                  return LBRACE;
\}                                                                                  return RBRACE;
=                                                                                   return ASSIGN;
[<>=!]=|>|<                                                                         return RELOP;
[-+*/]                                                                              return BINOP;
\/\/[^\n\r]*                                                                        return COMMENT;
{letter}{letterdigit}*                                                              return ID;
([1-9]+{digit}*)|0                                                                  return NUM;
\"({string}|\\{escape}|\\({hex_digit_top}|{hex_digit})|\\{special_hex})*\"          return STRING;
\"({string}|\\{escape}|\\({hex_digit_top}|{hex_digit})|\\{special_hex})*(\\n|\\0)    		return UNCLOSED_STRING;
\"({string}|\\{escape}|\\({hex_digit_top}|{hex_digit}|{special_hex}))*\\[^x\\ntr\"0]                                         return INVALID_ESCAPE_SEQUENCE;
\"({string}|\\{escape}|\\({hex_digit_top}|{hex_digit}|{special_hex}))*\\x([^0-7][0-9A-Fa-f]|7f|[01].|[01]|[0-7][^0-9A-Fa-f]|[^0-7][^0-9A-Fa-f]|[^0-9A-Fa-f].) return INVALID_HEX;
{whitespace}                                                                        ;
.                                                                                   return ERROR;
