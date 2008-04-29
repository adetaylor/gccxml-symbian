/*

  +---------------------------------------------------------------------+
  |  THIS FILE CONTAINS MODIFICATIONS NOT PART OF THE ORIGINAL GCC-XML  |
  |  DISTRIBUTION! THE CORRESPONDING PARTS ARE MARKED AS SUCH.          |
  |                                                                     |
  |  Copyright (c) 2006 Daniel J. Lauk                                  |
  +---------------------------------------------------------------------+

*/

#include "config.h"
#include "system.h"
#include "sys/stat.h"

/* GCC 3.4 and above need these headers here.  The GCC-XML patches for
   these versions define GCC_XML_GCC_VERSION in config.h instead of
   cp-tree.h, so the macro is available here.  The patches for older
   versions may provide the macro in cp-tree.h, but in that case
   we don't need these headers anyway.  */
#if defined(GCC_XML_GCC_VERSION) && (GCC_XML_GCC_VERSION >= 0x030400)
# include "coretypes.h"
# include "tm.h"
#endif

#include "tree.h"
#include "cp-tree.h"
#include "decl.h"
#include "rtl.h"
#include "varray.h"

#include "splay-tree.h"

#include "demangle.h"

#include "tree-iterator.h"

#include "toplev.h" /* ident_hash */
/* If you want to know anything about the source code's organisation,
have a look at "DJL_extensions.h". */

#ifndef __GCC_XML_DJL_EXTENSIONS_H_INCLUDED__
#include "DJL_extensions.h"
#endif
#include "real.h"


