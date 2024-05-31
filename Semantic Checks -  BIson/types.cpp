#include "types.h"
#include "symbol_table.h"
#include "hw3_output.hpp"

#define TRUE "true"
#define FALSE "false"
extern TableStack tables;
extern int yylineno;

static bool check_types_compatible(string type1, string type2) {
    if (type1 == type2)
        return true;

    if (type1 == "bool" || type2 == "bool" || type1 == "string" || type2 == "string" || type2 == "void")
        return false;
    return true;
}

//************FORMALS******************
FormalDecl::FormalDecl(FormalDecl *formal) : Node(formal->value), type(formal->type) {
}

FormalDecl::FormalDecl(Type *type, Node *node) : Node(*node), type(type->type) {
}

FormalsList::FormalsList(Node *node) : Node(), formals_list() {
    formals_list.insert(formals_list.begin(), new FormalDecl(dynamic_cast<FormalDecl *>(node)));
}

FormalsList::FormalsList(Node *node, FormalsList *formals_list) : Node(), formals_list() {

    for (auto it = formals_list->formals_list.begin(); it != formals_list->formals_list.end(); ++it) {
        FormalDecl *decl = new FormalDecl(*it);
        this->formals_list.push_back(decl);
    }
    this->formals_list.insert(this->formals_list.begin(), dynamic_cast<FormalDecl *>(node));
}

Formals::Formals() : Node(), formals_list() {
}

Formals::Formals(FormalsList *formals_list) : Node(), formals_list() {

    for (auto it = formals_list->formals_list.begin(); it != formals_list->formals_list.end(); ++it) {
        FormalDecl *decl = new FormalDecl(*it);
        this->formals_list.push_back(decl);
    }
}

//************FUNCDECL*****************
// FuncDecl -> RetType ID LPAREN Formals RPAREN LBRACE OS Statements ES RBRACE
FuncDecl::FuncDecl(RetType *return_type, Node *id, Formals *params, Override *isOverride) {
    bool override= isOverride->is_override;

    vector<string> function_param_types = vector<string>();
    for (int i = 0; i < params->formals_list.size(); ++i) {
        auto it = params->formals_list[i];
        function_param_types.push_back((*it).type);
    }
	
	
		

    if (tables.symbol_exists(id->value)) {
        if(tables.get_symbol(id->value)->is_override) {
            
            if(!(tables.check_override(id->value, return_type->type, function_param_types))){
                output::errorDef(yylineno, id->value);
                exit(0);
            }
        }
        else if(!override){
            output::errorDef(yylineno, id->value);
            exit(0);
        }
        

    }

    tables.add_symbol(id->value, return_type->type, true, override, function_param_types);
    tables.push_scope(false, return_type->type);
    int offset = -1;
    for (int i = 0; i < params->formals_list.size(); ++i) {
        auto it = params->formals_list[i];
        if (tables.symbol_exists(it->value)) {
            output::errorDef(yylineno, it->value);
            exit(0);
        }

        tables.add_function_symbol(it->value, it->type, offset);
        offset -= 1;
    }
}

//************STATEMENT****************
// Statement -> BREAK SC / CONTINUE SC
Statement::Statement(Node *node) {
    if (node->value == "break") {
        if (!tables.check_loop()) {
            output::errorUnexpectedBreak(yylineno);
            exit(0);
        }
    } else if (node->value == "continue") {
        if (!tables.check_loop()) {
            output::errorUnexpectedContinue(yylineno);
            exit(0);
        }
    }

}

Statement::Statement(Exp *exp, bool is_return) : Node() {
    SymbolTable *scope = tables.current_scope();
    string *return_type = scope->return_type;

    if (*return_type != "" && *return_type != exp->type) {
        if (*return_type != "int" || exp->type != "byte") {
            output::errorMismatch(yylineno);
            exit(0);
        }

    }
    if (exp->is_var) {
        Symbol *symbol = tables.get_symbol(exp->value);
        if (symbol->is_function) {
            output::errorUndef(yylineno, symbol->name);
            exit(0);
        }
    }
}

