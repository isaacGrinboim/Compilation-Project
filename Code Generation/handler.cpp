#include "handler.h"
#include "symbol_table.h"
#include <string>

extern CodeBuffer buffer;
extern TableStack tables;

std::string Handler::registerAllocate(bool is_global) {
    if (is_global) {
        std::string reg = "@var_" + std::to_string(register_max_num);
        register_max_num++;
        return reg;
    }
    // case - register is not global
    std::string reg = "%var_" + std::to_string(register_max_num);
    register_max_num++;
    return reg;
};

string Handler::labelAllocate(string label) {
    string new_label = label + to_string(label_max_num);
    label_max_num++;
    return new_label;
}

void Handler::handle_global_code() {
    buffer.emit("@.DIV_BY_ZERO_ERROR = internal constant [23 x i8] c\"Error division by zero\\00\"");

    // Division by zero check function
    buffer.emit("define void @check_division(i32) {");
    buffer.emit("%valid = icmp eq i32 %0, 0");
    buffer.emit("br i1 %valid, label %ILLEGAL, label %LEGAL");
    buffer.emit("ILLEGAL:");
    buffer.emit("call void @print(i8* getelementptr([23 x i8], [23 x i8]* @.DIV_BY_ZERO_ERROR, i32 0, i32 0))");
    buffer.emit("call void @exit(i32 0)");
    buffer.emit("ret void");
    buffer.emit("LEGAL:");
    buffer.emit("ret void");
    buffer.emit("}");
}


void Handler::label_block_code(string label) {
    //std::cout<<"br label %" << label<< "\n";
    //size_t found = buffer.buffer.back().find("br ");
    if(buffer.buffer.back().rfind("br ", 0) !=0)
        buffer.emit("br label %" + label);
    buffer.emit(label + ":");
}

void Handler::binop_code(Exp *res, const Exp &operand1, const Exp &operand2, const string &op) {
    res->reg = registerAllocate(false);
    string op_text;
    if (op == "+") {
        op_text = "add";
    } else if (op == "-") {
        op_text = "sub";
    } else if (op == "*") {
        op_text = "mul";
    } else {
        if (res->type == "int") {
            op_text = "sdiv";
        } else {
            op_text = "udiv";
        }
    }
    if (op == "/") {
        buffer.emit("call void @check_division(i32 " + operand2.reg + ")");
        buffer.emit(res->reg + " = " + op_text + " i32 " + operand1.reg + ", " + operand2.reg);
    } else {
        buffer.emit(res->reg + " = " + op_text + " i32 " + operand1.reg + ", " + operand2.reg);
        if (res->type == "byte") {
            string old_reg = res->reg;
            res->reg = registerAllocate(false);
            buffer.emit(res->reg + " = and i32 255, " + old_reg);
        }
    }

}

void Handler::relop_code(Exp *res, const Exp *operand1, const Exp *operand2, const string &op) {
    res->reg = registerAllocate(false);
    string op_text;
    if (op == "==") {
        op_text = "eq";
    } else if (op == "!=") {
        op_text = "ne";
    } else if (op == ">") {
        op_text = "sgt";
    } else if (op == ">=") {
        op_text = "sge";
    } else if (op == "<") {
        op_text = "slt";
    } else {
        op_text = "sle";
    }

    buffer.emit(res->reg + " = icmp " + op_text + " i32 " + operand1->reg + ", " + operand2->reg);
    int address = buffer.emit("br i1 " + res->reg + ", label @, label @");
    res->true_list = buffer.makelist(pair<int, BranchLabelIndex>(address, FIRST));
    res->false_list = buffer.makelist(pair<int, BranchLabelIndex>(address, SECOND));
}


void Handler::bool_eval_code(Exp *res, const Exp *operand1, const Exp *operand2, const string &op, const string &label) {
    if (op == "and") {
        buffer.bpatch(operand1->true_list, label);
        res->true_list = backPatchList(operand2->true_list);
        res->false_list = buffer.merge(operand1->false_list, operand2->false_list);
    } else if (op == "or") {
        buffer.bpatch(operand1->false_list, label);
        res->true_list = buffer.merge(operand1->true_list, operand2->true_list);
        res->false_list = backPatchList (operand2->false_list);
    } else {
        res->true_list = backPatchList(operand1->false_list);
        res->false_list = backPatchList(operand1->true_list);
    }

}