/* your entry point to this module */
void do_xml_dump_body(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Dump";
  DJL_xml_indent(xdi, indent_level++);
  fprintf(xdi->file, "<%s:%s xmlns:%s=\"%s\">\n",
    DJL_xml_ns, tag_name, DJL_xml_ns, DJL_xml_ns_uri);
  DJL_xml_output_body(xdi, t, indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

void do_xml_dump_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Dump";
  if (!t || (t == error_mark_node)) return;
  if (TREE_CODE (t) == CONSTRUCTOR) return; //FIXME - This may be needed for c++
  DJL_xml_indent(xdi, indent_level++);
  fprintf(xdi->file, "<%s:%s xmlns:%s=\"%s\">\n",
    DJL_xml_ns, tag_name, DJL_xml_ns, DJL_xml_ns_uri);
  
  DJL_xml_output_expression(xdi, t, indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

/* xml helpers */
static void DJL_xml_indent(xml_dump_info_p xdi, int indent_level) {
  int i;
  for (i=0; i < indent_level; i++) fprintf(xdi->file, "  ");
}

static void DJL_xml_comment_short(xml_dump_info_p xdi, int indent_level, const char *comment) {
  DJL_xml_indent(xdi, indent_level);
  fprintf(xdi->file, "<!-- %s -->\n", comment);
}

static void DJL_xml_comment_long(xml_dump_info_p xdi, int indent_level, const char *comment) {
  DJL_xml_indent(xdi, indent_level);
  fprintf(xdi->file, "<!--\n%s\n", comment);
  DJL_xml_indent(xdi->file, indent_level);
  fprintf(xdi->file, "-->\n");
}

static void DJL_xml_print_escaped(xml_dump_info_p xdi,const char *string) {
  const char *newval = xml_escape_string(string);
  fprintf(xdi->file,newval);
  free(newval);
}

static void DJL_xml_output_tree_chain(xml_dump_info_p xdi, tree t, int indent_level,
                                      const char *tag_name_element,
                                      const char *tag_name_purpose,
                                      const char *tag_name_value) {
  for ( ; t ; t = TREE_CHAIN(t)) {
    if (tag_name_purpose || tag_name_value) {
      DJL_xml_open_tag(xdi, indent_level++, tag_name_element);
      if (tag_name_purpose)
        DJL_xml_output_single_child_node(xdi, TREE_PURPOSE(t), indent_level, tag_name_purpose, &DJL_xml_output_expression);
      if (tag_name_value)
        DJL_xml_output_single_child_node(xdi, TREE_VALUE(t), indent_level, tag_name_value, &DJL_xml_output_expression);
      DJL_xml_close_tag(xdi, --indent_level, tag_name_element);
    } else
      DJL_xml_empty_tag(xdi, indent_level, tag_name_element);
  }
}

static void DJL_xml_output_single_child_node(xml_dump_info_p xdi, tree t, int indent_level,
                                      const char *tag_name,
                                      void (*child_handler)(xml_dump_info_p, tree, int)) {
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  (*child_handler)(xdi, t, indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

/* gcc helper stuff */
static char *DJL_gcc_get_decl_name_unescaped(tree t) {
  char *rc = "";
  tree dn = NULL; 
  if(t) dn = DECL_NAME(t);
  //printf("Got past DECL_NAME\n");
  //printf("DECL_NAME = %s(0x%08X)\n", dn, dn);
  if(dn){
    //char *ip = IDENTIFIER_POINTER(dn);
    //return IDENTIFIER_POINTER(DECL_NAME(t));
    rc = IDENTIFIER_POINTER(dn);
  }
  return rc;
}

static void DJL_xml_output_decl_name(xml_dump_info_p xdi,tree t) {
  char* dn = DJL_gcc_get_decl_name_unescaped(t);
  DJL_xml_print_escaped(xdi,dn);
}

static void DJL_xml_output_expr_location_attribute(xml_dump_info_p xdi, tree t) {
  unsigned int source_file = xml_queue_file (xdi, EXPR_FILENAME (t));
  unsigned int source_line = EXPR_LINENO (t);

  fprintf (xdi->file, " location=\"f%d:%d\" file=\"f%d\" line=\"%d\"",
           source_file, source_line, source_file, source_line);
}

/* for simple tags without attributes */
static void DJL_xml_open_tag(xml_dump_info_p xdi, int indent_level, const char *tag_name) {
  DJL_xml_indent(xdi, indent_level);
//  fprintf(xdi->file, "<%s:%s>\n", DJL_xml_ns, tag_name);
  fprintf(xdi->file, "<%s:%s ", DJL_xml_ns, tag_name);
  //xml_print_location_attribute (xdi, d);
  fprintf(xdi->file, ">\n");
}

static void DJL_xml_close_tag(xml_dump_info_p xdi, int indent_level, const char *tag_name) {
  DJL_xml_indent(xdi, indent_level);
  fprintf(xdi->file, "</%s:%s>\n", DJL_xml_ns, tag_name);
}

static void DJL_xml_empty_tag(xml_dump_info_p xdi, int indent_level, const char *tag_name) {
  DJL_xml_indent(xdi, indent_level);
  fprintf(xdi->file, "<%s:%s />\n", DJL_xml_ns, tag_name);
}

/* for tags with location attributes but no other attributes */
static void DJL_xml_open_tag_with_location(xml_dump_info_p xdi, int indent_level, const char *tag_name, tree t) {
  DJL_xml_indent(xdi, indent_level);
  fprintf(xdi->file, "<%s:%s ", DJL_xml_ns, tag_name);
  if (EXPR_HAS_LOCATION(t))
    DJL_xml_output_expr_location_attribute (xdi, t);
  fprintf(xdi->file, ">\n");
}

/* -------------------------------------------------------------------------- */
/* functional stuff */

/* This is a omnivalent place holder. use it whenever you don't know what will
   be coming up. It will tell you the tree code and it's name (-> macro) */
static void DJL_xml_output_(xml_dump_info_p xdi, tree t, int indent_level) {
  DJL_xml_indent(xdi, indent_level);
  fprintf(xdi->file, "<%s:UNDEFINED TREE_CODE=\"%d\" tree_code_name=\"%s\" />\n",
    DJL_xml_ns, (int)TREE_CODE(t), tree_code_name[(int)TREE_CODE(t)]);
}

static void DJL_xml_output_abs_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Abs_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_addr_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Addr_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_aggr_init_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Aggr_Init_Expr";
  tree arg = TREE_OPERAND(t, 1);
  int via_constructor = AGGR_INIT_VIA_CTOR_P(t) ? 1 : 0;

  DJL_xml_indent(xdi, indent_level++);
  fprintf(xdi->file, "<%s:%s via_constructor=\"%d\">\n",
    DJL_xml_ns, tag_name, via_constructor);
  /* operand 0: pointer to function to call */
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, "Address", &DJL_xml_output_expression);
  /* operand 1: arguments, a TREE_LIST */
  DJL_xml_open_tag(xdi, indent_level++, "Arguments");
  if (via_constructor) {
  /* operand 2: replacement for first argument in the
                argument list, if init via constructor */
    DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 2), indent_level, "Argument", &DJL_xml_output_expression);
    arg = TREE_CHAIN(arg); /* first one was replaced, right? -> skip it */
  }
  for ( ; arg ; arg = TREE_CHAIN(arg))
      DJL_xml_output_single_child_node(xdi, TREE_VALUE(arg), indent_level, "Argument", &DJL_xml_output_expression);
  DJL_xml_close_tag(xdi, --indent_level, "Arguments");
  /* operand 2: var_decl to init */
  DJL_xml_output_var_decl(xdi, TREE_OPERAND(t, 2), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_array_ref(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Array_Ref";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_array_range_ref(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Array_Range_Ref";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_asm_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Asm_Stmt";
  bool is_volatile = ASM_VOLATILE_P (t);
  DJL_xml_indent(xdi, indent_level++);
  fprintf(xdi->file, "<%s:%s volatile=\"%d\">", DJL_xml_ns, tag_name, is_volatile);
  DJL_xml_output_single_child_node(xdi, ASM_STRING(t), indent_level, "Asm_String", &DJL_xml_output_string_cst);
  if (ASM_OUTPUTS(t)) DJL_xml_output_single_child_node(xdi, ASM_OUTPUTS(t), indent_level, "Asm_Outputs", &DJL_xml_output_expression);
  if (ASM_INPUTS(t)) DJL_xml_output_single_child_node(xdi, ASM_INPUTS(t), indent_level, "Asm_Inputs", &DJL_xml_output_expression);
  if (ASM_CLOBBERS(t)) DJL_xml_output_single_child_node(xdi, ASM_CLOBBERS(t), indent_level, "Asm_Clobbers", &DJL_xml_output_expression);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_bind_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Bind_Expr";
  tree local_var = NULL;
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  /* local variables */
  DJL_xml_open_tag(xdi, indent_level++, "Local_Vars");
  for (local_var = TREE_OPERAND(t, 0) ; local_var ; local_var = TREE_CHAIN(local_var))
    DJL_xml_output_expression(xdi, local_var, indent_level);
  DJL_xml_close_tag(xdi, --indent_level, "Local_Vars");
  /* body */
  DJL_xml_output_body(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_bit_and_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Bit_And_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_bit_field_ref(xml_dump_info_p xdi, tree t, int indent_level){
  char *tag_name = "Bit_Field_Ref";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 2), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}



static void DJL_xml_output_bit_ior_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Bit_Ior_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_bit_not_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Bit_Not_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_bit_xor_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Bit_Xor_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_body(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Body";
  DJL_xml_output_single_child_node(xdi, t, indent_level, tag_name, &DJL_xml_output_statement);
}

static void DJL_xml_output_break_stmt(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Break_Stmt";
  DJL_xml_empty_tag(xdi, indent_level, tag_name);
}

static void DJL_xml_output_call_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Call_Expr";
  DJL_xml_open_tag_with_location(xdi, indent_level++, tag_name, t);
  /* operand 0: pointer to function to call */
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, "Address", &DJL_xml_output_expression);
  /* operand 1: arguments, a TREE_LIST */
  DJL_xml_open_tag(xdi, indent_level++, "Arguments");
  for (t = TREE_OPERAND(t, 1) ; t ; t = TREE_CHAIN(t))
      DJL_xml_output_single_child_node(xdi, TREE_VALUE(t), indent_level, "Argument", &DJL_xml_output_expression);
  DJL_xml_close_tag(xdi, --indent_level, "Arguments");
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

//static void DJL_xml_output_case_label(xml_dump_info_p xdi, tree t, int indent_level) {
//  char *tag_name = "Case_Label";
//  if (CASE_LOW(t) == NULL_TREE) {
//    DJL_xml_empty_tag(xdi, indent_level, "Default");
//    return;
//  }
//  DJL_xml_open_tag(xdi, indent_level++, tag_name);
//  /* Handle lower (or single) bound of case expr */
//  DJL_xml_output_single_child_node(xdi, CASE_LOW(t), indent_level, "Case_Low", &DJL_xml_output_expression);
//  /* Handle upper (optional) bound of case expr */
//  if (CASE_HIGH(t))
//    DJL_xml_output_single_child_node(xdi, CASE_LOW(t), indent_level, "Case_High", &DJL_xml_output_expression);
//
//  DJL_xml_close_tag(xdi, --indent_level, tag_name);
//}

static void DJL_xml_output_catch_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char* tag_name = "Catch_Expr";
  char* catch_types = "Catch_Types";
  char* catch_body = "Catch_Body";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  if (CATCH_TYPES(t) != NULL_TREE)
  {
    DJL_xml_open_tag(xdi, indent_level++, catch_types);
    DJL_xml_output_expression(xdi, CATCH_TYPES(t), indent_level);
    DJL_xml_close_tag(xdi, --indent_level, catch_types);
  }
  DJL_xml_open_tag(xdi, indent_level++, catch_body);
  DJL_xml_output_expression(xdi, CATCH_BODY(t), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, catch_body);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_cast_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Cast_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  /* break recursion */
  if (TREE_CODE(t) == CONVERT_EXPR || TREE_CODE(t) == FLOAT_EXPR) {
    DJL_xml_output_type(xdi, TREE_TYPE(t), indent_level);
    DJL_xml_output_cast_expr(xdi, TREE_OPERAND(t, 0), indent_level);
  } else {
    DJL_xml_output_unary_expr(xdi, t, indent_level);
  }
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_ceil_div_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Ceil_Div_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_ceil_mod_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Ceil_Mod_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_cleanup_point_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Cleanup_Point_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_cleanup_stmt(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Cleanup_Stmt";
  DJL_xml_output_single_child_node(xdi, t, indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_complex_cst(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Complex_Cst";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_single_child_node(xdi, TREE_REALPART(t), indent_level, "Real_Part", &DJL_xml_output_expression);
  DJL_xml_output_single_child_node(xdi, TREE_IMAGPART(t), indent_level, "Imag_Part", &DJL_xml_output_expression);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_complex_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Complex_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, "Real_Part", &DJL_xml_output_expression);
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 1), indent_level, "Imag_Part", &DJL_xml_output_expression);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_component_ref(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Component_Ref";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_field_decl(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_compound_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Compound_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_compound_literal_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Compound_Literal_Expr";
  DJL_xml_output_single_child_node(xdi, COMPOUND_LITERAL_EXPR_DECL_STMT(t), indent_level, tag_name, &DJL_xml_output_statement);
}

static void DJL_xml_output_statement_list(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Statement_List";
  tree_stmt_iterator ll;
  DJL_xml_open_tag_with_location(xdi, indent_level++, tag_name,t);
  //Here we get the head of the ll
  ll=tsi_start(t); 
  while(!tsi_end_p(ll)){
    DJL_xml_output_expression(xdi, tsi_stmt(ll), indent_level);
    tsi_next(&ll);
  }
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

/*static void DJL_xml_output_compound_stmt(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Compound_Stmt";
  DJL_xml_output_single_child_node(xdi, COMPOUND_BODY(t), indent_level, tag_name, &DJL_xml_output_statement);
}*/

static void DJL_xml_output_cond_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Cond_Expr";
  DJL_xml_open_tag_with_location(xdi, indent_level++, tag_name, t);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 2), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_condition(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Condititon";
  DJL_xml_output_single_child_node(xdi, t, indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_conj_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Conj_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_const_decl(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Const_Decl";
  int id = xml_add_node(xdi, t, 1);
  DJL_xml_indent(xdi, indent_level++);
  fprintf(xdi->file, "<%s:%s id=\"_%d\" name=\"",
    DJL_xml_ns, tag_name, id);
  DJL_xml_output_decl_name(xdi,t);
  fprintf(xdi->file, "\">\n");
  DJL_xml_output_integer_cst(xdi, DECL_INITIAL(t), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_constructor(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Constructor";
  char *type;
  if (TREE_TYPE(t) == RECORD_TYPE) type = "RECORD_TYPE"; 
  else if (TREE_TYPE(t) == UNION_TYPE) type = "UNION_TYPE";
  else if (TREE_TYPE(t) == ARRAY_TYPE) type = "ARRAY_TYPE";
  else type = "UNKNOWN";

  DJL_xml_indent(xdi, indent_level++);
  fprintf(xdi->file, "<%s:%s type=\"%s\">\n", DJL_xml_ns, tag_name, type);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_continue_stmt(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Continue_Stmt";
  DJL_xml_empty_tag(xdi, indent_level, tag_name);
}

static void DJL_xml_output_convert_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Convert_Expr";
  DJL_xml_indent(xdi, indent_level++);
  fprintf(xdi->file, "<%s:%s precision=\"%d\" type=\"",
    DJL_xml_ns, tag_name, TYPE_PRECISION (TREE_TYPE (t)));
  DJL_xml_output_decl_name(xdi,TYPE_NAME(TREE_TYPE(t)));
  fprintf(xdi->file, "\">\n");
  //DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
  //DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

/*static void DJL_xml_output_decl_stmt(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Decl_Stmt";
  DJL_xml_output_single_child_node(xdi, DECL_STMT_DECL(t), indent_level, tag_name, &DJL_xml_output_expression);
}*/

static void DJL_xml_output_do_stmt(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Do_Stmt";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_body(xdi, DO_BODY(t), indent_level);
  DJL_xml_output_condition(xdi, DO_COND(t), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_empty_class_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Empty_Class_Expr";
  DJL_xml_empty_tag(xdi, indent_level, tag_name);
}

static void DJL_xml_output_eq_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Eq_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_exact_div_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Exact_Div_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_exit_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Exit_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_condition);
}

static void DJL_xml_output_expr_stmt(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Expr_Stmt";
  DJL_xml_output_single_child_node(xdi, EXPR_STMT_EXPR(t), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_expression(xml_dump_info_p xdi, tree t, int indent_level) {
  switch (TREE_CODE (t)) {

  /* constants */
  case COMPLEX_CST:
    DJL_xml_output_complex_cst(xdi, t, indent_level);
    break;
  case INTEGER_CST:
    DJL_xml_output_integer_cst(xdi, t, indent_level);
    break;
  case REAL_CST:
    DJL_xml_output_real_cst(xdi, t, indent_level);
    break;
  case STRING_CST:
    DJL_xml_output_string_cst(xdi, t, indent_level);
    break;
  case VECTOR_CST:
    DJL_xml_output_vector_cst(xdi, t, indent_level);
    break;

  /* declarations */
  case CONST_DECL:
    DJL_xml_output_const_decl(xdi, t, indent_level);
    break;
  case FIELD_DECL:
    DJL_xml_output_field_decl(xdi, t, indent_level);
    break;
  case FUNCTION_DECL:
    DJL_xml_output_function_decl(xdi, t, indent_level);
    break;
  case LABEL_EXPR:
    DJL_xml_output_label_expr(xdi, t, indent_level);
    break;
  case LABEL_DECL:
    DJL_xml_output_label_decl(xdi, t, indent_level);
    break;
  case PARM_DECL:
    DJL_xml_output_parm_decl(xdi, t, indent_level);
    break;
  case RESULT_DECL:
    DJL_xml_output_result_decl(xdi, t, indent_level);
    break;
  case VAR_DECL:
    DJL_xml_output_var_decl(xdi, t, indent_level);
    break;

  /* expressions */
  case ABS_EXPR:
    DJL_xml_output_abs_expr(xdi, t, indent_level);
    break;
  case ADDR_EXPR:
    DJL_xml_output_addr_expr(xdi, t, indent_level);
    break;
  case AGGR_INIT_EXPR:
    DJL_xml_output_aggr_init_expr(xdi, t, indent_level);
    break;
  case ALIGNOF_EXPR:
    /* alignof is not documented in the gcc internals */
    DJL_xml_output_(xdi, t, indent_level);
    break;
  case ARROW_EXPR:
    /* arrow_expr is not documented in the gcc internals */
    DJL_xml_output_(xdi, t, indent_level);
    break;
  case BIND_EXPR:
    DJL_xml_output_bind_expr(xdi, t, indent_level);
    break;
  case BIT_AND_EXPR:
    DJL_xml_output_bit_and_expr(xdi, t, indent_level);
    break;
  case BIT_IOR_EXPR:
    DJL_xml_output_bit_ior_expr(xdi, t, indent_level);
    break;
  case BIT_FIELD_REF:
    DJL_xml_output_bit_field_ref(xdi, t, indent_level);
    break;
  case BIT_NOT_EXPR:
    DJL_xml_output_bit_not_expr(xdi, t, indent_level);
    break;
  case BIT_XOR_EXPR:
    DJL_xml_output_bit_xor_expr(xdi, t, indent_level);
    break;
  case CALL_EXPR:
    DJL_xml_output_call_expr(xdi, t, indent_level);
    break;
  case CEIL_DIV_EXPR:
    DJL_xml_output_ceil_div_expr(xdi, t, indent_level);
    break;
  case CEIL_MOD_EXPR:
    DJL_xml_output_ceil_mod_expr(xdi, t, indent_level);
    break;
  case CLEANUP_POINT_EXPR:
    DJL_xml_output_cleanup_point_expr(xdi, t, indent_level);
    break;
  case COMPLEX_EXPR:
    DJL_xml_output_complex_expr(xdi, t, indent_level);
    break;
  case COMPONENT_REF:
    DJL_xml_output_component_ref(xdi, t, indent_level);
    break;
  case COMPOUND_EXPR:
    DJL_xml_output_compound_expr(xdi, t, indent_level);
    break;
  case COMPOUND_LITERAL_EXPR:
    DJL_xml_output_compound_literal_expr(xdi, t, indent_level);
    break;
  case COND_EXPR:
    DJL_xml_output_cond_expr(xdi, t, indent_level);
    break;
  case CONJ_EXPR:
    DJL_xml_output_conj_expr(xdi, t, indent_level);
    break;
  case CONSTRUCTOR:
    DJL_xml_output_constructor(xdi, t, indent_level);
    break;
  case CONVERT_EXPR:
    DJL_xml_output_convert_expr(xdi, t, indent_level);
    break;
  case EQ_EXPR:
    DJL_xml_output_eq_expr(xdi, t, indent_level);
    break;
  case EXACT_DIV_EXPR:
    DJL_xml_output_exact_div_expr(xdi, t, indent_level);
    break;
  case EXIT_EXPR:
    DJL_xml_output_exit_expr(xdi, t, indent_level);
    break;
  //case FFS_EXPR:
  //  /* ffs_expr is not documented in the gcc internals */
  //  DJL_xml_output_(xdi, t, indent_level);
  //  break;
  case FIX_TRUNC_EXPR:
    DJL_xml_output_fix_trunc_expr(xdi, t, indent_level);
    break;
  case FLOAT_EXPR:
    DJL_xml_output_float_expr(xdi, t, indent_level);
    break;
  case FLOOR_DIV_EXPR:
    DJL_xml_output_floor_div_expr(xdi, t, indent_level);
    break;
  case FLOOR_MOD_EXPR:
    DJL_xml_output_floor_mod_expr(xdi, t, indent_level);
    break;
  case GE_EXPR:
    DJL_xml_output_ge_expr(xdi, t, indent_level);
    break;
  case GT_EXPR:
    DJL_xml_output_gt_expr(xdi, t, indent_level);
    break;
  case IMAGPART_EXPR:
    DJL_xml_output_imagpart_expr(xdi, t, indent_level);
    break;
  case INIT_EXPR:
    DJL_xml_output_init_expr(xdi, t, indent_level);
   break;
  case LE_EXPR:
    DJL_xml_output_le_expr(xdi, t, indent_level);
    break;
  case LOOP_EXPR:
    DJL_xml_output_loop_expr(xdi, t, indent_level);
    break;
  case LSHIFT_EXPR:
    DJL_xml_output_lshift_expr(xdi, t, indent_level);
    break;
  case LT_EXPR:
    DJL_xml_output_lt_expr(xdi, t, indent_level);
    break;
  case MAX_EXPR:
    DJL_xml_output_max_expr(xdi, t, indent_level);
    break;
  case MIN_EXPR:
    DJL_xml_output_min_expr(xdi, t, indent_level);
    break;
  case MINUS_EXPR:
    DJL_xml_output_minus_expr(xdi, t, indent_level);
    break;
  case MODIFY_EXPR:
    DJL_xml_output_modify_expr(xdi, t, indent_level);
    break;
  case MULT_EXPR:
    DJL_xml_output_mult_expr(xdi, t, indent_level);
    break;
  case NE_EXPR:
    DJL_xml_output_ne_expr(xdi, t, indent_level);
    break;
  case NEGATE_EXPR:
    DJL_xml_output_negate_expr(xdi, t, indent_level);
    break;
  case NON_LVALUE_EXPR:
    DJL_xml_output_non_lvalue_expr(xdi, t, indent_level);
    break;
  case NOP_EXPR:
    DJL_xml_output_nop_expr(xdi, t, indent_level);
    break;
  case ORDERED_EXPR:
    DJL_xml_output_ordered_expr(xdi, t, indent_level);
    break;
  case OBJ_TYPE_REF:
    DJL_xml_output_obj_type_ref(xdi, t, indent_level);
	break;
  case PLUS_EXPR:
    DJL_xml_output_plus_expr(xdi, t, indent_level);
    break;
  case POSTDECREMENT_EXPR:
    DJL_xml_output_postdecrement_expr(xdi, t, indent_level);
    break;
  case POSTINCREMENT_EXPR:
    DJL_xml_output_postincrement_expr(xdi, t, indent_level);
    break;
  case PREDECREMENT_EXPR:
    DJL_xml_output_predecrement_expr(xdi, t, indent_level);
    break;
  case PREINCREMENT_EXPR:
    DJL_xml_output_preincrement_expr(xdi, t, indent_level);
    break;
  case RDIV_EXPR:
    DJL_xml_output_rdiv_expr(xdi, t, indent_level);
    break;
  case REALPART_EXPR:
    DJL_xml_output_realpart_expr(xdi, t, indent_level);
    break;
  case ROUND_DIV_EXPR:
    DJL_xml_output_round_div_expr(xdi, t, indent_level);
    break;
  case ROUND_MOD_EXPR:
    DJL_xml_output_round_mod_expr(xdi, t, indent_level);
    break;
  case RSHIFT_EXPR:
    DJL_xml_output_rshift_expr(xdi, t, indent_level);
    break;
  case SAVE_EXPR:
    DJL_xml_output_save_expr(xdi, t, indent_level);
    break;
  case SIZEOF_EXPR:
    /* sizeof_expr is not documented in the gcc internals */
    DJL_xml_output_(xdi, t, indent_level);
    break;
  case STMT_EXPR:
    DJL_xml_output_stmt_expr(xdi, t, indent_level);
    break;
  case TARGET_EXPR:
    DJL_xml_output_target_expr(xdi, t, indent_level);
    break;
  case THROW_EXPR:
    DJL_xml_output_throw_expr(xdi, t, indent_level);
    break;
  case TRUNC_DIV_EXPR:
    DJL_xml_output_trunc_div_expr(xdi, t, indent_level);
    break;
  case TRUNC_MOD_EXPR:
    DJL_xml_output_trunc_mod_expr(xdi, t, indent_level);
    break;
  case TRUTH_AND_EXPR:
    DJL_xml_output_truth_and_expr(xdi, t, indent_level);
    break;
  case TRUTH_ANDIF_EXPR:
    DJL_xml_output_truth_andif_expr(xdi, t, indent_level);
    break;
  case TRUTH_NOT_EXPR:
    DJL_xml_output_truth_not_expr(xdi, t, indent_level);
    break;
  case TRUTH_OR_EXPR:
    DJL_xml_output_truth_or_expr(xdi, t, indent_level);
    break;
  case TRUTH_ORIF_EXPR:
    DJL_xml_output_truth_orif_expr(xdi, t, indent_level);
    break;
  case TRUTH_XOR_EXPR:
    DJL_xml_output_truth_xor_expr(xdi, t, indent_level);
    break;
  case UNEQ_EXPR:
    DJL_xml_output_uneq_expr(xdi, t, indent_level);
    break;
  case UNGE_EXPR:
    DJL_xml_output_unge_expr(xdi, t, indent_level);
    break;
  case UNGT_EXPR:
    DJL_xml_output_ungt_expr(xdi, t, indent_level);
    break;
  case UNLE_EXPR:
    DJL_xml_output_unle_expr(xdi, t, indent_level);
    break;
  case UNLT_EXPR:
    DJL_xml_output_unlt_expr(xdi, t, indent_level);
    break;
  case UNORDERED_EXPR:
    DJL_xml_output_unordered_expr(xdi, t, indent_level);
    break;
  case VA_ARG_EXPR:
    DJL_xml_output_va_arg_expr(xdi, t, indent_level);
    break;

  /* other stuff */
  case ARRAY_RANGE_REF:
    DJL_xml_output_array_range_ref(xdi, t, indent_level);
    break;
  case ARRAY_REF:
    DJL_xml_output_array_ref(xdi, t, indent_level);
    break;
  case ERROR_MARK:
    /* error_mark is not documented in the gcc internals */
    DJL_xml_output_(xdi, t, indent_level);
    break;
  case INDIRECT_REF:
    DJL_xml_output_indirect_ref(xdi, t, indent_level);
    break;
  case RETURN_EXPR:
    DJL_xml_output_return_expr(xdi, t, indent_level);
    break;
  case GOTO_EXPR:
    DJL_xml_output_goto_expr(xdi, t, indent_level);
    break;
  case ASM_EXPR:
    DJL_xml_output_asm_expr(xdi, t, indent_level);
    break;
  case SWITCH_EXPR:
    DJL_xml_output_switch_stmt(xdi, t, indent_level);
    break;
  case CASE_LABEL_EXPR:
    DJL_xml_output_case_label_expr(xdi, t, indent_level);
    break;
  case EXC_PTR_EXPR:
    DJL_xml_output_exc_ptr_expr(xdi, t, indent_level);
    break;
  default:
    //DJL_xml_comment_short(xdi, indent_level, "Unhandled case in DJL_xml_output_expression");
    //DJL_xml_output_(xdi, t, indent_level);
    DJL_xml_output_statement(xdi, t, indent_level);
  }
}

static void DJL_xml_output_field_decl(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Field_Decl";
  int id = xml_add_node(xdi, t, 1);
  DJL_xml_indent(xdi, indent_level);
  fprintf(xdi->file, "<%s:%s id=\"_%d\" name=\"",
    DJL_xml_ns, tag_name, id);
  DJL_xml_output_decl_name(xdi,t);
  fprintf(xdi->file,"\" />\n");
}

static void DJL_xml_output_fix_trunc_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Fix_Trunc_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_float_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Float_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_floor_div_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Floor_Div_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_floor_mod_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Floor_Mod_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_for_stmt(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "For_Stmt";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  if(FOR_INIT_STMT(t))
    DJL_xml_output_single_child_node(xdi, FOR_INIT_STMT(t), indent_level, "For_Init_Stmt", &DJL_xml_output_statement);
  if(FOR_COND(t)){
    //char *tag_name_child = "For_Stmt";
    //DJL_xml_open_tag(xdi, indent_level++, tag_name_child);
    DJL_xml_output_condition(xdi, FOR_COND(t), indent_level);
    //DJL_xml_close_tag(xdi, --indent_level, tag_name_child);
  }
  if(FOR_EXPR(t))
    DJL_xml_output_single_child_node(xdi, FOR_EXPR(t), indent_level, "For_Init_Expr", &DJL_xml_output_expression);
  if(FOR_BODY(t))
    DJL_xml_output_single_child_node(xdi, FOR_BODY(t), indent_level, "For_Body", &DJL_xml_output_statement);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_function_decl(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Function_Decl";
  int id = xml_add_node(xdi, t, 1);
  DJL_xml_indent(xdi, indent_level);
  fprintf(xdi->file, "<%s:%s id=\"_%d\" name=\"",
    DJL_xml_ns, tag_name, id);
  DJL_xml_output_decl_name(xdi,t);
  fprintf(xdi->file, "\" />\n");
}

static void DJL_xml_output_ge_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Ge_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_gt_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Gt_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_goto_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Goto_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t,0), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_identifier(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Identifier";
  DJL_xml_indent(xdi, indent_level);
  fprintf(xdi->file, "<%s:%s>%s</%s:%s>\n",
    DJL_xml_ns, tag_name, IDENTIFIER_POINTER(t), DJL_xml_ns, tag_name);
}

static void DJL_xml_output_if_stmt(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "If_Stmt";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_condition(xdi, IF_COND(t), indent_level);
  /* then */
  DJL_xml_output_single_child_node(xdi, THEN_CLAUSE(t), indent_level, "Then_Clause", &DJL_xml_output_statement);
  /* else */
  if (ELSE_CLAUSE (t))
    DJL_xml_output_single_child_node(xdi, ELSE_CLAUSE(t), indent_level, "Else_Clause", &DJL_xml_output_statement);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_indirect_ref(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Indirect_Ref";
  //int k=0;
  int id = 0;
  //int id2 = -1;
  //tree type;
  //type = DECL_ORIGINAL_TYPE (TREE_TYPE(t));
  /*printf("Our tree code is %s TREE_TYPE=%s\n", tree_code_name[TREE_CODE (t)] , tree_code_name[TREE_CODE(TREE_TYPE(t))]);
  printf("Typename = %s\n", tree_code_name[TREE_CODE(DECL_NAME(TYPE_NAME(TREE_TYPE(t))))]);
  //if(type)printf("Found a type called %s\n", tree_code_name[TREE_CODE(type)]);
  //k = TYPE_NAME(TREE_TYPE(t));
  //printf("Our type of tree code is %s\n", tree_code_name[TREE_CODE (TREE_TYPE(t))] );
  if(TREE_CODE(TREE_TYPE(t)) == RECORD_TYPE)
    id = xml_add_node(xdi, TYPE_NAME(TREE_TYPE(t)), 1);
  else
    id = xml_add_node_real(xdi, TYPE_NAME(TREE_TYPE(t)), 1);
    
  //  id = xml_add_typedef(xdi, t, 1); 
  if(!id){
    id2 = xml_add_node(xdi, TREE_TYPE(t), 1);
  }
  printf("%s -> id = %d, id2=%d k=%d type=%s\n", tag_name, id, id2, k, DJL_gcc_get_decl_name(TYPE_NAME(TREE_TYPE(t))));
  */

  DJL_xml_indent(xdi, indent_level++);
  fprintf(xdi->file, "<%s:%s ", 
    DJL_xml_ns, tag_name);
  if(TREE_CODE(TREE_TYPE(t)) == RECORD_TYPE){
    id = xml_add_node(xdi, TYPE_NAME(TREE_TYPE(t)), 1);
    fprintf(xdi->file, "type=\"_%d\" ", id);
  }else{
    fprintf(xdi->file, "typestring=\"");
	DJL_xml_output_decl_name(xdi,TYPE_NAME(TREE_TYPE(t)));
	fprintf(xdi->file, "\" ");
  }
  //This would be nice for all routines...
  //xml_print_location_attribute (xdi, t);
  fprintf(xdi->file, ">\n");
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_integer_cst(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Integer_Cst";
  long val = ((TREE_INT_CST_HIGH (t) << HOST_BITS_PER_WIDE_INT) + TREE_INT_CST_LOW (t));
  DJL_xml_indent(xdi, indent_level);
  fprintf(xdi->file, "<%s:%s>%d</%s:%s>\n", DJL_xml_ns, tag_name, val, DJL_xml_ns, tag_name);
}

static void DJL_xml_output_imagpart_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Imagpart_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_init_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Init_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_handler(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Handler";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_comment_short(xdi, indent_level, "(Handler) Not implemented yet!");
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_label_decl(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Label_Decl";
  DJL_xml_indent(xdi, indent_level);
  if(DECL_NAME(t))
    fprintf(xdi->file, "<%s:%s name=\"%d\" />\n", DJL_xml_ns, tag_name, IDENTIFIER_POINTER(DECL_NAME(t)));
  else
    fprintf(xdi->file, "<%s:%s name=\"%d\" />\n", DJL_xml_ns, tag_name, DECL_UID(t));//LABEL_DECL_UID(t));//, IDENTIFIER_POINTER(DECL_NAME(t)));
}

static void DJL_xml_output_label_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Label_Expr";
  tree op;
  op = TREE_OPERAND (t, 0);
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
  /* If this is for break or continue, don't bother printing it.  */
  /*if (DECL_NAME (op)){
    DJL_xml_indent(xdi, indent_level);
    fprintf(xdi->file, "<%s:%s name=\"%s\" />\n", DJL_xml_ns, tag_name, IDENTIFIER_POINTER(DECL_NAME(op)));
  }else{
    fprintf(xdi->file, "<%s:%s name=\"FIXME!!!\" />\n", DJL_xml_ns, tag_name);//, IDENTIFIER_POINTER(DECL_NAME(t)));
  }*/
   // printf("skipping unnamed label!!!\n");  
}

/*static void DJL_xml_output_label_stmt(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Label_Stmt";
  DJL_xml_output_single_child_node(xdi, LABEL_STMT_LABEL(t), indent_level, tag_name, &DJL_xml_output_label_decl);
}*/

static void DJL_xml_output_le_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Le_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_loop_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Loop_Expr";
  DJL_xml_output_single_child_node(xdi, LOOP_EXPR_BODY(t), indent_level, tag_name, &DJL_xml_output_body);
}

static void DJL_xml_output_lshift_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Lshift_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_lt_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Lt_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_max_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Max_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_min_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Min_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_minus_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Minus_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_modify_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Modify_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_mult_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Mult_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_namespace_decl(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Namespace_Decl";
  int id = xml_add_node(xdi, t, 1);
  DJL_xml_indent(xdi, indent_level);
  fprintf(xdi->file, "<%s:%s id =\"_%d\" name=\"",
    DJL_xml_ns, tag_name, id);
  DJL_xml_output_decl_name(xdi,t);
  fprintf(xdi->file, "\" />\n");
}

static void DJL_xml_output_ne_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Ne_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_negate_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Negate_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_non_lvalue_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Non_Lvalue_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_nop_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Nop_Expr";
  /*fprintf(xdi->file, "<%s:%s id=\"_%d\" name=\"%s\" />\n",
    DJL_xml_ns, tag_name, id, DJL_gcc_get_decl_name(t));*/
  //DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
  DJL_xml_indent(xdi, indent_level++);
  fprintf(xdi->file, "<%s:%s precision=\"%d\" type=\"",
    DJL_xml_ns, tag_name, TYPE_PRECISION (TREE_TYPE (t)));
  DJL_xml_output_decl_name(xdi,TYPE_NAME(TREE_TYPE(t)));
  fprintf(xdi->file, "\">\n");
  //DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_obj_type_ref(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Obj_Type_Ref";
  char *expr_tag_name = "Obj_Type_Ref_Expr";
  char *obj_tag_name = "Obj_Type_Ref_Object";
  char *token_tag_name = "Obj_Type_Ref_Token";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_open_tag(xdi, indent_level++, expr_tag_name);
  DJL_xml_output_expression(xdi, OBJ_TYPE_REF_EXPR(t), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, expr_tag_name);
  DJL_xml_open_tag(xdi, indent_level++, obj_tag_name);
  DJL_xml_output_expression(xdi, OBJ_TYPE_REF_OBJECT(t), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, obj_tag_name);
  DJL_xml_open_tag(xdi, indent_level++, token_tag_name);
  DJL_xml_output_expression(xdi, OBJ_TYPE_REF_TOKEN(t), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, token_tag_name);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_ordered_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Ordered_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_parm_decl(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Parm_Decl";
  DJL_xml_indent(xdi, indent_level);
  fprintf(xdi->file, "<%s:%s name=\"", DJL_xml_ns, tag_name);
  DJL_xml_output_decl_name(xdi,t);
  fprintf(xdi->file, "\" />\n");
}

static void DJL_xml_output_plus_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Plus_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_postdecrement_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Postdecrement_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_postincrement_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Postincrement_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_predecrement_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Predecrement_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_preincrement_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Preincrement_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_primary_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Primary_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_comment_short(xdi, indent_level, "(Primary_Expr) Not implemented yet!");
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_ptrmem_cst(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Ptrmem_Cst";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_comment_short(xdi, indent_level, "(Ptrmem_Cst) Not implemented yet!");
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_real_cst(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Real_Cst";
  REAL_VALUE_TYPE d;
  char string[64];

  if (TREE_OVERFLOW (t))
    strcpy(string, "overflow");
  d = TREE_REAL_CST (t);
  if (REAL_VALUE_ISINF (d))
    strcpy(string,"Inf");
  else if (REAL_VALUE_ISNAN (d))
    strcpy(string,"Nan");
  else{
    real_to_decimal (string, &d, sizeof (string), 0, 1);
  }
  /*DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_comment_short(xdi, indent_level, "Sorry, but the GCC internal docs wouldn't tell, how to do that :-(");
  DJL_xml_close_tag(xdi, --indent_level, tag_name);*/
  
  //As soon as http://gcc.gnu.org/onlinedocs/gccint/Expression-trees.html decides to
  //put some information on their page, we can implement that.

  DJL_xml_indent(xdi, indent_level);
  fprintf(xdi->file, "<%s:%s>%s</%s:%s>\n",
    DJL_xml_ns, tag_name, string, DJL_xml_ns, tag_name);
  
}

static void DJL_xml_output_realpart_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Realpart_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_rdiv_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Rdiv_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_result_decl(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Result_Decl";
  DJL_xml_empty_tag(xdi, indent_level, tag_name);
}

static void DJL_xml_output_return_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Return_Stmt";
  /* is there an expression to be returned? */
  if (TREE_OPERAND(t,0) != NULL_TREE)
    DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t,0), indent_level++, tag_name, &DJL_xml_output_expression);
  else
    /* a simple: return ; */
    DJL_xml_empty_tag(xdi, indent_level, tag_name);
}

static void DJL_xml_output_round_div_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Round_Div_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_round_mod_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Round_Mod_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_rshift_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Rshift_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_save_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Save_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

/* please note that this function may change the indent level! */
/*static void DJL_xml_output_scope_stmt(xml_dump_info_p xdi, tree t, int *indent_level) {
  char *tag_name = "Scope_Stmt";
  if (SCOPE_NULLIFIED_P(t)) return;
  if (!SCOPE_NO_CLEANUPS_P(t)) return;
  if (SCOPE_BEGIN_P(t)) DJL_xml_open_tag(xdi, (*indent_level)++, tag_name);
  else if (SCOPE_END_P(t)) DJL_xml_close_tag(xdi, --(*indent_level), tag_name);
  else DJL_xml_comment_short(xdi, *indent_level, "Exceptional stuff in DJL_xml_output_scope!");
}*/

static void DJL_xml_output_statement(xml_dump_info_p xdi, tree t, int indent_level) {
  for ( ; t ; (t) = TREE_CHAIN (t)) {
    switch (TREE_CODE (t)) {
//    case ASM_STMT:
//      DJL_xml_output_asm_stmt(xdi, t, indent_level);
//      break;
    case BREAK_STMT:
      DJL_xml_output_break_stmt(xdi, t, indent_level);
      break;
//    case CASE_LABEL:
//      DJL_xml_output_case_label(xdi, t, indent_level);
//      break;
	case CATCH_EXPR:
	  DJL_xml_output_catch_expr(xdi, t, indent_level);
	  break;
    case CLEANUP_STMT:
      DJL_xml_output_cleanup_stmt(xdi, CLEANUP_EXPR(t), indent_level);
      break;
    case STATEMENT_LIST:
      DJL_xml_output_statement_list(xdi, t, indent_level);
      break;
/*    case COMPOUND_STMT:
      DJL_xml_output_compound_stmt(xdi, t, indent_level);
      break;*/
    case CONTINUE_STMT:
      DJL_xml_output_continue_stmt(xdi, t, indent_level);
      break;
/*    case DECL_STMT:
      DJL_xml_output_decl_stmt(xdi, t, indent_level);
      break;*/
    case DO_STMT:
      DJL_xml_output_do_stmt(xdi, t, indent_level);
      break;
    case EMPTY_CLASS_EXPR:
      DJL_xml_output_empty_class_expr(xdi, t, indent_level);
      break;
    case EXPR_STMT:
      DJL_xml_output_expr_stmt(xdi, t, indent_level);
      break;
    case FOR_STMT:
      DJL_xml_output_for_stmt(xdi, t, indent_level);
      break;

    case HANDLER:
      DJL_xml_output_handler(xdi, t, indent_level);
      break;
    case IF_STMT:
      DJL_xml_output_if_stmt(xdi, t, indent_level);
      break;
/*    case LABEL_STMT:
      DJL_xml_output_label_stmt(xdi, t, indent_level);
      break;*/
//    case RETURN_STMT:

//    case SCOPE_STMT:
//      DJL_xml_output_scope_stmt(xdi, t, &indent_level);
//      break;
    case SWITCH_STMT:
      DJL_xml_output_switch_stmt(xdi, t, indent_level);
      break;
    case TRY_BLOCK:
      DJL_xml_output_try_block(xdi, t, indent_level);
      break;
    case TRY_CATCH_EXPR:
      DJL_xml_output_try_catch_expr(xdi, t, indent_level);
  	  break;
	case TRY_FINALLY_EXPR:
      DJL_xml_output_try_finally_expr(xdi, t, indent_level);
  	  break;
    case USING_STMT:
      DJL_xml_output_using_stmt(xdi, t, indent_level);
      break;
    case WHILE_STMT:
      DJL_xml_output_while_stmt(xdi, t, indent_level);
      break;
    default:
      DJL_xml_comment_short(xdi, indent_level, "Unhandled case in DJL_xml_output_statement");
      DJL_xml_output_(xdi, t, indent_level);
    }
  }
}

static void DJL_xml_output_stmt_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Stmt_Expr";
  DJL_xml_output_single_child_node(xdi, STMT_EXPR_STMT(t), indent_level, tag_name, &DJL_xml_output_statement);
}

#define UCHAR(cp)       ((unsigned char *) (cp))
/* escape - reverse transformation */

void escape(char *result, const char *data, size_t len)
{
    int     ch;
    int index=0;  
    while (len-- > 0) {
        ch = *UCHAR(data++);
        if (isprint(ch)) {
            if (ch == '\\')
                result[index++] = ch;
            result[index++] = ch;
            //VSTRING_ADDCH(result, ch);
            continue;
        } else if (ch == '\a') {            /* \a -> audible bell */
            result[index++] = '\\';
            result[index++] = 'a';
            continue;
        } else if (ch == '\b') {            /* \b -> backspace */
            result[index++] = '\\';
            result[index++] = 'b';
            continue;
        } else if (ch == '\f') {            /* \f -> formfeed */
            result[index++] = '\\';
            result[index++] = 'f';
            continue;
        } else if (ch == '\n') {            /* \n -> newline */
            result[index++] = '\\';
            result[index++] = 'n';
            continue;
        } else if (ch == '\r') {            /* \r -> carriagereturn */
            result[index++] = '\\';
            result[index++] = 'r';
            continue;
        } else if (ch == '\t') {            /* \t -> horizontal tab */
            result[index++] = '\\';
            result[index++] = 't';
            continue;
        } else if (ch == '\v') {            /* \v -> vertical tab */
            result[index++] = '\\';
            result[index++] = 'v';
            continue;
        }
        if (isdigit(*UCHAR(data)))
            sprintf(result + index, "\\%03d",ch);
        else
            sprintf(result + index, "\\%d",ch);
    }
    result[index] = 0;
}
static void DJL_xml_output_string_cst(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "String_Cst";
  //FIXME - WPM - There is something wrong with the way this handles inline assembly - FIXME
  char *val = (char *)xcalloc(TREE_STRING_LENGTH(t) + 1, sizeof(char));
  char *newval = (char *) alloca(2*TREE_STRING_LENGTH(t) + 1);
  int i,j=0;

  strncpy(val, TREE_STRING_POINTER(t), TREE_STRING_LENGTH(t));
  /* Known bug here - strings containing a null character (\0) will be truncated.
     We really ought to use memcpy. But even then, the DJL_xml_print_escaped function
     won't cope with \0 characters.
     This is a surprisingly common problem because for wide strings every other byte
     will often be 0, so only the first character will be printed. */

  DJL_xml_indent(xdi, indent_level);
  fprintf(xdi->file, "<%s:%s>",
    DJL_xml_ns, tag_name);
  /* And another bug here. It turns out that the escaping code doesn't escape non-ASCII
     characters which are sometimes found in strings. So that produces invalid XML.
	 Hence temporarily disabling the output of the actual string. 
	 To be reinstated when I have time to fix the algorithm. */
  /* DJL_xml_print_escaped(xdi,val); */
  /* End of disablement */
  fprintf(xdi->file, "</%s:%s>\n",
    DJL_xml_ns, tag_name);
  free(val); val = NULL;
}

static void DJL_xml_output_switch_stmt(xml_dump_info_p xdi, tree t, int indent_level) {
  int i;
  char *tag_name = "Switch_Stmt";
  tree lbls;
//#define SWITCH_LABELS(NODE)     TREE_OPERAND (SWITCH_EXPR_CHECK (NODE), 2)/
  DJL_xml_open_tag_with_location(xdi, indent_level++, tag_name, t);
  DJL_xml_output_condition(xdi, SWITCH_COND(t), indent_level);
  DJL_xml_output_body(xdi, SWITCH_BODY(t), indent_level);
//  DJL_xml_output_body(xdi, SWITCH_LABELSY(t), indent_level);
  //Here we have to output the TREE_VEC which are the switch labels...
  lbls = SWITCH_LABELS(t);
  if(SWITCH_LABELS(t)){
    char * ntag = "Switch_Labels";
    DJL_xml_open_tag(xdi, indent_level++, ntag);
    for(i=0;i<TREE_VEC_LENGTH(lbls);i++){
      //DJL_xml_output_body(xdi, TREE_VEC_ELT(lbls,i), indent_level);
      DJL_xml_output_expression(xdi, TREE_VEC_ELT(lbls,i), indent_level);
      /*DJL_xml_indent(xdi, indent_level);
      fprintf(xdi->file, "<%s:%s id=\"_%d\" />\n",
        DJL_xml_ns, "SWITCH_LABEL", TREE_VEC_ELT(lbls,i));*/
    }
    DJL_xml_close_tag(xdi, --indent_level, ntag);
  }
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_case_label_expr(xml_dump_info_p xdi, tree t, int indent_level){
  char *tag_name = "Case_Label_Expr";
  if(CASE_LOW(t)==NULL_TREE){
    DJL_xml_indent(xdi, indent_level++);
    fprintf(xdi->file, "<%s:%s default=\"1\">\n", DJL_xml_ns, tag_name); 
  }else{
    DJL_xml_open_tag(xdi, indent_level++, tag_name);
    DJL_xml_output_expression(xdi, CASE_LOW(t), indent_level);
    if(CASE_HIGH(t)!= NULL_TREE) //This is used in case this spans multiple 
      DJL_xml_output_expression(xdi, CASE_HIGH(t), indent_level);
  }
  DJL_xml_output_expression(xdi, CASE_LABEL(t), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);  
}

static void DJL_xml_output_exc_ptr_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char* tag_name = "Exc_Ptr_Expr";
  DJL_xml_empty_tag(xdi, indent_level, tag_name);
}

static void DJL_xml_output_target_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Target_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_throw_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Throw_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_trunc_div_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Trunc_Div_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_trunc_mod_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Trunc_Mod_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_truth_and_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Truth_And_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_truth_andif_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Truth_Andif_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_truth_or_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Truth_Or_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_truth_orif_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Truth_Orif_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_truth_not_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Truth_Not_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_truth_xor_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Truth_xor_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}



static void DJL_xml_output_try_block(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Try_Block";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  /* body of try block */
  DJL_xml_output_body(xdi, TRY_STMTS(t), indent_level);
  /* either cleanup code or exception handlers */
  if (CLEANUP_P(t))
    DJL_xml_output_cleanup_stmt(xdi, TRY_HANDLERS(t), indent_level);
  else
    DJL_xml_output_statement(xdi, TRY_HANDLERS(t), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_try_catch_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char* tag_name = "Try_Catch_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_try_finally_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char* tag_name = "Try_Finally_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_type(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Type";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_comment_short(xdi, indent_level, "(Type) Not implemented yet!");
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_unary_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Unary_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_comment_short(xdi, indent_level, "(Unary_Expr) Not implemented yet!");
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_uneq_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Uneq_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_unge_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Unge_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_ungt_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Ungt_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_unle_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Unle_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_unlt_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Unlt_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_unordered_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Unordered_Expr";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 0), indent_level);
  DJL_xml_output_expression(xdi, TREE_OPERAND(t, 1), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_using_stmt(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Using_Stmt";
  DJL_xml_output_single_child_node(xdi, USING_STMT_NAMESPACE(t), indent_level, tag_name, &DJL_xml_output_namespace_decl);
}

static void DJL_xml_output_va_arg_expr(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Va_Arg_Expr";
  DJL_xml_output_single_child_node(xdi, TREE_OPERAND(t, 0), indent_level, tag_name, &DJL_xml_output_expression);
}

static void DJL_xml_output_var_decl(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Var_Decl";
  //FIXME - WPM - Clean up code!!!
  //printf("VAR_DECL\n");
  int id = xml_add_node(xdi, t, 1);
  //printf("VAR_DECL1\n");
  DJL_xml_indent(xdi, indent_level);
  //printf("VAR_DECL2\n");
  /*printf("<%s:%s id=\"_%d\" name=\"0x???\" />\n",
    DJL_xml_ns, tag_name, id);*/
  //char * temp = DJL_gcc_get_decl_name(t);
  fprintf(xdi->file, "<%s:%s id=\"_%d\" name=\"",
    DJL_xml_ns, tag_name, id);
  DJL_xml_output_decl_name(xdi,t);
  fprintf(xdi->file,"\" />\n");
  //printf("Done with VAR_DECL\n");
}

static void DJL_xml_output_vector_cst(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "Vector_Cst";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  for (t = TREE_VECTOR_CST_ELTS(t) ; t ; t = TREE_CHAIN(t)) {
    DJL_xml_output_expression(xdi, t, indent_level);
  }
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}

static void DJL_xml_output_while_stmt(xml_dump_info_p xdi, tree t, int indent_level) {
  char *tag_name = "While_Stmt";
  DJL_xml_open_tag(xdi, indent_level++, tag_name);
  DJL_xml_output_condition(xdi, WHILE_COND(t), indent_level);
  DJL_xml_output_body(xdi, WHILE_BODY(t), indent_level);
  DJL_xml_close_tag(xdi, --indent_level, tag_name);
}
