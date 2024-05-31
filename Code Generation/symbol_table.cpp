#include "symbol_table.h"
#include "hw3_output.hpp"
#include <iostream>

extern TableStack tables;

void SymbolTable::add_symbol(const Symbol &symbol) {
    symbols.push_back(new Symbol(symbol));
    if (symbol.offset >= 0)
        max_offset = symbol.offset;
}


bool SymbolTable::symbol_exists(const string &name) {
    for(int i = 0; i < symbols.size(); i++){
        if (symbols[i]->name == name)
            return true;
    }
    return false;
}


Symbol *SymbolTable::get_symbol(const string &name, string ret_type) {
    for (auto it = symbols.begin(); it != symbols.end(); ++it) {

        if (((*it)->name == name&& ret_type=="" )||((*it)->name == name&& ret_type==(*it)->type))
            return (*it);

    }
    return nullptr;
}

/*Symbol *SymbolTable::get_next_symbol(const string &name, vector<string> type_of_params, int i)
{
    for (auto it = symbols.begin(); it != symbols.end(); ++it) {
        if ((*it)->name == name)
            for (int i = 0; i < (*it)->symbol->params.size(); i++) {
            return (*it);
    }
}
}*/

//**************TABLESTACK*************************

TableStack::TableStack() : table_stack(), offsets() {
    offsets.push_back(0);
    push_scope(false);
    add_symbol("print", "void", true, false, {"string"});
    add_symbol("printi", "void", true, false, {"int"});
add_symbol("readi", "int", true, false, {"int"});
}

bool TableStack::symbol_exists(const string &name) {

    for (auto it = table_stack.rbegin(); it != table_stack.rend(); ++it) {
        SymbolTable *current = *it;
        if (current->symbol_exists(name))
            return true;
    }
    return false;
}

bool TableStack::check_override(string id, string return_type, vector<string> param_types, int &count){
    bool is_override = false;
    for (auto it = table_stack.rbegin(); it != table_stack.rend(); ++it) {
        SymbolTable *current = *it;
        for(int i = 0; i < current->symbols.size(); i++) {
            if (current->symbols[i]->name == id && current->symbols[i]->is_function){
                if (current->symbols[i]->params == param_types && current->symbols[i]->type == return_type)
                    return false;
                else{
                    count++;
                }
        }
        }
    }
    return true;
    }


bool TableStack::check_loop() {
    for (auto it = table_stack.rbegin(); it != table_stack.rend(); ++it) {
        SymbolTable *current = *it;
        if (current->is_loop)
            return true;
    }
    return false;
}

Symbol *TableStack::get_symbol(const string &name, string type) {
    for (auto it = table_stack.begin(); it != table_stack.end(); ++it) {
        Symbol *symbol = (*it)->get_symbol(name, type);
        if (symbol)
            return symbol;
    }
    return nullptr;
}

Symbol *TableStack::checkTheRightSymbol(const string &name, vector<string> type_of_params, int yylineno, int &count) {
    int j = 0;
    Symbol *symbol = nullptr;
    bool find = false;
    //loop on the scops
    for (auto it = table_stack.begin(); it != table_stack.end(); ++it) {
        if ((*it)->symbols.size()) {

            //loop on the variable in this scope
            for (auto it1 = (*it)->symbols.begin(); it1 != (*it)->symbols.end(); ++it1) {
                if ((*it1)->name == name) {
                    if(find)
                        break;
                    count++;
                    if (type_of_params.size() == (*it1)->params.size()) {
                        if (type_of_params.size() == 0) {
                            j++;
                            symbol = (*it1);
                            find= true;
                        }
                        //check the params are equal
                        for (int i = 0; i < type_of_params.size(); i++) {
                            if (((*it1)->params[i] == type_of_params[i]) ||
                                ((*it1)->params[i] == "int" && type_of_params[i] == "byte")) {
                                if (i == (type_of_params.size() - 1)) {
                                    symbol = (*it1);
                                    j++;
                                    find= true;
                                }
                            } else
                                break;
                        }
                    }
                }

            }
        }
    }

    if (j >= 2) {
        output::errorAmbiguousCall(yylineno, name);
        exit(0);
    }
    if (j == 0) {
        output::errorPrototypeMismatch(yylineno, name);
        exit(0);
    }

    return symbol;
}


void TableStack::add_symbol(const string &name, const string &type, bool is_function, bool is_override, vector<string> params) {
    SymbolTable *current_scope = table_stack.back();
    int offset;
    if (is_function) {
        offset = 0;
    } else {
        offset = offsets.back();
        offsets.push_back(offset + 1);
    }

    Symbol symbol = Symbol(name, type, offset, params, is_function, is_override);
    current_scope->add_symbol(symbol);
}

void TableStack::add_function_symbol(const string &name, const string &type, int offset) {
    SymbolTable *current_scope = table_stack.back();
    Symbol symbol = Symbol(name, type, offset, vector<string>(), false);
    current_scope->add_symbol(symbol);
}

void TableStack::push_scope(bool is_loop, string return_type) {
    SymbolTable* new_scope = new SymbolTable(offsets.back(), is_loop, return_type);
    if(!table_stack.empty()){
        new_scope->rbp = table_stack.back()->rbp;
    } else {
        new_scope->rbp = "";
    }
    this->table_stack.push_back(new_scope);
    SymbolTable* current_scope = table_stack.back();
    offsets.push_back(current_scope->max_offset);
}

SymbolTable *TableStack::current_scope() {
    return table_stack.back();
}

string convert_to_upper_case(const string &str) {
    if (str == "bool")
        return "BOOL";
    else if (str == "byte")
        return "BYTE";
    else if (str == "int")
        return "INT";
    else if (str == "void")
        return "VOID";
    else
        return "STRING";
}

void TableStack::pop_scope() {

    SymbolTable *scope = table_stack.back();
    table_stack.pop_back();
    //output::endScope();
    /*for (auto it = scope->symbols.begin(); it != scope->symbols.end(); ++it) {
        auto offset = offsets.back();
        offsets.pop_back();
        if ((*it)->is_function) {
            vector<string> converted_params;
            for (int i = 0; i < (*it)->params.size(); ++i) {
                converted_params.push_back(convert_to_upper_case((*it)->params[i]));
            }
            output::printID((*it)->name, 0,
                            output::makeFunctionType(convert_to_upper_case((*it)->type), converted_params));
        } else {
            output::printID((*it)->name, (*it)->offset, convert_to_upper_case((*it)->type));
        }
    }*/
    if(offsets.size() > 0)
        offsets.pop_back();
    delete scope;
    //std::cout <<"pop!!!";

}


void TableStack::check_program() {
    SymbolTable *main_scope = tables.table_stack.front();
    if (main_scope->symbol_exists("main")) {
        Symbol *main_symbol = main_scope->get_symbol("main");
        if (main_symbol->type == "void") {
            if (main_symbol->params.size() == 0) {
                tables.pop_scope();
                return;
            }
        }
    }
    output::errorMainMissing();
    exit(0);
}