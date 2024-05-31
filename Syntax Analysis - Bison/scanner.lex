%{
    #include "output.hpp"
    #include "parser.tab.hpp"
%}

%option yylineno
%option noyywrap
digit              ([0-9])
letter           ([a-zA-Z])
letterdig        ([a-zA-Z0-9])
whitespace       ([\t\n\r ])

%%
void                                                                                return VOID;
int                                                                                 return INT;
byte                                                                                return BYTE;
b                                                                                   return B;
bool                                                                                return BOOL;
override                                                                            return OVERRIDE;
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
==|!= 																				return RELOP1;
>=|<|>|<=                                                                   return RELOP;
\-|\+                                                                               return SUB_ADD;
\*|\/                                                                               return DIV_MUL;
{letter}{letterdig}*                                                                return ID;
0|([1-9]+{digit}*)                                                                  return NUM;
\"([^\n\r\"\\]|\\[rnt"\\])+\"                                                       return STRING;
\/\/[^\n\r]*[\r|\n|\r\n]?                                                           ;
{whitespace}                                                                        ;
.                                                                                   {output::errorLex(yylineno); exit(0);}