void Handler::assign_code(Exp *exp, int offset, bool is_bool) {
    if (is_bool) {
        Exp *new_exp = bool_exp(exp);
        handle_store_var(tables.current_scope()->rbp, offset, new_exp->reg);
    } else {
        handle_store_var(tables.current_scope()->rbp, offset, exp->reg);
    }
}

void Handler::return_code(string &type, string &reg) {
    if (type.empty())
        buffer.emit("ret i32 0");
    else {
        string return_type;
        if (type == "string") {
            return_type = "i8*";
        } else {
            return_type = "i32";
        }
        buffer.emit("ret " + return_type + " " + reg);
    }
    //exit(0);
}

void Handler::return_value(string &type, string &value) {
    string return_type;
    if (type == "string") {
        return_type = "i8*";
    } else {
        return_type = "i32";
    }
    buffer.emit("ret " + return_type + " " + value);
}

void Handler::ruleNum(Exp *exp) {
    buffer.emit(exp->reg + " = add i32 " + exp->value + ", 0");
}

void Handler::ruleStr(Exp *exp) {
    string str = exp->value;
    str.pop_back();
    string reg = registerAllocate(true);
    string size_str = "[" + to_string(str.length()) + " x i8]";
    string get_ptr = "getelementptr" + size_str + ", " + size_str + "* " + reg + ", i32 0, i32 0";
    buffer.emitGlobal(reg + " = constant " + size_str + " c" + str + "\\00\"");
    reg.replace(0, 1, "%");
    buffer.emit(reg + ".ptr = " + get_ptr);
    exp->reg = reg + ".ptr";
}

void Handler::ruleBool(Exp *exp) {
    int address = buffer.emit("br label @");
    if (exp->value == "true") {
        exp->true_list = buffer.makelist(pair<int, BranchLabelIndex>(address, FIRST));
    } else {
        exp->false_list = buffer.makelist(pair<int, BranchLabelIndex>(address, SECOND));
    }

}

void Handler::ruleID(Exp *exp) {
    Symbol *symbol = tables.get_symbol(exp->value);
    if (symbol->offset < 0) {
        exp->reg = "%" + std::to_string(((-1) * symbol->offset - 1));
    } else {
        exp->reg = handle_load_var(tables.current_scope()->rbp, symbol->offset);
    }

    if (exp->type == "bool") {
        string cmp = registerAllocate(false);
        buffer.emit(cmp + " = icmp ne i32 0, " + exp->reg);
        int address = buffer.emit("br i1 " + cmp + ", label @, label @");
        exp->true_list = buffer.makelist(pair<int, BranchLabelIndex>(address, FIRST));
        exp->false_list = buffer.makelist(pair<int, BranchLabelIndex>(address, SECOND));
    }
//    exp->type = symbol->type;
}


string Handler::handle_load_var(string rbp, int offset) {
    string reg = registerAllocate(false);
    string var_ptr = registerAllocate(false);
    buffer.emit(var_ptr + " = getelementptr i32, i32* " + rbp + ", i32 " + std::to_string(offset));
    buffer.emit(reg + " = load i32, i32* " + var_ptr);
    return reg;
}

void Handler::handle_store_var(string rbp, int offset, string reg) {
    string var_ptr = registerAllocate(false);
    buffer.emit(var_ptr + " = getelementptr i32, i32* " + rbp + ", i32 " + std::to_string(offset));
    buffer.emit("store i32 " + reg + ", i32* " + var_ptr);
}