// Statement -> Type ID SC
Statement::Statement(Type *type, Node *id) : Node() {

    if (tables.symbol_exists(id->value)) {
        output::errorDef(yylineno, id->value);
        exit(0);
    }
    tables.add_symbol(id->value, type->type, false);
    value = type->value;//

}

// Statement -> Type ID ASSIGN Exp SC or Statement -> AUTO ID ASSIGN Exp SC
Statement::Statement(Type *type, Node *id, Exp *exp) : Node() {


    if (tables.symbol_exists(id->value)) {
        output::errorDef(yylineno, id->value);
        exit(0);
    }

    if (type) {
        if (!check_types_compatible(type->type, exp->type)) {
            output::errorMismatch(yylineno);
            exit(0);
        }
        if (type->type == "byte" && exp->type == "int") {
            output::errorMismatch(yylineno);
            exit(0);
        }
        tables.add_symbol(id->value, type->type, false);
    } else {
        if (exp->type == "void" || exp->type == "string") {
            output::errorMismatch(yylineno);
            exit(0);
        }
        tables.add_symbol(id->value, exp->type, false);
    }
}

// Statement -> ID ASSIGN Exp SC
Statement::Statement(Node *id, Exp *exp) : Node() {

    if (!tables.get_symbol(id->value)) {
        output::errorUndef(yylineno, id->value);
        exit(0);
    }
    if(tables.get_symbol(id->value)->is_function)
    {
            output::errorUndef(yylineno, id->value);
            exit(0);
    }

    Symbol *symbol = tables.get_symbol(id->value);
    if (symbol->is_function || !check_types_compatible(symbol->type, exp->type)) {
        output::errorMismatch(yylineno);
        exit(0);
    }
    if(symbol->type == "byte" && exp->type == "int"){
        output::errorMismatch(yylineno);
        exit(0);
    }
}

// Statement -> Call SC
Statement::Statement(Call *call) : Node() {

    if (!tables.symbol_exists(call->value)) {
        output::errorUndefFunc(yylineno, call->value);
        exit(0);
    }
    Symbol *symbol = tables.get_symbol(call->value);
    if (!symbol->is_function) {
        output::errorUndefFunc(yylineno, call->value);
        exit(0);
    }
}

Statement::Statement(const string name, Exp *exp) {


    if (exp->type != "bool") {
        output::errorMismatch(yylineno);
        exit(0);
    }
}

//****************TYPE************************
Type::Type(
        const string type) : Node(), type(type) {

}

RetType::RetType(
        const string type) : Node(), type(type) {}

// ***************EXP******************
// Exp -> LPAREN Exp RPAREN
Exp::Exp(Exp *exp) : Node(exp->value), type(exp->type) {
}

// Exp -> CONST(bool, num, byte, string)
Exp::Exp(Node *terminal, string
type) : Node(terminal->value), type(type) {
    if (type == "byte") {
        int value = stoi(terminal->value);
        if (value > 255) {
            output::errorByteTooLarge(yylineno, terminal->value);
            exit(0);
        }
    }
}

//Exp -> ID, Call
Exp::Exp(bool is_var, Node *terminal) : Node(), is_var(is_var) {

    if (is_var && !tables.symbol_exists(terminal->value)) {
        output::errorUndef(yylineno, terminal->value);
        exit(0);
    }
     Call *call = dynamic_cast<Call *>(terminal);
    if(is_var && tables.get_symbol(terminal->value)->is_function){
        output::errorUndef(yylineno, terminal->value);
        exit(0);
    }
    Symbol *symbol;
    if(!is_var)
    {
        symbol = tables.get_symbol(call->value, call->type);
    } else{
        symbol = tables.get_symbol(terminal->value);
    }
    value = terminal->value;
    type = symbol->type;
}

