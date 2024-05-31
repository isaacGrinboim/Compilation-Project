#ifndef COMPI_HW3_SYMBOL_TABLE_H
#define COMPI_HW3_SYMBOL_TABLE_H

#include <string>
#include <vector>
#include <iostream>

#define DEBUG false

using std::string;
using std::vector;

string convert_to_upper_case(const string &str);

class Symbol
{
public:
    string name;
    // If class is function, this is return type
    string type;
    int offset;
    bool is_function;
    bool is_override;
    // param types
    vector<string> params;

    Symbol(const string name, const string type, int offset, vector<string> params, bool is_function, bool is_override = false) : name(name),
                                                                                                                                  type(type),
                                                                                                                                  offset(offset),
                                                                                                                                  params(params),
                                                                                                                                  is_function(is_function),
                                                                                                                                  is_override(is_override)
    {
    }
    ~Symbol() = default;
};

// SymbolTable Class:
// A class for managing symbols in a symbol table.
class SymbolTable
{

public:
    vector<Symbol *> symbols;
    int max_offset;
    bool is_loop;
    // string name;
    string *return_type;
    string rbp;

    SymbolTable(int offset, bool is_loop, string return_type = "")
        : symbols(), max_offset(offset), is_loop(is_loop)
    {
        this->return_type = new string(return_type);
    }

    // Function to add a symbol to the symbol table.
    // Parameters:
    // - symbol: A Symbol object to be added to the symbol table.
    void add_symbol(const Symbol &symbol);

    // Function to check if a symbol with a given name exists in the symbol table.
    // Parameters:
    // - name: A string representing the name of the symbol.
    // Returns: true if the symbol exists, false otherwise.
    bool symbol_exists(const string &name);

    // Function to get a symbol with a given name and optional return type.
    // Parameters:
    // - name: A string representing the name of the symbol.
    // - ret_type: An optional string representing the return type of the symbol (default is an empty string).
    // Returns: A pointer to the Symbol object if found, nullptr otherwise.
    Symbol *get_symbol(const string &name, string ret_type = "");

    // Symbol *get_next_symbol(const string &name, vector<string> type_of_params, int i);

    ~SymbolTable()
    {
        delete return_type;
        for (auto it = symbols.begin(); it != symbols.end(); it++)
            delete (*it);
    }
};

// TableStack Class:
// A class for managing a stack of symbol tables.
class TableStack
{
public:
    vector<SymbolTable *> table_stack;
    vector<int> offsets;

    // Constructor for TableStack class.
    TableStack();

    // Function to push a new symbol table onto the stack.
    // Parameters:
    // - is_loop: A boolean indicating if the new scope represents a loop.
    // - return_type: An optional string representing the return type of the new scope (default is an empty string).
    void push_scope(bool is_loop = false, string return_type = "");

    // Function to get the current (top) symbol table on the stack.
    // Returns: A pointer to the current symbol table.
    SymbolTable *current_scope();

    // Function to pop the current (top) symbol table from the stack.
    void pop_scope();

    // Function to add a symbol to the current symbol table on the stack.
    // Parameters:
    // - name: A string representing the name of the symbol.
    // - type: A string representing the type of the symbol.
    // - is_function: A boolean indicating if the symbol is a function.
    // - is_override: A boolean indicating if the symbol is an overridden function.
    // - params: A vector of strings representing the parameter types of the function (if applicable).
    void add_symbol(const string &name, const string &type, bool is_function, bool is_override = false, vector<string> params = vector<string>());

    // Function to add a function symbol to the current symbol table on the stack.
    // Parameters:
    // - name: A string representing the name of the function.
    // - type: A string representing the return type of the function.
    // - offset: An integer representing the offset of the function.
    void add_function_symbol(const string &name, const string &type, int offset);

    // Function to check if a function with a given name, return type, and parameter types is overridden.
    // Parameters:
    // - id: A string representing the name of the function.
    // - return_type: A string representing the return type of the function.
    // - param_types: A vector of strings representing the parameter types of the function.
    // - count: A reference to an integer to count the number of overridden functions found.
    // Returns: true if the function is overridden, false otherwise.
    bool check_override(string id, string return_type, vector<string> param_types, int &count);

    // Function to find the right symbol with given name and parameter types.
    // Parameters:
    // - name: A string representing the name of the symbol.
    // - type_of_params: A vector of strings representing the parameter types.
    // - yylineno: An integer representing the line number where the symbol is found.
    // - count: A reference to an integer to count the number of matching symbols found.
    // Returns: A pointer to the right Symbol object if found, nullptr otherwise.
    Symbol *checkTheRightSymbol(const string &name, vector<string> type_of_params, int yylineno, int &count);

    // Function to check if a symbol with a given name exists in the symbol tables on the stack.
    // Parameters:
    // - name: A string representing the name of the symbol to check.
    // Returns: true if the symbol exists in any of the symbol tables on the stack, false otherwise.
    bool symbol_exists(const string &name);

    // Function to check if a loop is currently active on the stack.
    // Returns: true if a loop is active, false otherwise.
    bool check_loop();

    // Function to get a symbol with a given name and optional type from the symbol tables on the stack.
    // Parameters:
    // - name: A string representing the name of the symbol.
    // - type: An optional string representing the type of the symbol (default is an empty string).
    // Returns: A pointer to the Symbol object if found, nullptr otherwise.
    Symbol *get_symbol(const string &name, string type = "");

    // void insert_symbol(SymbolTable &table, Symbol &symbol);

    // void print_scopes();

    // Function to check the main function's existence in the global scope.
    void check_program();
    ~TableStack()
    {
        for (auto it = table_stack.begin(); it != table_stack.end(); it++)
        {
            SymbolTable *current = *it;
            delete current;
        }
    }
};

#endif