Exp *Handler::bool_exp(Exp *exp) {
    if (exp->type != "bool") {
        return new Exp(exp);
    }
    Exp *new_exp = new Exp();
    new_exp->reg = registerAllocate(false);
    new_exp->type = "bool";
    string true_list = labelAllocate("PHI_TRUE_LABEL");
    string false_list = labelAllocate("PHI_FALSE_LABEL");
    string next_list = labelAllocate("PHI_NEXT_LABEL");

    buffer.emit_uncond_jump(true_list);
    buffer.emit(true_list + ":");
    int address1 = buffer.emit_uncond_jump("@");

    buffer.emit(false_list + ":");
    int address2 = buffer.emit_uncond_jump("@");

    buffer.emit(next_list + ":");

    backPatchList next = buffer.merge(buffer.makelist(pair<int, BranchLabelIndex>(address1, FIRST)),
                               buffer.makelist(pair<int, BranchLabelIndex>(address2, FIRST)));
    buffer.bpatch(exp->true_list, true_list);
    buffer.bpatch(exp->false_list, false_list);
    buffer.bpatch(next, next_list);
    buffer.emit_phi(new_exp, true_list, false_list);

    return new_exp;
}

void Handler::next_label_code(Exp *exp) {
    int address = buffer.emit("br label @");
    exp->next_list = buffer.merge(buffer.makelist(pair<int, BranchLabelIndex>(address, FIRST)),
                                  exp->next_list);
}

string Handler::allocate_function_stack() {
    string rbp = registerAllocate(false);
    buffer.emit(rbp + " = alloca i32, i32 50");
    return rbp;
}

void Handler::function_code(Call *func, ExpList *args) {
    func->reg = registerAllocate(false);
    string emitted_args = "";
    for (int i = 0; i < args->expressions.size(); ++i) {
        Exp exp = args->expressions[i];
        if (exp.type == "string") {
            emitted_args += "i8*";
        } else {
            emitted_args += "i32";
        }
        emitted_args += " ";
        emitted_args += exp.reg;
        if (i < args->expressions.size() - 1)
            emitted_args += ", ";
    }
    if (func->type == "void")
        buffer.emit("call void @" + func->value + "(" + emitted_args + ")");
    else if (func->type == "bool") {
        buffer.emit(func->reg + " = call i32 @" + func->value + "(" + emitted_args + ")");
        Exp exp = Exp();
        exp.type = "bool";
        exp.reg = registerAllocate(false);
       buffer.emit(exp.reg + " = icmp ne i32 0, " + func->reg);
        func->reg = exp.reg;
        int address = buffer.emit("br i1 " + func->reg + " , label @, label @");
        func->true_list = buffer.makelist(pair<int, BranchLabelIndex>(address, FIRST));
        func->false_list = buffer.makelist(pair<int, BranchLabelIndex>(address, SECOND));
    } else {
        string emitted_return_type = func->type == "void" ? "i8*" : "i32";
        buffer.emit(func->reg + " = call " + emitted_return_type + " @" + func->value + "(" + emitted_args + ")");
    }
    //std::cout<<func->value<<"    "<<buffer.size()<<"\n";
}

void Handler::function_declaration_code(Node *func_id, Formals *params, RetType *ret_type) {
    string emitted_args = "";
    for (int i = 0; i < params->formals_list.size(); i++) {
        FormalDecl *param = params->formals_list[i];
        if (param->type == "string") {
            emitted_args += "i8*";
        } else {
            emitted_args += "i32";
        }
        if (i < params->formals_list.size() - 1)
            emitted_args += ", ";
    }
    string emitted_return_type;
    if (ret_type->type == "void") {
        emitted_return_type = "void";
    } else if (ret_type->type == "string") {
        emitted_return_type = "i8*";
    } else {
        emitted_return_type = "i32";
    }
    buffer.emit("define " + emitted_return_type + " @" + func_id->value + "(" + emitted_args + "){");
}

void Handler::close_function(RetType *type) {
    if (type->type == "void") {
        buffer.emit("ret void");
    } else {
        buffer.emit("ret i32 0");
    }
    buffer.emit("}");
    //std::cout <<"close!!!";
}

void Handler::merge_statement(Statement *statement1, Statement *statement2) {
    statement1->break_list = buffer.merge(statement1->break_list, statement2->break_list);
    statement1->cont_list = buffer.merge(statement1->cont_list, statement2->cont_list);
}



