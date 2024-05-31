#include "types.h"
#include "symbol_table.h"
#include "hw3_output.hpp"
#include "handler.h"

#define TRUE "true"
#define FALSE "false"

#define GLOBALB "true"
#define LOCALB "false"

extern TableStack tables;
extern int yylineno;
extern CodeBuffer buffer;
extern Handler code_handle;


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
	
	if(id->value == "main" && override){
		output::errorMainOverride(yylineno);
		exit(0);
	}
    int count =0;
    if (tables.symbol_exists(id->value)) {
        if(tables.get_symbol(id->value)->is_override) {
            if(!override){
                output::errorOverrideWithoutDeclaration(yylineno, id->value);
                exit(0);
            }
            if(!(tables.check_override(id->value, return_type->type, function_param_types, count))){
                output::errorDef(yylineno, id->value);
                exit(0);
            }
        }
        else if(!override){
            output::errorDef(yylineno, id->value);
            exit(0);
        }
        else{
            output::errorFuncNoOverride(yylineno, id->value);
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
    if (count) {
        id->value += "unique" + std::to_string(count);
        //std::cout <<id->value;
    }
    code_handle.function_declaration_code(id, params, return_type);
    tables.current_scope()->rbp = code_handle.allocate_function_stack();
}
//************STATEMENTS****************
Statements::Statements(Statement *statement) {
//    std::cout << "Statements1 statement sizes b " << statement->break_list.size() << " c " << statement->cont_list.size() << "\n";
    break_list = buffer.merge(break_list, statement->break_list);
    cont_list = buffer.merge(cont_list, statement->cont_list);
//    std::cout << "Statements1 b " << break_list.size() << " c " << cont_list.size() << "\n" ;
    delete statement;
}

Statements::Statements(Statements *statements, Statement *statement) : Node(), cont_list(), break_list() {
//    std::cout << "Statements2 statement sizes b " << statement->break_list.size() << " c " << statement->cont_list.size() << "\n";
//    std::cout << "Statements2 statement sizes b " << statements->break_list.size() << " c " << statements->cont_list.size() << "\n";
    break_list = buffer.merge(statements->break_list, statement->break_list);
    cont_list = buffer.merge(statements->cont_list, statement->cont_list);
//    std::cout << "Statements2 b " << break_list.size() << " c " << cont_list.size() << "\n" ;
    delete statements;
    delete statement;
}

//************STATEMENT****************
// Statement -> BREAK SC / CONTINUE SC
Statement::Statement(Node *node) {
    int address = buffer.emit("br label @");
    //std::cout<<node->value;
    if (node->value == "break") {
        if (!tables.check_loop()) {
            output::errorUnexpectedBreak(yylineno);
            exit(0);
        }

        this->break_list = buffer.makelist(pair<int, BranchLabelIndex>(address, FIRST));
       // std::cout<<break_list.size();
    } else if (node->value == "continue") {
        if (!tables.check_loop()) {
            output::errorUnexpectedContinue(yylineno);
            exit(0);
        }
        this->cont_list = buffer.makelist(pair<int, BranchLabelIndex>(address, FIRST));
    }

}
//RETURN Exp SC
Statement::Statement(Exp *exp, bool is_return) : Node() {
    SymbolTable *scope = tables.current_scope();
    string *return_type = scope->return_type;

    
    if (exp->is_var) {
        Symbol *symbol = tables.get_symbol(exp->value);
        if (symbol->is_function) {
            output::errorUndef(yylineno, symbol->name);
            exit(0);
        }
    }

    string rbp = tables.current_scope()->rbp;
    string reg;
    if (exp->is_var) {
        Symbol *symbol = tables.get_symbol(exp->value);
        if (symbol->is_function) {
            output::errorUndef(yylineno, symbol->name);
            exit(0);
        }
        if (symbol->offset >= 0) {
            //this reg contains the symbol value
            reg = code_handle.handle_load_var(rbp, symbol->offset);
        } else {
            reg = "%" + std::to_string(((-1) * symbol->offset -1));
        }
        if(exp->type == "bool"){
            exp->reg = reg;

            Exp* new_exp = code_handle.bool_exp(exp);
            code_handle.return_value(new_exp->type, new_exp->reg);
            delete new_exp;
        } else {
            code_handle.return_code(*return_type, reg);
        }
    } else {
        if (exp->type == "void") {
            string empty = "";
            code_handle.return_code(empty, reg);
        } else if (exp->type == "bool") {
            Exp *new_exp = code_handle.bool_exp(exp);
            code_handle.return_value(new_exp->type, new_exp->reg);
            delete new_exp;
        } else {
            if (!exp->value.empty()) {
                Symbol *symbol = tables.get_symbol(exp->value);
                if (symbol) {
                    // Function call
                    code_handle.return_value(exp->type, exp->reg);
                } else {
                    code_handle.return_value(exp->type, exp->value);
                }
            } else {
                code_handle.return_value(exp->type, exp->reg);
            }
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
    value = type->value;
    Symbol* symbol = tables.get_symbol(id->value);
    value = type->value;
    Exp *temp = new Exp();
    temp->reg = code_handle.registerAllocate(false);
    if (value == "bool") {
        temp->value = "false";
        code_handle.ruleBool(temp);
    } else {
        temp->value = "0";
        code_handle.ruleNum(temp);
        code_handle.assign_code(temp, symbol->offset, symbol->type == "bool");
    }
    delete temp;

}

// Statement -> Type ID ASSIGN Exp SC or Statement -> AUTO ID ASSIGN Exp SC
Statement::Statement(Type *type, Node *id, Exp *exp) : Node() {


    if (tables.symbol_exists(id->value)) {
        
        output::errorDef(yylineno, id->value);
        exit(0);
    }

    string var_type;
    if (type) {
        if (!check_types_compatible(type->type, exp->type)) {
            output::errorMismatch(yylineno);
            exit(0);
        }
        if (type->type == "byte" && exp->type == "int") {
            output::errorMismatch(yylineno);
            exit(0);
        }
        var_type = type->type;
        tables.add_symbol(id->value, type->type, false);
    } else {
        if (exp->type == "void" || exp->type == "string") {
            output::errorMismatch(yylineno);
            exit(0);
        }
        tables.add_symbol(id->value, exp->type, false);
        var_type = exp->type;
    }
    Symbol *symbol = tables.get_symbol(id->value);
    code_handle.assign_code(exp, symbol->offset, var_type == "bool");
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
    if(symbol->offset >= 0 ){

        code_handle.assign_code(exp, symbol->offset, symbol->type == "bool");
    }
}

// Statement -> Call SC
Statement::Statement(Call *call) : Node() {
        //std::cout << "311"<<std::endl;

    if (!tables.symbol_exists(call->value)) {
        //std::cout << "314"<<std::endl;
        output::errorUndefFunc(yylineno, call->value);
        exit(0);
    }
        //std::cout << "317"<<std::endl;
    Symbol *symbol = tables.get_symbol(call->value);
    if (!symbol->is_function) {
        output::errorUndefFunc(yylineno, call->value);
        exit(0);
    }
    Exp exp = Exp();
    exp.type = call->type;
    exp.true_list = call->true_list;
    exp.false_list = call->false_list;
    code_handle.bool_exp(&exp);
        //std::cout << "320"<<std::endl;
//buffer.printCodeBuffer();
}


//IF LPAREN CheckBool RPAREN OS Label Statement
Statement::Statement(Statement* statement, Exp *exp, Label *label) {
    if (exp->type != "bool") {
        output::errorMismatch(yylineno);
        exit(0);
    }
    break_list = buffer.merge(break_list, statement->break_list);
    cont_list = buffer.merge(cont_list, statement->cont_list);

    buffer.bpatch(exp->true_list, label->value);
    string new_label = buffer.genLabel();
    //std::cout<<label->value<<"   317\n";
   //std::cout<<new_label<<"   318\n";
    code_handle.label_block_code(new_label);
    buffer.bpatch(exp->false_list, new_label);
    buffer.bpatch(exp->next_list, new_label);
}

//IF LPAREN CheckBool RPAREN OS Label Statement  ELSE OS Label Statement
Statement::Statement(Statement* statement1, Statement* statement2, Exp *exp, Label *true_label, Label *false_label) {
    if (exp->type != "bool") {
        output::errorMismatch(yylineno);
        exit(0);
    }
    cont_list =  buffer.merge(statement1->cont_list, statement2->cont_list);
    break_list = buffer.merge(statement1->break_list, statement2->break_list);

    buffer.bpatch(exp->true_list, true_label->value);
    buffer.bpatch(exp->false_list, false_label->value);
    string new_label = buffer.genLabel();
   // std::cout<<new_label<<"335\n";
    code_handle.label_block_code(new_label);
    buffer.bpatch(exp->next_list, new_label);
}

//WHILE LPAREN Label CheckBool RPAREN Label Statement
Statement::Statement(const string& name, Exp *exp, Label *exp_label, Label *true_label, Statement *statement) {
    buffer.emit_uncond_jump(exp_label->value);
    string new_label = code_handle.labelAllocate("label_");
   // std::cout<<new_label<<"344\n";
    code_handle.label_block_code(new_label);
    buffer.bpatch(exp->true_list, true_label->value);
    buffer.bpatch(exp->false_list, new_label);
    buffer.bpatch(exp->next_list, new_label);
    buffer.bpatch(statement->break_list, new_label);
    //std::cout<<statement->break_list.size();
    buffer.bpatch(statement->cont_list, exp_label->value);
}

//****************TYPE************************
Type::Type(
        const string type) : Node(), type(type) {

}

RetType::RetType(
        const string type) : Node(), type(type) {}

// ***************EXP******************
// Exp -> LPAREN Exp RPAREN
Exp::Exp(Exp *exp) : Node(exp->value), type(exp->type), reg(exp->reg), false_list(exp->false_list),
                     true_list(exp->true_list) , next_list(exp->next_list) {
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
    if (type == "int" || type == "byte") {
        reg = code_handle.registerAllocate(false);
        code_handle.ruleNum(this);
    } else if (type == "string") {
        code_handle.ruleStr(this);
    } else {
        code_handle.ruleBool(this);
    }
}

//Exp -> ID, Call
Exp::Exp(bool is_var, Node *terminal) : Node(), is_var(is_var) {

    if (is_var && !tables.symbol_exists(terminal->value)) {
        output::errorUndef(yylineno, terminal->value);
        exit(0);
    }
    //Symbol *symbol;
    Symbol *symbol = tables.get_symbol(terminal->value);
    value = terminal->value;
    type = symbol->type;
    this->is_var = is_var;
    if (!is_var) {
        Call* func = dynamic_cast<Call *>(terminal);
        reg = func->reg;
        true_list = func->true_list;
        false_list = func->false_list;
        return;
    }
    code_handle.ruleID(this);
}

Exp::Exp(Node *terminal1, Node *terminal2,
         const string op,
         const string type,
         const string label = "") {
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
        code_handle.bool_eval_code(this, dynamic_cast<Exp *>(exp1), dynamic_cast<Exp *>(exp2), op, label);
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
        this->value = "";
        code_handle.binop_code(this, exp1, exp2, op);

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
        code_handle.relop_code(this, exp1, exp2, op);
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
    this->reg = converted_exp->reg;

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
    int count =0;
    symbol = tables.checkTheRightSymbol(name, type_of_params, yylineno, count);

    type = symbol->type;
    string tempValue = symbol->name;
    if(count>1) {
        value = symbol->name + "unique" + std::to_string(count - 1);
    }
    else
        value = symbol->name;
    if(noParam) {
        ExpList empty_exp = ExpList();
        code_handle.function_code(this, &empty_exp);
    }
    else{
        code_handle.function_code(this, expressions_list);
    }
    value=tempValue;
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

Label::Label() : Node("") {
    value = code_handle.labelAllocate("label_");

    code_handle.label_block_code(value);
}

void Statement::merge_lists_statements(Node *node) {
    Statements *s = dynamic_cast<Statements *>(node);
    break_list = buffer.merge(break_list, s->break_list);
    cont_list = buffer.merge(cont_list, s->cont_list);
}