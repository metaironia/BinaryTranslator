#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "../../lib/tree/tree_func.h"
#include "../../lib/tree/tree_log.h"

#include "../../lib/tree/math_tree/dsl_commands.h"
#include "../../lib/tree/math_tree/math_tree_func.h"

#include "ast_to_ir.h"
#include "ir_dsl.h"

IntReprFuncStatus IntReprCtor (IntRepr *interm_repr) {

    assert (interm_repr);

    IR_SIZE_     = 0;
    IR_CAPACITY_ = IR_DEFAULT_CAPACITY;

    IntReprDataCtor (interm_repr);

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprDataCtor (IntRepr *interm_repr) {

    IR_CELL_ = (IntReprCell *) calloc (IR_CAPACITY_, sizeof (IntReprCell));
    assert (IR_CELL_);

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprDataDtor (IntRepr *interm_repr) {

    assert (interm_repr);

    free (IR_CELL_);
    IR_CELL_ = NULL;

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprDtor (IntRepr *interm_repr) {

    assert (interm_repr);

    IntReprDataDtor (interm_repr);

    IR_SIZE_     = 0;
    IR_CAPACITY_ = 0;

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprDataRecalloc (IntRepr *interm_repr) {

    assert (interm_repr);

    const size_t all_cells_new_size = sizeof (IntReprCell) * (IR_INCREASE_NUM * IR_CAPACITY_); 
    const size_t all_cells_old_size = sizeof (IntReprCell) * IR_CAPACITY_;

    IR_CELL_ = (IntReprCell *) realloc (IR_CELL_, all_cells_new_size);

    memset (IR_CELL_ + IR_CAPACITY_, 0, all_cells_new_size - all_cells_old_size);

    IR_CAPACITY_ *= IR_INCREASE_NUM;

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprEmit (IntRepr *interm_repr, 
                               const char        *cmd_name,            const CommandType cmd_type,
                               const OperandType  dest_operand_type,   const ValueType   dest_value_type,
                               const double       dest_operand_value,  const int64_t     dest_operand_disp,
                               const bool         is_dest_operand_mem, const OperandType src_operand_type, 
                               const ValueType    src_value_type,      const double      src_operand_value,
                               const int64_t      src_operand_disp,    const bool        is_src_operand_mem,
                                     IntReprCell *jump_ptr,            const int64_t     jump_cell_index,
                               const int64_t      jump_addr,           const bool        need_patch) {

    assert (interm_repr);

    if (IR_SIZE_ >= IR_CAPACITY_)
        IntReprDataRecalloc (interm_repr);

    IR_TOP_CELL_ -> cmd_name = cmd_name;
    IR_TOP_CELL_ -> cmd_type = cmd_type;

    (IR_TOP_CELL_ -> dest_operand).operand_type       = dest_operand_type;  
    (IR_TOP_CELL_ -> dest_operand).operand_value_type = dest_value_type;
    (IR_TOP_CELL_ -> dest_operand).operand_value      = dest_operand_value;
    (IR_TOP_CELL_ -> dest_operand).operand_disp       = dest_operand_disp;
    (IR_TOP_CELL_ -> dest_operand).is_operand_mem     = is_dest_operand_mem;

    (IR_TOP_CELL_ -> src_operand).operand_type       = src_operand_type;
    (IR_TOP_CELL_ -> src_operand).operand_value_type = src_value_type;
    (IR_TOP_CELL_ -> src_operand).operand_value      = src_operand_value;
    (IR_TOP_CELL_ -> src_operand).operand_disp       = src_operand_disp;
    (IR_TOP_CELL_ -> src_operand).is_operand_mem     = is_src_operand_mem;

    IR_TOP_CELL_ -> jump_ptr         = jump_ptr;
    IR_TOP_CELL_ -> jump_cell_index  = jump_cell_index; 
    IR_TOP_CELL_ -> jump_addr        = jump_addr;
    IR_TOP_CELL_ -> need_patch       = need_patch;

    IR_SIZE_++;

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus TreeToIntRepr (IntRepr *interm_repr, const Tree *lang_tree) {

    assert (interm_repr);
    assert (lang_tree);

    IntReprBeginAndEnd (interm_repr);

    IntReprNewFuncWrite (interm_repr, lang_tree -> root);

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprBeginAndEnd (IntRepr *interm_repr) {

    assert (interm_repr);

    IR_EMIT_CMD_FUNC_CALL ("main");
    IR_EMIT_CMD_EXIT_;

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprFuncNameWrite (IntRepr *interm_repr, const TreeNode *current_node) {

    assert (interm_repr);

    MATH_TREE_NODE_VERIFY (current_node, IR);

    char *func_name = strdup ((char *) ((size_t) NODE_VALUE));

    IR_EMIT_COMMENT ("\n;----------------------------------------\n");
    IR_EMIT_COMMENT ("; ");
    IR_EMIT_COMMENT (func_name);
    IR_EMIT_COMMENT ("\n;----------------------------------------\n");

    IR_EMIT_CMD_FUNC_START (func_name); //TODO fix the mem leak

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprFuncPrologueWrite (IntRepr *interm_repr) {

    assert (interm_repr);

    IR_EMIT_COMMENT ("; rbp-chain\n");

    IR_EMIT_CMD_PUSH    (IR_OP_REG_RBP);
    IR_EMIT_CMD_MOVE_RR (IR_OP_REG_RBP, IR_OP_REG_RSP);

    IR_EMIT_COMMENT ("\n");

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprFuncEpilogueWrite (IntRepr *interm_repr) {

    assert (interm_repr);

    IR_EMIT_COMMENT ("\n; restoring rbp\n");
    
    IR_EMIT_CMD_POP (IR_OP_REG_RBP);

    IR_EMIT_COMMENT ("\n");

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprInitFuncArgsWrite (IntRepr *interm_repr, const TreeNode *current_node, int *mem_disp) {

    assert (interm_repr);
    assert (mem_disp);

    MATH_TREE_NODE_VERIFY (current_node, IR);

    if (!current_node) 
        return IR_FUNC_STATUS_OK;

    const TreeNode *current_arg_node = current_node;

    if (NODE_TYPE == LANGUAGE_OPERATOR) {

        switch (NODE_LANG_OPERATOR) {

            case COMMA:
                current_node = current_node -> right_branch;
            //fallthrough
            case INIT:
                current_node = current_node -> right_branch;
                break;

            default:
                return IR_FUNC_STATUS_FAIL;
        }

        IR_EMIT_CMD_MOVE_DOUBLE_RM (IR_OP_REG_XMM4, IR_OP_REG_RBP, -(*mem_disp) + 3 * STACK_CELL_SIZE);
        IR_EMIT_CMD_MOVE_DOUBLE_MR (IR_OP_REG_RBP, -((int64_t) NODE_VALUE + 1) * STACK_CELL_SIZE, IR_OP_REG_XMM4);

        *mem_disp   -= STACK_CELL_SIZE;
        current_node = current_arg_node;

        switch (NODE_LANG_OPERATOR) {

            case COMMA:
                return IntReprInitFuncArgsWrite (interm_repr, current_node -> left_branch, mem_disp);
                break;

            case INIT:
                return IR_FUNC_STATUS_OK;

            default:
                return IR_FUNC_STATUS_FAIL;
        }
    }

    return IR_FUNC_STATUS_FAIL;
}

IntReprFuncStatus IntReprNewFuncWrite (IntRepr *interm_repr, const TreeNode *current_node) {

    assert (interm_repr);

    MATH_TREE_NODE_VERIFY (current_node, IR);

    if (NODE_TYPE == LANGUAGE_OPERATOR) {

        const TreeNode *func_node = NULL;
     
        int mem_disp = 0;

        switch (NODE_LANG_OPERATOR) {

            case NEW_FUNC:
                func_node = current_node -> right_branch;
                break;

            case INIT:
                func_node = current_node;
                break;

            default:
                return IR_FUNC_STATUS_FAIL;
        }

        IntReprFuncNameWrite     (interm_repr, CURRENT_FUNC_NAME_NODE (func_node));
        IntReprFuncPrologueWrite (interm_repr);

        IR_EMIT_COMMENT          ("; start of init of func args\n");
        IntReprInitFuncArgsWrite (interm_repr, CURRENT_FUNC_NAME_NODE (func_node) -> left_branch, &mem_disp);
        IR_EMIT_COMMENT          ("; end of init of func args\n\n");

        IntReprLangOperatorWrite (interm_repr, CURRENT_FUNC_FIRST_END_LINE_NODE (func_node), &mem_disp);

        IR_EMIT_CMD_FUNC_END_;
        IR_EMIT_CMD_RET_;

        switch (NODE_LANG_OPERATOR) {

            case NEW_FUNC:
                IntReprNewFuncWrite (interm_repr, current_node -> left_branch);
                break;

            case INIT:
                return IR_FUNC_STATUS_OK;

            default:
                return IR_FUNC_STATUS_FAIL;
        }
    }

    return IR_FUNC_STATUS_FAIL;
}

IntReprFuncStatus IntReprLangOperatorWrite (IntRepr *interm_repr, const TreeNode *current_node, int *mem_disp) {

    assert (interm_repr);
    assert (mem_disp);

    if (!current_node)
        return IR_FUNC_STATUS_OK;

    MATH_TREE_NODE_VERIFY (current_node, IR);

    if (!(NODE_TYPE == LANGUAGE_OPERATOR && NODE_LANG_OPERATOR == END_LINE))
        return IR_FUNC_STATUS_FAIL;

    const TreeNode *end_line_node = current_node;

    current_node = current_node -> left_branch;

    if (NODE_TYPE == LANGUAGE_OPERATOR)
        switch (NODE_LANG_OPERATOR) {
            
            case IF:
                IntReprOperatorIfWrite (interm_repr, current_node, mem_disp);
                break;
            /*
            case WHILE:
                IntReprOperatorWhileWrite (asm_file, current_node);
                break;
            */
            case INIT:
                current_node = current_node -> right_branch;
            //fallthrough
            case ASSIGN:
                IntReprOperatorAssignWrite (interm_repr, current_node, mem_disp);
                break;
            
            case FUNC_RET:
                IntReprOperatorRetWrite (interm_repr, current_node, mem_disp);
                break;
            
            case FUNC_CALL:
                IntReprFuncCallWrite (interm_repr, current_node -> left_branch, mem_disp);
                break;
            
            case PRINT:
                IntReprOperatorPrintWrite (interm_repr, current_node, mem_disp);
                break;

            case READ:
                IntReprOperatorReadWrite   (interm_repr,   current_node, mem_disp);
                IR_EMIT_CMD_MOVE_DOUBLE_MR (IR_OP_REG_RBP, *mem_disp,    IR_OP_REG_XMM4);
                break;

            default:
                return IR_FUNC_STATUS_FAIL;
        }

    return IntReprLangOperatorWrite (interm_repr, end_line_node -> right_branch, mem_disp);
}

IntReprFuncStatus IntReprOperatorReadWrite (IntRepr *interm_repr, const TreeNode *current_node, int *mem_disp) {

    assert (interm_repr);
    assert (mem_disp);

    MATH_TREE_NODE_VERIFY (current_node, IR);

    if (NODE_TYPE == LANGUAGE_OPERATOR && NODE_LANG_OPERATOR == READ) {

        IR_EMIT_COMMENT ("\n");

        IR_EMIT_CMD_ADD_RI  (IR_OP_REG_RSP, *mem_disp - STACK_CELL_SIZE);
        IR_EMIT_CMD_READ_DOUBLE_;                                         // read value located in IR_OP_REG_XMM4
        IR_EMIT_CMD_MOVE_RR (IR_OP_REG_RSP, IR_OP_REG_RBP);

        IR_EMIT_COMMENT ("\n");

        return IR_FUNC_STATUS_OK;
    }

    return IR_FUNC_STATUS_FAIL;
}

IntReprFuncStatus IntReprOperatorPrintWrite (IntRepr *interm_repr, const TreeNode *current_node, int *mem_disp) {

    assert (interm_repr);
    assert (mem_disp);

    MATH_TREE_NODE_VERIFY (current_node, IR);

    if (NODE_TYPE == LANGUAGE_OPERATOR && NODE_LANG_OPERATOR == PRINT) {

        IR_EMIT_COMMENT ("; print operator\n");

        IntReprMathExpressionWrite (interm_repr, current_node -> left_branch, mem_disp);

        IR_EMIT_CMD_ADD_RI         (IR_OP_REG_RSP, *mem_disp - STACK_CELL_SIZE);

        IR_EMIT_COMMENT ("\n");

        IR_EMIT_CMD_MOVE_DOUBLE_RM (IR_OP_REG_XMM4, IR_OP_REG_RBP, *mem_disp);
        IR_EMIT_CMD_PRINT_DOUBLE_; // value to print located in IR_OP_REG_XMM4

        IR_EMIT_CMD_MOVE_RR        (IR_OP_REG_RSP, IR_OP_REG_RBP);

        IR_EMIT_COMMENT ("\n");

        *mem_disp += STACK_CELL_SIZE;

        return IR_FUNC_STATUS_OK;
    }

    return IR_FUNC_STATUS_FAIL;
}

IntReprFuncStatus IntReprOperatorRetWrite (IntRepr *interm_repr, const TreeNode *current_node, int *mem_disp) {

    assert (interm_repr);
    assert (mem_disp);

    MATH_TREE_NODE_VERIFY (current_node, IR);

    if (NODE_TYPE == LANGUAGE_OPERATOR && NODE_LANG_OPERATOR == FUNC_RET) {

        IntReprMathExpressionWrite (interm_repr,    current_node -> left_branch, mem_disp);
        IR_EMIT_CMD_MOVE_DOUBLE_RM (IR_OP_REG_XMM4, IR_OP_REG_RBP,               *mem_disp);

        IntReprFuncEpilogueWrite (interm_repr);

        IR_EMIT_CMD_RET_;

        return IR_FUNC_STATUS_OK;
    }

    return IR_FUNC_STATUS_FAIL;

}

IntReprFuncStatus IntReprOperatorIfWrite (IntRepr *interm_repr, const TreeNode *current_node, int *mem_disp) {

    assert (interm_repr);
    assert (mem_disp);

    MATH_TREE_NODE_VERIFY (current_node, IR);

    const TreeNode *current_if_node = current_node; 
    const int       old_mem_disp    = *mem_disp;

    IR_EMIT_COMMENT ("; if operator\n");

    IntReprOperatorOrAndWrite (interm_repr, current_node -> left_branch, mem_disp);

    const size_t before_if_interm_repr_size = IR_SIZE_;

    current_node = current_if_node -> left_branch;

    switch (NODE_MATH_OPERATOR) {

        case OPERATOR_GREATER:
            IR_EMIT_CMD_JUMP_GREATER_EQ_;
            break;

        case OPERATOR_LESS:
            IR_EMIT_CMD_JUMP_LESS_EQ_;
            break;

        case OPERATOR_EQUAL:
            IR_EMIT_CMD_JUMP_NOT_EQUAL_;
            break;

        case OPERATOR_NOT_EQUAL:
            IR_EMIT_CMD_JUMP_EQUAL_;
            break;
        
        default:
            IR_EMIT_COMMENT ("\nCAN NOT CONVERT IF OPERATOR\n");
    }

    current_node = current_if_node -> right_branch;
    *mem_disp    = old_mem_disp;

    IR_EMIT_COMMENT ("\n");

    IntReprLangOperatorWrite (interm_repr, current_node, mem_disp);

    IR_PATCH_CMD_JUMP          (before_if_interm_repr_size, IR_SIZE_);
    IR_ACTIVATE_JUMPABLE_STATE (IR_SIZE_);
    
    IR_EMIT_COMMENT ("\n");

    *mem_disp = old_mem_disp;

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprOperatorWhileWrite (IntRepr *interm_repr, const TreeNode *current_node, int *mem_disp) {

    assert (interm_repr);

    MATH_TREE_NODE_VERIFY (current_node, IR);

    IR_EMIT_COMMENT ("; while operator\n");

    const size_t before_while_interm_repr_size = IR_SIZE_;

    IntReprOperatorOrAndWrite (interm_repr, current_node -> left_branch, mem_disp);

    const size_t after_cond_while_interm_repr_size = IR_SIZE_;

    IR_EMIT_CMD_JUMP_EQUAL_;

    IR_EMIT_COMMENT ("\n");

    IR_ACTIVATE_JUMPABLE_STATE (IR_SIZE_);

    IntReprLangOperatorWrite (interm_repr, current_node -> right_branch, mem_disp);

    const size_t after_body_while_interm_repr_size = IR_SIZE_;

    IR_EMIT_CMD_JUMP_;

    IR_PATCH_CMD_JUMP (after_body_while_interm_repr_size, before_while_interm_repr_size);
    IR_PATCH_CMD_JUMP (after_cond_while_interm_repr_size, IR_SIZE_);

    IR_ACTIVATE_JUMPABLE_STATE (IR_SIZE_);

    IR_EMIT_COMMENT ("\n");

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprOperatorOrAndWrite (IntRepr *interm_repr, const TreeNode *current_node, int *mem_disp) {

/*
    WARNING:
    operators "and" and "or" are not supported now!
*/

    assert (interm_repr);
    assert (mem_disp);

    MATH_TREE_NODE_VERIFY (current_node, IR);

    if (NODE_TYPE == LANGUAGE_OPERATOR && (NODE_LANG_OPERATOR == OR || NODE_LANG_OPERATOR == AND)) {

        IntReprOperatorOrAndWrite      (interm_repr, current_node -> left_branch, mem_disp);

        IntReprOperatorComparisonWrite (interm_repr, current_node -> right_branch, mem_disp);
    }

    else
        return IntReprOperatorComparisonWrite (interm_repr, current_node, mem_disp);

    if (NODE_TYPE == LANGUAGE_OPERATOR)
        switch (NODE_LANG_OPERATOR) {

            case OR:
                //fprintf (asm_file, "add\n");
                break;

            case AND:
                //fprintf (asm_file, "mul\n");
                break;

            default:
                return IR_FUNC_STATUS_FAIL;
        }

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprOperatorComparisonWrite (IntRepr *interm_repr, const TreeNode *current_node, int *mem_disp) {

    assert (interm_repr);
    assert (mem_disp);

    MATH_TREE_NODE_VERIFY (current_node, IR);

    if (NODE_TYPE == BINARY_OPERATOR) {

        if (NODE_MATH_OPERATOR == OPERATOR_GREATER ||
            NODE_MATH_OPERATOR == OPERATOR_LESS ||
            NODE_MATH_OPERATOR == OPERATOR_EQUAL ||
            NODE_MATH_OPERATOR == OPERATOR_NOT_EQUAL) {

            IntReprMathExpressionWrite (interm_repr, current_node -> left_branch,  mem_disp);

            IR_EMIT_COMMENT ("\n");

            IntReprMathExpressionWrite (interm_repr, current_node -> right_branch, mem_disp);
        }

        else
            IntReprMathExpressionWrite (interm_repr, current_node, mem_disp);
    }

    else if (NODE_TYPE == NUMBER || NODE_TYPE == VARIABLE)
        IntReprMathExpressionWrite (interm_repr, current_node, mem_disp);

    else
        return IR_FUNC_STATUS_FAIL;

    switch (NODE_MATH_OPERATOR) {

        case OPERATOR_GREATER:
        case OPERATOR_LESS:
        case OPERATOR_EQUAL:
        case OPERATOR_NOT_EQUAL:
            IR_EMIT_CMD_MOVE_DOUBLE_RM (IR_OP_REG_XMM4, IR_OP_REG_RBP, *mem_disp);
            *mem_disp += STACK_CELL_SIZE;
            break;
        
        default:
            IR_EMIT_CMD_MOVE_DOUBLE_RI (IR_OP_REG_XMM4, 0);
    }

    IR_EMIT_COMMENT ("\n");

    IR_EMIT_CMD_CMP_DOUBLE_RM (IR_OP_REG_XMM4, IR_OP_REG_RBP, *mem_disp);

    *mem_disp += STACK_CELL_SIZE;

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprConditionWrite (IntRepr *interm_repr, const TreeNode *current_node, int *mem_disp) {

    assert (interm_repr);

    MATH_TREE_NODE_VERIFY (current_node, IR);

    return IntReprOperatorOrAndWrite (interm_repr, current_node, mem_disp);
}

IntReprFuncStatus IntReprOperatorAssignWrite (IntRepr *interm_repr, const TreeNode *current_node, int *mem_disp) {

    assert (interm_repr);
    assert (mem_disp);

    MATH_TREE_NODE_VERIFY (current_node, IR);

    IR_EMIT_COMMENT ("; assign operator\n");

    IntReprMathExpressionWrite (interm_repr, current_node -> right_branch, mem_disp);

    current_node = current_node -> left_branch;

    const int var_mem_disp = - ((int) NODE_VALUE * STACK_CELL_SIZE + STACK_CELL_SIZE);

    IR_EMIT_CMD_MOVE_DOUBLE_RM (IR_OP_REG_XMM4, IR_OP_REG_RBP, *mem_disp);
    IR_EMIT_CMD_MOVE_DOUBLE_MR (IR_OP_REG_RBP,  var_mem_disp,  IR_OP_REG_XMM4);

    if (var_mem_disp <= *mem_disp)
        *mem_disp = var_mem_disp;

    else
        *mem_disp += STACK_CELL_SIZE;

    IR_EMIT_COMMENT ("\n");

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprMathExpressionWrite (IntRepr *interm_repr, const TreeNode *current_node, int *mem_disp) {

    assert (interm_repr);
    assert (mem_disp);

    if (!current_node)
        return IR_FUNC_STATUS_FAIL;

    MATH_TREE_NODE_VERIFY (current_node, IR);

    switch (NODE_TYPE) {

        case NUMBER:
        case VARIABLE:
            IntReprVarOrNumWrite (interm_repr, current_node, mem_disp);
            return IR_FUNC_STATUS_OK;

        case LANGUAGE_OPERATOR:
            switch (NODE_LANG_OPERATOR) {
                
                case FUNC_CALL:
                    IntReprFuncCallWrite       (interm_repr,   current_node -> left_branch, mem_disp);
                    *mem_disp -= STACK_CELL_SIZE;
                    IR_EMIT_CMD_MOVE_DOUBLE_MR (IR_OP_REG_RBP, *mem_disp,                   IR_OP_REG_XMM4);
                    return IR_FUNC_STATUS_OK;
                
                case READ:
                    *mem_disp -= STACK_CELL_SIZE;
                    IntReprOperatorReadWrite   (interm_repr,   current_node, mem_disp);  // read value in IR_OP_REG_XMM4
                    IR_EMIT_CMD_MOVE_DOUBLE_MR (IR_OP_REG_RBP, *mem_disp,    IR_OP_REG_XMM4);
                    return IR_FUNC_STATUS_OK;

                default:
                    return IR_FUNC_STATUS_FAIL;
            }

        default:
            break;
    }

    switch (NODE_TYPE) {

        case BINARY_OPERATOR:
            if (IntReprMathExpressionWrite (interm_repr, current_node -> left_branch, mem_disp) == IR_FUNC_STATUS_FAIL)
                return IR_FUNC_STATUS_FAIL;

            if (IntReprMathExpressionWrite (interm_repr, current_node -> right_branch, mem_disp) == IR_FUNC_STATUS_FAIL)
                return IR_FUNC_STATUS_FAIL;

            break;

        case UNARY_OPERATOR:
            if (IntReprMathExpressionWrite (interm_repr, current_node -> left_branch, mem_disp) == IR_FUNC_STATUS_FAIL)
                return IR_FUNC_STATUS_FAIL;

            break;

        default:
            return IR_FUNC_STATUS_OK;
    }

    if (IntReprMathOperatorWrite (interm_repr, current_node, mem_disp) == IR_FUNC_STATUS_FAIL)
        return IR_FUNC_STATUS_FAIL;

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprFuncCallWrite (IntRepr *interm_repr, const TreeNode *current_node, int *mem_disp) {

    assert (interm_repr);

    MATH_TREE_NODE_VERIFY (current_node, IR);

    int old_mem_disp = *mem_disp;

    if (current_node -> left_branch) {

        IR_EMIT_COMMENT            ("; start of writing passed args\n");
        IntReprFuncPassedArgsWrite (interm_repr, current_node -> left_branch, mem_disp);
        IR_EMIT_COMMENT            ("; end of writing passed args\n");
    }

    IR_EMIT_COMMENT ("\n");

    IR_EMIT_CMD_ADD_RI    (IR_OP_REG_RSP, *mem_disp - STACK_CELL_SIZE);
    IR_EMIT_CMD_FUNC_CALL ((char *) ((size_t) NODE_VALUE));
    IR_EMIT_CMD_MOVE_RR   (IR_OP_REG_RSP, IR_OP_REG_RBP);

    IR_EMIT_COMMENT ("\n");

    *mem_disp = old_mem_disp;

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprFuncPassedArgsWrite (IntRepr *interm_repr, const TreeNode *current_node, int *mem_disp) {

    assert (interm_repr);

    if (!current_node)
        return IR_FUNC_STATUS_OK;

    MATH_TREE_NODE_VERIFY (current_node, IR);

    if (NODE_TYPE == LANGUAGE_OPERATOR && NODE_LANG_OPERATOR == COMMA) {

        IntReprFuncPassedArgsWrite (interm_repr, current_node -> left_branch, mem_disp);

        current_node = current_node -> right_branch;
    }

    if (IntReprMathExpressionWrite (interm_repr, current_node, mem_disp) == IR_FUNC_STATUS_FAIL)
        return IR_FUNC_STATUS_FAIL;

        //TODO make call_func as argument

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprVarOrNumWrite (IntRepr *interm_repr, const TreeNode *current_node, int *mem_disp) {

    assert (interm_repr);
    assert (mem_disp);

    MATH_TREE_NODE_VERIFY (current_node, IR);

    *mem_disp -= STACK_CELL_SIZE;

    if (NODE_TYPE == NUMBER)
        IR_EMIT_CMD_MOVE_DOUBLE_RI (IR_OP_REG_XMM4, NODE_VALUE);

    else if (NODE_TYPE == VARIABLE)
        IR_EMIT_CMD_MOVE_DOUBLE_RM (IR_OP_REG_XMM4, IR_OP_REG_RBP, -((int64_t) NODE_VALUE + 1) * STACK_CELL_SIZE);

    else
        return IR_FUNC_STATUS_FAIL;

    IR_EMIT_CMD_MOVE_DOUBLE_MR (IR_OP_REG_RBP, *mem_disp, IR_OP_REG_XMM4);

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus IntReprMathOperatorWrite (IntRepr *interm_repr, const TreeNode *current_node, int *mem_disp) {

    assert (interm_repr);
    assert (mem_disp);

    MATH_TREE_NODE_VERIFY (current_node, IR);

    IR_EMIT_CMD_MOVE_DOUBLE_RM (IR_OP_REG_XMM0, IR_OP_REG_RBP, *mem_disp + STACK_CELL_SIZE);
    IR_EMIT_CMD_MOVE_DOUBLE_RM (IR_OP_REG_XMM1, IR_OP_REG_RBP, *mem_disp);

    *mem_disp += STACK_CELL_SIZE;

    if (NODE_TYPE == BINARY_OPERATOR || NODE_TYPE == UNARY_OPERATOR)
        switch (NODE_MATH_OPERATOR) {

            case OPERATOR_ADD:
                IR_EMIT_CMD_ADD_DOUBLE_RR (IR_OP_REG_XMM0, IR_OP_REG_XMM1);
                break;

            case OPERATOR_SUB:
                IR_EMIT_CMD_SUB_DOUBLE_RR (IR_OP_REG_XMM0, IR_OP_REG_XMM1);
                break;

            case OPERATOR_MUL:
                IR_EMIT_CMD_MUL_DOUBLE_RR (IR_OP_REG_XMM0, IR_OP_REG_XMM1);
                break;

            case OPERATOR_DIV:
                IR_EMIT_CMD_DIV_DOUBLE_RR (IR_OP_REG_XMM0, IR_OP_REG_XMM1);
                break;

            case OPERATOR_SQRT:
                //fprintf (asm_file, "sqrt\n"); //TODO make sqrt command in dsl
                break;

            default:
                return IR_FUNC_STATUS_FAIL;
        }

    else
        return IR_FUNC_STATUS_FAIL;

    IR_EMIT_CMD_MOVE_DOUBLE_MR (IR_OP_REG_RBP, *mem_disp, IR_OP_REG_XMM0);

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus LangTreeVarsSet (Tree *lang_tree, const NameTable *lang_name_table) {

    MATH_TREE_VERIFY  (lang_tree,       IR, lang_name_table);
    NAME_TABLE_VERIFY (lang_name_table, IR);

    TreeNode *current_node = lang_tree -> root;

    while (current_node && NODE_TYPE == LANGUAGE_OPERATOR) {

        NameTable local_func_name_table = {};
        NameTableCtor (&local_func_name_table);

        TreeNode *current_func_node = current_node;

        switch (NODE_LANG_OPERATOR) {

            case NEW_FUNC:
                current_node = CURRENT_FUNC_NAME_NODE (current_node -> right_branch);
                break;

            case INIT:
                current_node = CURRENT_FUNC_NAME_NODE (current_node);
                break;

            default:
                return IR_FUNC_STATUS_FAIL;
        }

        NODE_VALUE = (double) ((size_t) NameTableVariableFind ((size_t) NODE_VALUE, lang_name_table));

        LangFuncVarsSet (current_node -> left_branch,  lang_name_table, &local_func_name_table);
        LangFuncVarsSet (current_node -> right_branch, lang_name_table, &local_func_name_table);

        current_node = current_func_node;

        switch (NODE_LANG_OPERATOR) {

            case NEW_FUNC:
                current_node = current_func_node -> left_branch;
                break;

            case INIT:
                return IR_FUNC_STATUS_OK;

            default:
                break;
        }

        NameTableDtor (&local_func_name_table);
    }

    return IR_FUNC_STATUS_OK;
}

IntReprFuncStatus LangFuncVarsSet (TreeNode *current_node, const NameTable *lang_name_table,
                                   NameTable *local_func_name_table) {

    if (!current_node)
        return IR_FUNC_STATUS_OK;

    MATH_TREE_NODE_VERIFY (current_node,          IR);
    NAME_TABLE_VERIFY     (local_func_name_table, IR);

    if (NODE_TYPE == VARIABLE) {

        const char *current_var_name  = NameTableVariableFind ((size_t) NODE_VALUE,   lang_name_table);
        long long   current_var_index = NameTableWordFind     (local_func_name_table, current_var_name, 0);

        if (current_var_index == -1) {

            current_var_index = local_func_name_table -> table_size;

            NameTableAdd (local_func_name_table, NAME_TABLE_VARIABLE, current_var_name, current_var_index);
        }

        NODE_VALUE = (double) current_var_index;

        return IR_FUNC_STATUS_OK;
    }

    if (NODE_TYPE == LANGUAGE_OPERATOR && NODE_LANG_OPERATOR == FUNC_CALL) {

        current_node = current_node -> left_branch;
        NODE_VALUE   = (double) ((size_t) NameTableVariableFind ((size_t) NODE_VALUE, lang_name_table));

        LangFuncVarsSet (current_node -> left_branch, lang_name_table, local_func_name_table);
    }

    else {

        LangFuncVarsSet (current_node -> left_branch,  lang_name_table, local_func_name_table);
        LangFuncVarsSet (current_node -> right_branch, lang_name_table, local_func_name_table);
    }

    return IR_FUNC_STATUS_OK;
}