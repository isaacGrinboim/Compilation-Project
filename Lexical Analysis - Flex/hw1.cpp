#include <iostream>
#include <sstream>
#include "tokens.hpp"

using std::cout;
using std::endl;

void handleToken(int token);
void printString();
void printInvalidEscapeSequence();
void printInvalidHexSequence();

int main() {
    int token;
    while ((token = yylex())) {
        handleToken(token);
    }
    return 0;
}

void printToken(std::string name){
    cout << yylineno << " " << name << " " << yytext << endl;
}

void handleToken(int token) {
    switch (token) {
        case VOID:
            printToken("VOID");
            break;
        case INT:
            printToken("INT");
            break;
        case BYTE:
            printToken("BYTE");
            break;
        case B:
            printToken("B");
            break;
        case BOOL:
            printToken("BOOL");
            break;
        case AND:
            printToken("AND");
            break;
        case OR:
            printToken("OR");
            break;
        case NOT:
            printToken("NOT");
            break;
        case TRUE:
            printToken("TRUE");
            break;
        case FALSE:
            printToken("FALSE");
            break;
        case RETURN:
            printToken("RETURN");
            break;
        case IF:
            printToken("IF");
            break;
        case ELSE:
            printToken("ELSE");
            break;
        case WHILE:
            printToken("WHILE");
            break;
        case BREAK:
            printToken("BREAK");
            break;
        case CONTINUE:
            printToken("CONTINUE");
            break;
        case SC:
            printToken("SC");
            break;
        case COMMA:
            printToken("COMMA");
            break;
        case LPAREN:
            printToken("LPAREN");
            break;
        case RPAREN:
            printToken("RPAREN");
            break;
        case LBRACE:
            printToken("LBRACE");
            break;
        case RBRACE:
            printToken("RBRACE");
            break;
        case ASSIGN:
            printToken("ASSIGN");
            break;
        case RELOP:
            printToken("RELOP");
            break;
        case COMMENT:
            cout << yylineno << " COMMENT //" << endl;
            break;
        case BINOP:
            printToken("BINOP");
            break;
        case ID:
            printToken("ID");
            break;
        case NUM:
            printToken("NUM");
            break;
        case STRING:
            printString();
            break;
        case UNCLOSED_STRING:
            cout<< "Error unclosed string" << endl;
   
            exit(0);
            break;
        case INVALID_ESCAPE_SEQUENCE:
            printInvalidEscapeSequence();
            
            break;
        case INVALID_HEX:
            printInvalidHexSequence();
            exit(0);
        default:
        if(yytext[0] != '"'){
			cout << "Error " << yytext[0] << endl;
			exit(0);
		}
        do {
		if(yytext[0] < ' ' || yytext[0] > '~'){ //illegal
			cout << "Error " << yytext[0] << endl;
			exit(0);
		}
    } while ((token = yylex()));
        
    }
}



void printInvalidHexSequence(){
    std::string str(yytext);
    int size = str.size();
    cout << "Error undefined escape sequence ";
    if(str[size-2] == 'x'){
        cout << str[size - 2] << endl;
        return;
    }
    cout << "x" << str[size - 2];
    if(str[size - 1] != '"')
        cout << str[size-1];
    cout << endl;
}

void printInvalidEscapeSequence(){
    std::string str(yytext);
    char ch = str[str.size() - 1];
    cout << "Error undefined escape sequence " << ch << endl;
    exit(0);
}

void printString(){
    std::string str(yytext);
    cout << yylineno << " STRING " ;
    for(int i = 0; i < str.size(); i++){
        char ch = str[i];
        char next_ch = str[i + 1];
        if(ch == '"') continue;
        else if(ch == '\\' && next_ch == 'x'){
            std::stringstream ss;
            ss << std::hex << str.substr(i+2, 2);
            int x;
            ss >> x;
            cout << char(x);
            i += 3;
        } else if(ch == '\\' && next_ch == 'n'){
            cout << '\n';
            i++;
        } else if(ch == '\\' && next_ch == 'r') {
            cout << '\r';
            i++;
        } else if(ch == '\\' && next_ch == 't'){
            cout << '\t';
            i++;
        } else if(ch == '\\' && next_ch == '0'){
            cout<<endl;
			return;
        }
        
        else if(ch == '\\' && next_ch == '\"'){
            cout << '\"';
            i++;
        } else if(ch == '\\'){
            cout << '\\';
            i++;
        } else {
            cout << ch;
        }
    }
    cout << endl;
}

