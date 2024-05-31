#ifndef COMPI_HW5_Handler_H
#define COMPI_HW5_Handler_H

#include <string>
#include "types.h"
#include "bp.hpp"

using std::string;


class Handler{
    int register_max_num;
    int label_max_num;
public:
    Handler(): register_max_num(0), label_max_num(0){}

    //returns "[%, @] var_ [free reg num]
    std::string registerAllocate(bool is_global);

    //returns uniquelabel
    string labelAllocate(string label);

    void handle_global_code();
    string handle_load_var(string rbp, int offset);
    void handle_store_var(string rbp, int offset, string reg);
    void binop_code(Exp* res, const Exp& operand1, const Exp& operand2, const string& op);
    void relop_code(Exp* res, const Exp* operand1, const Exp* operand2, const string& op);
    void bool_eval_code(Exp* res, const Exp* operand1, const Exp* operand2, const string& op, const string& label);
    void assign_code(Exp* exp, int offset, bool is_bool);
    void return_code(string& type, string& reg);
    void return_value(string& type, string& value);
    void next_label_code(Exp* exp);
    string allocate_function_stack();
    void function_code(Call* func, ExpList* args);
    void function_declaration_code(Node* func_id, Formals* params, RetType* ret_type);
    void close_function(RetType* type);
    void merge_statement(Statement* statement1, Statement* statement2);
    void label_block_code(string label);
    void ruleID(Exp* exp);
    void ruleNum(Exp* exp);
    void ruleStr(Exp* exp);
    void ruleBool(Exp* exp);
    Exp* bool_exp(Exp* exp);

};
#endif //COMPI_HW5_Handler_H
