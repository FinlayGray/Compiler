# Compiler
program ::= extern_list decl_list 
           | decl_list

extern_list ::= extern extern_listI
extern_listI ::= extern extern_listI | epsilon

extern ::= "extern" type_spec IDENT "(" params ")" ";"

decl_list ::= decl decl_listI
decl_listI ::= decl decl_listI | epsilon
decl ::= var_decl 
      |  fun_decl
var_decl ::= var_type IDENT ";" 
type_spec ::= "void"
            |  var_type           
var_type  ::= "int" |  "float" |  "bool"
fun_decl ::= type_spec IDENT "(" params ")" block
params ::= param_list  
      |  "void" | epsilon

param_list ::= param param_listI
param_listI ::= "," param param_listI | epsilon
param ::= var_type IDENT
block ::= "{" local_decls stmt_list "}"

local_decls ::= local_decl local_decls | epsilon

local_decl ::= var_type IDENT ";"
stmt_list ::= stmt stmt_list 
            |  epsilon
stmt ::= expr_stmt 
      |  block 
      |  if_stmt 
      |  while_stmt 
      |  return_stmt
expr_stmt ::= expr ";" 
            |  ";"
while_stmt ::= "while" "(" expr ")" stmt 
if_stmt ::= "if" "(" expr ")" block else_stmt
else_stmt  ::= "else" block
            |  epsilon
return_stmt ::= "return" ";" 
            |  "return" expr ";"               
# operators in order of increasing precedence      
expr ::= IDENT "=" expr
      | rval


rval ::= rval2 rvalI
ravlI ::= "||" rval2 rvalI | epsilon

rval2 ::= rval3 rval2I
rval2I ::= "&&" rval3 rval2I | epsilon

rval3 ::= rval4 rval3I
rval3I ::= "==" rval4 rval3I | "!=" rval4 rval3I | epsilon

rval4 ::= rval5 rval4I
rval4I ::= "<=" rval5 rval4I | "<" rval5 rval4I | ">=" rval5 rval4I | ">" rval5 rval4I | epsilon

rval5 ::= rval6 rval5I
rval5I ::= "+" rval6 rval5I | "-" rval6 rval5I | epsilon

rval6 ::= rval7 rval6I
rval6I ::= "*" rval7 rval6I | "/" rval7 rval6I | "%" rval7 rval6I | epsilon

rval7 ::= "-" rval8 | "!" rval8 | rval8
rval8 ::= "(" expr ")" | IDENT | IDENT "(" args ")" | INT_LIT | FLOAT_LIT | BOOL_LIT 

args ::= arg_list 
      |  epsilon
arg_list ::= expr arg_listI
arg_listI ::= "," expr arg_listI | epsilon
