#ifndef COMPI_HW3_SYMBOL_TABLE_H
#define COMPI_HW3_SYMBOL_TABLE_H

#include <string>
#include <vector>
#include <iostream>

#define DEBUG false

using std::string;
using std::vector;


string convert_to_upper_case(const string &str);

class Symbol {
public:
    string name;
    // If class is function, this is return type
    string type;
    int offset;
    bool is_function;
    bool is_override;
    // param types
    vector<string> params;

    Symbol(const string name, const string type, int offset, vector<string> params, bool is_function, bool is_override=false) :
    name(name),
    type(type),
    offset(offset),
    params(params),
    is_function(is_function),
    is_override(is_override)
    {}
    ~Symbol() = default;
};

class SymbolTable {

public:
    vector<Symbol*> symbols;
    int max_offset;
    bool is_loop;
    //string name;
    string* return_type;

    SymbolTable(int offset, bool is_loop, string return_type = "")
            : symbols(), max_offset(offset), is_loop(is_loop) {
        this->return_type = new string(return_type);
    }

    void add_symbol(const Symbol &symbol);

    bool symbol_exists(const string &name);

    Symbol *get_symbol(const string &name, string ret_type ="");

    Symbol *get_next_symbol(const string &name, vector<string> type_of_params, int i);

    ~SymbolTable(){
        delete return_type;
        for(auto it = symbols.begin(); it != symbols.end(); it++)
            delete (*it);
    }
};

class TableStack {
public:
    vector<SymbolTable*> table_stack;
    vector<int> offsets;

    TableStack();

    void push_scope(bool is_loop = false, string return_type = "");

    SymbolTable *current_scope();

    void pop_scope();

    void add_symbol(const string &name, const string &type, bool is_function, bool is_override=false, vector<string> params = vector<string>());

    void add_function_symbol(const string &name, const string &type, int offset);

    bool check_override(string id, string return_type, vector<string> param_types);

    Symbol *checkTheRightSymbol(const string &name, vector<string> type_of_params, int yylineno);

    bool symbol_exists(const string &name);

    bool check_loop();

    Symbol *get_symbol(const string &name, string type = "");

    void insert_symbol(SymbolTable &table, Symbol &symbol);

    void print_scopes();

    void check_program();
    ~TableStack(){
        for(auto it = table_stack.begin(); it != table_stack.end(); it++){
            SymbolTable* current = *it;
            delete current;
        }

    }
};

#endif