Exp::Exp(Node *terminal1, Node *terminal2,
         const string op,
         const string type) {
    Exp *exp1 = dynamic_cast<Exp *>(terminal1);
    Exp *exp2 = dynamic_cast<Exp *>(terminal2);

    if (exp1->is_var && !tables.symbol_exists(exp1->value)) {
        output::errorUndef(yylineno, terminal1->value);
        exit(0);
    }

    if (exp2->is_var && !tables.symbol_exists(exp2->value)) {
        output::errorUndef(yylineno, terminal2->value);
        exit(0);
    }

    if (!check_types_compatible(exp1->type, exp2->type)) {
        output::errorMismatch(yylineno);
        exit(0);
    }

    if (type == "bool") {
        if (exp1->type != "bool" || exp2->type != "bool") {
            output::errorMismatch(yylineno);
            exit(0);
        }

        this->type = "bool";
    } else if (type == "int") {

        if ((exp1->type != "int" && exp1->type != "byte") || (exp1->type != "int" && exp1->type != "byte")) {
            output::errorMismatch(yylineno);
            exit(0);
        }

        if (exp1->type == "int" || exp2->type == "int") {
            this->type = "int";
        } else {
            this->type = "byte";
        }

    } else if (type == "relop") {
        if (!check_types_compatible(exp1->type, exp2->type)) {
            output::errorMismatch(yylineno);
            exit(0);
        }
        if ((exp1->type != "int" && exp1->type != "byte") || (exp1->type != "int" && exp1->type != "byte")) {
            output::errorMismatch(yylineno);
            exit(0);
        }
        this->type = "bool";
    }
}

// Exp -> LPAREN Type RPAREN Exp
Exp::Exp(Node *exp, Node *type) {
    Exp *converted_exp = dynamic_cast<Exp *>(exp);
    Type *converted_type = dynamic_cast<Type *>(type);

    if (!check_types_compatible(converted_exp->type, converted_type->type)) {
        output::errorMismatch(yylineno);
        exit(0);
    }

    this->value = converted_exp->value;
    this->type = converted_type->type;
}

//*******************EXPLIST************************

// ExpList -> Exp
ExpList::ExpList(Node *exp) : Node(), expressions() {

    Exp *expression = dynamic_cast<Exp *>(exp);
    expressions.push_back(expression);
}

// ExpList -> Exp, ExpList
ExpList::ExpList(Node *exp_list, Node *exp) : Node(), expressions() {

    expressions.push_back(dynamic_cast<Exp *>(exp));
    vector < Exp * > expressions_list = (dynamic_cast<ExpList *>(exp_list))->expressions;
    for (int i = 0; i < expressions_list.size(); ++i) {
        expressions.push_back(new Exp(expressions_list[i]));

    }
}

// Call -> ID LPAREN ExpList RPAREN
Call::Call(Node *terminal, Node *exp_list) : Node() {

    ExpList *expressions_list;
    bool noParam = true;
    if(exp_list) {
        expressions_list = dynamic_cast<ExpList *>(exp_list);
        noParam =false;
    }
    string name = terminal->value;



    if (!tables.symbol_exists(name)) {
        output::errorUndefFunc(yylineno, name);
        exit(0);
    }

    Symbol *symbol = tables.get_symbol(name);

    if (!symbol->is_function) {
        output::errorUndefFunc(yylineno, name);
        exit(0);
    }


    vector<string> type_of_params = vector<string>();
    if(!noParam)
        for (int i = 0; i < expressions_list->expressions.size(); ++i) {
            type_of_params.push_back(expressions_list->expressions[i]->type);
        }

    symbol = tables.checkTheRightSymbol(name, type_of_params, yylineno);

    type = symbol->type;
    value = symbol->name;
}



Program::Program() {

}

void check_bool(Node *node) {
    Exp *exp = dynamic_cast<Exp *>(node);
    if (exp->type != "bool") {
        output::errorMismatch(yylineno);
        exit(0);
    }
}

