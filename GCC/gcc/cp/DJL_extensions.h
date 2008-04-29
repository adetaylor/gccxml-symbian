/*

  +---------------------------------------------------------------------+
  |  THIS FILE CONTAINS MODIFICATIONS NOT PART OF THE ORIGINAL GCC-XML  |
  |  DISTRIBUTION! THE CORRESPONDING PARTS ARE MARKED AS SUCH.          |
  |                                                                     |
  |  Copyright (c) 2006 Daniel J. Lauk                                  |
  +---------------------------------------------------------------------+

*/

/*
I shall have a short word on source code organisation, shall I?

1) GCC stuff
============
1.1) Syntax tree
----------------
In GCC's syntax tree there are three main node types, which are
  - expressions
  - declarations
  - statements
For each of those there is a DJL_xml_output_* function, which branches
into the specific output functions for each node.

All those specific functions are named by their corresponding macro's name.
So if the macro's name is ASM_EXPR the function is called
DJL_xml_output_asm_expr. Easy, isn't it?

BUT BE AWARE that ASM_EXPR denotes a statement, not an expression!
Now, don't blame me! I did not decide on those names. I'm just using them.

1.2) Helpers
------------
There are concepts that are used at different places. So instead of
copy & paste I rather have function calls do the magic. The concepts are:
  - DECL_NAME --> DJL_gcc_get_decl_name
  - TREE_CHAIN --> DJL_xml_output_tree_chain
  - nodes with a single child --> DJL_xml_output_single_child_node
  - conditions --> DJL_xml_output_condition
  - bodies --> DJL_xml_output_body

2) XML stuff
============
When doing XML, I like to have a higher level view of things. I just find
it more relaxing to use functions for openening and closing a tag, indenting
and all that stuff.

3) What goes where?
===================
Within their corresponding major sections functions are sorted in alphabetical
order for ease of working without IDEs that eat up all your memory and swap ;-)
...did I mention, that I simply *love* vim? :-)
*/

#ifndef __GCC_XML_DJL_EXTENSIONS_H_INCLUDED__
#define __GCC_XML_DJL_EXTENSIONS_H_INCLUDED__

#ifndef __GCC_XML_XML_H_INCLUDED__
#include "xml.h"
#endif

/* your entry point to this module */
void do_xml_dump_body(xml_dump_info_p xdi, tree t, int indent_level);
void do_xml_dump_expr(xml_dump_info_p xdi, tree t, int indent_level); 

/* gcc helper stuff */
static char *DJL_gcc_get_decl_name_unescaped(tree t);
static void DJL_xml_output_decl_name(xml_dump_info_p xdi,tree t);
static void DJL_xml_output_expr_location_attribute(xml_dump_info_p xdi, tree t);
static void DJL_xml_output_tree_chain(xml_dump_info_p xdi, tree t, int indent_level,
                                      const char *tag_name_element,
                                      const char *tag_name_purpose,
                                      const char *tag_name_value);
static void DJL_xml_output_single_child_node(xml_dump_info_p xdi, tree t, int indent_level,
                                      const char *tag_name,
                                      void (*child_handler)(xml_dump_info_p, tree, int));

/* XML helper stuff */
static char const *DJL_xml_ns = "body";
static char const *DJL_xml_ns_uri = "http://www.djlauk.de/";
static void DJL_xml_indent(xml_dump_info_p xdi, int indent_level);
static void DJL_xml_comment(xml_dump_info_p xdi, int indent_level, const char *comment);
static void DJL_xml_print_escaped(xml_dump_info_p xdi,const char *string);
/* for simple tags without attributes */
static void DJL_xml_open_tag(xml_dump_info_p xdi, int indent_level, const char *tag_name);
static void DJL_xml_close_tag(xml_dump_info_p xdi, int indent_level, const char *tag_name);
static void DJL_xml_empty_tag(xml_dump_info_p xdi, int indent_level, const char *tag_name);
/* for tags which may get location attributes */
static void DJL_xml_open_tag_with_location(xml_dump_info_p xdi, int indent_level, const char *tag_name, tree t);

/* higher level "catch-alls" */
static void DJL_xml_output_expression(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_statement(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_declaration(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_body(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_condition(xml_dump_info_p xdi, tree t, int indent_level);

/* low level syntax tree node handlers... */
static void DJL_xml_output_(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_abs_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_addr_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_aggr_init_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_array_ref(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_array_range_ref(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_asm_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_bind_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_bit_and_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_bit_ior_epxr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_bit_not_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_bit_xor_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_break_stmt(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_call_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_case_label(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_case_label_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_cast_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_catch_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_ceil_div_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_ceil_mod_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_cleanup_stmt(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_cleanup_point_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_complex_cst(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_complex_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_component_ref(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_compound_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_compound_literal_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_compound_stmt(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_cond_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_conj_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_const_decl(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_constructor(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_continue_stmt(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_convert_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_decl_stmt(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_do_stmt(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_empty_class_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_eq_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_exact_div_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_exc_ptr_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_exit_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_expr_stmt(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_expression(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_field_decl(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_fix_trunc_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_float_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_floor_div_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_floor_mod_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_for_stmt(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_function_decl(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_ge_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_gt_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_goto_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_handler(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_identifier(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_if_stmt(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_imagpart_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_indirect_ref(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_init_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_integer_cst(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_label_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_label_decl(xml_dump_info_p xdi, tree t, int indent_level); 
static void DJL_xml_output_label_stmt(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_le_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_loop_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_lshift_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_lt_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_max_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_min_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_minus_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_modify_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_mult_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_namespace_decl(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_ne_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_negate_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_non_lvalue_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_nop_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_obj_type_ref(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_ordered_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_parm_decl(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_plus_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_postdecrement_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_postincrement_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_predecrement_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_preincrement_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_primary_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_ptrmem_cst(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_rdiv_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_real_cst(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_realpart_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_result_decl(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_return_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_round_div_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_round_mod_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_rshift_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_save_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_scope_stmt(xml_dump_info_p xdi, tree t, int *indent_level);
static void DJL_xml_output_statement(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_stmt_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_string_cst(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_switch_stmt(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_target_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_throw_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_trunc_div_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_trunc_mod_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_truth_and_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_truth_andif_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_truth_not_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_truth_or_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_truth_orif_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_truth_xor_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_try_block(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_try_catch_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_try_finally_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_type(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_unary_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_uneq_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_unge_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_ungt_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_unle_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_unlt_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_unordered_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_using_stmt(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_va_arg_expr(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_var_decl(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_vector_cst(xml_dump_info_p xdi, tree t, int indent_level);
static void DJL_xml_output_while_stmt(xml_dump_info_p xdi, tree t, int indent_level);

#endif /* __GCC_XML_DJL_EXTENSIONS_H_INCLUDED__ */
