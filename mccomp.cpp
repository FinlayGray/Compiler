#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <string.h>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

using namespace llvm;
using namespace llvm::sys;

FILE *pFile;
static bool errorReported = false;

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns one of these for known things.
enum TOKEN_TYPE {

  IDENT = -1,        // [a-zA-Z_][a-zA-Z_0-9]*
  ASSIGN = int('='), // '='

  // delimiters
  LBRA = int('{'),  // left brace
  RBRA = int('}'),  // right brace
  LPAR = int('('),  // left parenthesis
  RPAR = int(')'),  // right parenthesis
  SC = int(';'),    // semicolon
  COMMA = int(','), // comma

  // types
  INT_TOK = -2,   // "int"
  VOID_TOK = -3,  // "void"
  FLOAT_TOK = -4, // "float"
  BOOL_TOK = -5,  // "bool"

  // keywords
  EXTERN = -6,  // "extern"
  IF = -7,      // "if"
  ELSE = -8,    // "else"
  WHILE = -9,   // "while"
  RETURN = -10, // "return"
  // TRUE   = -12,     // "true"
  // FALSE   = -13,     // "false"

  // literals
  INT_LIT = -14,   // [0-9]+
  FLOAT_LIT = -15, // [0-9]+.[0-9]+
  BOOL_LIT = -16,  // "true" or "false" key words

  // logical operators
  AND = -17, // "&&"
  OR = -18,  // "||"

  // operators
  PLUS = int('+'),    // addition or unary plus
  MINUS = int('-'),   // substraction or unary negative
  ASTERIX = int('*'), // multiplication 
  DIV = int('/'),     // division
  MOD = int('%'),     // modular
  NOT = int('!'),     // unary negation

  // comparison operators
  EQ = -19,      // equal
  NE = -20,      // not equal
  LE = -21,      // less than or equal to
  LT = int('<'), // less than
  GE = -23,      // greater than or equal to
  GT = int('>'), // greater than

  // special tokens
  EOF_TOK = 0, // signal end of file

  // invalid
  INVALID = -100 // signal invalid token
};

// TOKEN struct is used to keep track of information about a token
struct TOKEN {
  int type = -100;
  std::string lexeme;
  int lineNo;
  int columnNo;
};

static std::string IdentifierStr; // Filled in if IDENT
static int IntVal;                // Filled in if INT_LIT
static bool BoolVal;              // Filled in if BOOL_LIT
static float FloatVal;            // Filled in if FLOAT_LIT
static std::string StringVal;     // Filled in if String Literal
static int lineNo, columnNo;

static TOKEN returnTok(std::string lexVal, int tok_type) {
  TOKEN return_tok;
  return_tok.lexeme = lexVal;
  return_tok.type = tok_type;
  return_tok.lineNo = lineNo;
  return_tok.columnNo = columnNo - lexVal.length() - 1;
  return return_tok;
}

// Read file line by line -- or look for \n and if found add 1 to line number
// and reset column number to 0
/// gettok - Return the next token from standard input.
static TOKEN gettok() {

  static int LastChar = ' ';
  static int NextChar = ' ';

  // Skip any whitespace.
  while (isspace(LastChar)) {
    if (LastChar == '\n' || LastChar == '\r') {
      lineNo++;
      columnNo = 1;
    }
    LastChar = getc(pFile);
    columnNo++;
  }

  if (isalpha(LastChar) ||
      (LastChar == '_')) { // identifier: [a-zA-Z_][a-zA-Z_0-9]*
    IdentifierStr = LastChar;
    columnNo++;

    while (isalnum((LastChar = getc(pFile))) || (LastChar == '_')) {
      IdentifierStr += LastChar;
      columnNo++;
    }

    if (IdentifierStr == "int")
      return returnTok("int", INT_TOK);
    if (IdentifierStr == "bool")
      return returnTok("bool", BOOL_TOK);
    if (IdentifierStr == "float")
      return returnTok("float", FLOAT_TOK);
    if (IdentifierStr == "void")
      return returnTok("void", VOID_TOK);
    if (IdentifierStr == "bool")
      return returnTok("bool", BOOL_TOK);
    if (IdentifierStr == "extern")
      return returnTok("extern", EXTERN);
    if (IdentifierStr == "if")
      return returnTok("if", IF);
    if (IdentifierStr == "else")
      return returnTok("else", ELSE);
    if (IdentifierStr == "while")
      return returnTok("while", WHILE);
    if (IdentifierStr == "return")
      return returnTok("return", RETURN);
    if (IdentifierStr == "true") {
      BoolVal = true;
      return returnTok("true", BOOL_LIT);
    }
    if (IdentifierStr == "false") {
      BoolVal = false;
      return returnTok("false", BOOL_LIT);
    }

    return returnTok(IdentifierStr.c_str(), IDENT);
  }

  if (LastChar == '=') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // EQ: ==
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("==", EQ);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("=", ASSIGN);
    }
  }

  if (LastChar == '{') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("{", LBRA);
  }
  if (LastChar == '}') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("}", RBRA);
  }
  if (LastChar == '(') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("(", LPAR);
  }
  if (LastChar == ')') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(")", RPAR);
  }
  if (LastChar == ';') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(";", SC);
  }
  if (LastChar == ',') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(",", COMMA);
  }

  if (isdigit(LastChar) || LastChar == '.') { // Number: [0-9]+.
    std::string NumStr;

    if (LastChar == '.') { // Floatingpoint Number: .[0-9]+
      do {
        NumStr += LastChar;
        LastChar = getc(pFile);
        columnNo++;
      } while (isdigit(LastChar));

      FloatVal = strtof(NumStr.c_str(), nullptr);
      return returnTok(NumStr, FLOAT_LIT);
    } else {
      do { // Start of Number: [0-9]+
        NumStr += LastChar;
        LastChar = getc(pFile);
        columnNo++;
      } while (isdigit(LastChar));

      if (LastChar == '.') { // Floatingpoint Number: [0-9]+.[0-9]+)
        do {
          NumStr += LastChar;
          LastChar = getc(pFile);
          columnNo++;
        } while (isdigit(LastChar));

        FloatVal = strtof(NumStr.c_str(), nullptr);
        return returnTok(NumStr, FLOAT_LIT);
      } else { // Integer : [0-9]+
        IntVal = strtod(NumStr.c_str(), nullptr);
        return returnTok(NumStr, INT_LIT);
      }
    }
  }

  if (LastChar == '&') {
    NextChar = getc(pFile);
    if (NextChar == '&') { // AND: &&
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("&&", AND);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("&", int('&'));
    }
  }

  if (LastChar == '|') {
    NextChar = getc(pFile);
    if (NextChar == '|') { // OR: ||
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("||", OR);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("|", int('|'));
    }
  }

  if (LastChar == '!') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // NE: !=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("!=", NE);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("!", NOT);
      ;
    }
  }

  if (LastChar == '<') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // LE: <=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("<=", LE);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("<", LT);
    }
  }

  if (LastChar == '>') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // GE: >=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok(">=", GE);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok(">", GT);
    }
  }

  if (LastChar == '/') { // could be division or could be the start of a comment
    LastChar = getc(pFile);
    columnNo++;
    if (LastChar == '/') { // definitely a comment
      do {
        LastChar = getc(pFile);
        columnNo++;
      } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

      if (LastChar != EOF)
        return gettok();
    } else
      return returnTok("/", DIV);
  }

  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF) {
    columnNo++;
    return returnTok("0", EOF_TOK);
  }

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;
  std::string s(1, ThisChar);
  LastChar = getc(pFile);
  columnNo++;
  return returnTok(s, int(ThisChar));
}

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static TOKEN CurTok;
static std::deque<TOKEN> tok_buffer;

static TOKEN getNextToken() {

  if (tok_buffer.size() == 0)
    tok_buffer.push_back(gettok());

  TOKEN temp = tok_buffer.front();
  tok_buffer.pop_front();

  return CurTok = temp;
}

static void putBackToken(TOKEN tok) { tok_buffer.push_front(tok); }

//===----------------------------------------------------------------------===//
// AST nodes
//===----------------------------------------------------------------------===//

/// ASTnode - Base class for all AST nodes.
class ASTnode {
public:
  virtual ~ASTnode() {}
  virtual Value *codegen() = 0;
  virtual std::string to_string() const {return "";};
};
Recursive Descent Parser - Function call for each production
/// IntASTnode - Class for integer literals like 1, 2, 10,
class IntASTnode : public ASTnode {
  int Val;
  TOKEN Tok;
  std::string Name;

public:
  IntASTnode(TOKEN tok, int val) : Val(val), Tok(tok) {}
  virtual Value *codegen() override;
  // virtual std::string to_string() const override {
  // return a sting representation of this AST node
  //};
};

/* add other AST nodes as nessasary */


//===----------------------------------------------------------------------===//
// First Sets
//===----------------------------------------------------------------------===//

std::vector<TOKEN_TYPE> first_program = {EXTERN,INT_TOK,FLOAT_TOK,BOOL_TOK,VOID_TOK};
std::vector<TOKEN_TYPE> first_arg_listI = {COMMA}; // "," NULLABLE
std::vector<TOKEN_TYPE> first_arg_list = {MINUS, NOT, LPAR, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT}; // -, !, (, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT
std::vector<TOKEN_TYPE> first_args = {MINUS, NOT, LPAR, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT}; // -, !, (, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT, ϵ NULLABLE
std::vector<TOKEN_TYPE> first_rval8 = {LPAR, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT}; // (, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT
std::vector<TOKEN_TYPE> first_rval7_to_rval = {MINUS, NOT, LPAR, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT}; // -, !, (, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT
std::vector<TOKEN_TYPE> first_rval6I = {ASTERIX, DIV, MOD}; // *, /, %, ϵ NULLABLE
std::vector<TOKEN_TYPE> first_rval5I = {PLUS, MINUS}; // +, -, ϵ NULLABLE
std::vector<TOKEN_TYPE> first_rval4I = {LE, LT, GE, GT}; // <=, <, >=, >, ϵ NULLABLE
std::vector<TOKEN_TYPE> first_rval3I = {EQ, NE}; // ==, !=, ϵ NULLABLE
std::vector<TOKEN_TYPE> first_rval2I = {AND}; // &&, ϵ NULLABLE
std::vector<TOKEN_TYPE> first_rvalI = {OR}; // ||, ϵ NULLABLE
std::vector<TOKEN_TYPE> first_expr = {MINUS, NOT, LPAR, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT}; // -, !, (, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT
std::vector<TOKEN_TYPE> first_return_stmt = {RETURN}; // "return"
std::vector<TOKEN_TYPE> first_else_stmt = {ELSE}; // "else", ϵ NULLABLE
std::vector<TOKEN_TYPE> first_if_stmt = {IF}; // "if"
std::vector<TOKEN_TYPE> first_while_stmt = {WHILE}; // "while"
std::vector<TOKEN_TYPE> first_expr_stmt = {MINUS, NOT, LPAR, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT, SC}; // -, !, (, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT, ;
std::vector<TOKEN_TYPE> first_stmt = {MINUS, NOT, LPAR, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT, SC, LBRA, IF, WHILE, RETURN}; // -, !, (, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT, ;, {, "if", "while", "return"
std::vector<TOKEN_TYPE> first_stmt_list = {MINUS, NOT, LPAR, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT, SC, LBRA, IF, WHILE, RETURN}; // -, !, (, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT, ;, {, "if", "while", "return", ϵ NULLABLE
std::vector<TOKEN_TYPE> first_var_type = {INT_TOK, FLOAT_TOK, BOOL_TOK}; // "int", "float", "bool"
std::vector<TOKEN_TYPE> first_local_decl = {INT_TOK, FLOAT_TOK, BOOL_TOK}; // "int", "float", "bool"
std::vector<TOKEN_TYPE> first_local_decls = {INT_TOK, FLOAT_TOK, BOOL_TOK}; // "int", "float", "bool", ϵ NULLABLE
std::vector<TOKEN_TYPE> first_block = {LBRA}; // "{"
std::vector<TOKEN_TYPE> first_param = {INT_TOK, FLOAT_TOK, BOOL_TOK}; // "int", "float", "bool"
std::vector<TOKEN_TYPE> first_param_list = {INT_TOK, FLOAT_TOK, BOOL_TOK}; // "int", "float", "bool"
std::vector<TOKEN_TYPE> first_param_listI = {COMMA}; // ",", ϵ NULLABLE
std::vector<TOKEN_TYPE> first_params = {INT_TOK, FLOAT_TOK, BOOL_TOK, VOID_TOK}; // "int", "float", "bool", "void", ϵ NULLABLE
std::vector<TOKEN_TYPE> first_type_spec = {INT_TOK, FLOAT_TOK, BOOL_TOK, VOID_TOK}; // "int", "float", "bool", "void"
std::vector<TOKEN_TYPE> first_fun_decl = {INT_TOK, FLOAT_TOK, BOOL_TOK, VOID_TOK}; // "int", "float", "bool", "void"
std::vector<TOKEN_TYPE> first_var_decl = {INT_TOK, FLOAT_TOK, BOOL_TOK}; // "int", "float", "bool"
std::vector<TOKEN_TYPE> first_decl = {INT_TOK, FLOAT_TOK, BOOL_TOK, VOID_TOK}; // "int", "float", "bool", "void"
std::vector<TOKEN_TYPE> first_decl_listI = {INT_TOK, FLOAT_TOK, BOOL_TOK, VOID_TOK}; // "int", "float", "bool", "void", ϵ NULLABLE
std::vector<TOKEN_TYPE> first_decl_list = {INT_TOK, FLOAT_TOK, BOOL_TOK, VOID_TOK}; // "int", "float", "bool", "void"
std::vector<TOKEN_TYPE> first_extern = {EXTERN}; // "extern"
std::vector<TOKEN_TYPE> first_extern_listI = {EXTERN}; // "extern", ϵ NULLABLE
std::vector<TOKEN_TYPE> first_extern_list = {EXTERN}; // "extern"


//===----------------------------------------------------------------------===//
// Follow Sets
//===----------------------------------------------------------------------===//







//===----------------------------------------------------------------------===//
// Recursive Descent Parser - Function call for each production
//===----------------------------------------------------------------------===//

static bool isIn(int type, vector<TOKEN_TYPE> l)
{
  for(int i = 0; i < l.size(); i++)
  {
    if(l[i] == type)
      return true;
  }
  return false;
}

bool match(TOKEN_TYPE tok) {
  if(Curtok.type == tok) {
    getNextToken();
    return true;
  }
  else
    return false;
  }

/* Add function calls for each production */
bool program();
bool extern_list();
bool extern_listI();
bool pas_extern();
bool decl_list();
bool decl_listI();
bool decl();
bool var_decl();
bool var_type();
bool type_spec();
bool fun_decl();
bool params();
bool param_list();
bool param_listI();
bool param();
bool block();
bool local_decls();
bool local_decl();
bool stmt_list();
bool stmt();
bool expr_stmt();
bool while_stmt();
bool if_stmt();
bool else_stmt();
bool return_stmt();
bool expr();
bool rval();
bool ravlI();
bool rval2();
bool rval2I();
bool rval3();
bool rval3I();
bool rval4();
bool rval4I();
bool rval5();
bool rval5I();
bool rval6();
bool rval6I();
bool rval7();
bool rval8();
bool args();
bool arg_list();
bool arg_listI();




// extern externlistI
bool extern_list() {
  return pas_extern() && extern_listI();
}

// extern extern_listI | epsilon
bool extern_listI(){
  if (isIn(CurTok.type, first_extern)){
    return pas_extern() && extern_listI();
  }
  else {
    if (isIn(CurTok.type, Follow_extern_listI)){
      return true;
    }
    else {
      if (!errorReported)
          {errs()<<"Syntax error: Invalid token "<<CurTok.lexeme<<" found at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
      errorReported = true;
      return false;
    }
  }
}
// "extern" type_spec IDENT "(" params ")" ";"
bool pas_extern(){
  if (!match(EXTERN)){
    if(!errorReported)
        {errs()<<"Syntax error: Expected  `extern`  at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return false;
  }
  if (isIn(CurTok.type, first_type_spec)){
    type_spec();
    
  } else {
    if(!errorReported)
        {errs()<<"Syntax error: Invalid type spec token" <<CurTok.lexeme<<" at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
      errorReported = true;
      return false;
  }
  if (!match(IDENT)){
    if(!errorReported)
        {errs()<<"Syntax error: Expected an Identifier at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return false;
  }
  if (!match(LPAR)){
    if(!errorReported)
        {errs()<<"Syntax error: Expected '(' at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return false;
  }
  if (!params()){
      if(!errorReported)
        {errs()<<"Syntax error: Invalid params token" <<CurTok.lexeme<<" at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return false;
  }
  if (!match(RPAR)){
    if(!errorReported)
        {errs()<<"Syntax error: Expected ')' at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return false;
  }
  if (!match(SC)){
    if(!errorReported)
        {errs()<<"Syntax error: Expected ';' at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return false;
  }
  return true;
}

// decl_list ::= decl decl_listI
bool decl_list() {
  if (isIn(CurTok.type, first_decl)){
    return decl() && decl_listI();
  }
}

// decl_listI ::= decl decl_listI | epsilon
bool decl_listI() {
  if (isIn(CurTok.type, first_decl)){
    return decl() && decl_listI();
  }
  else {
    if (isIn(CurTok.type, Follow_decl_listI)){
      return true;
    }
    else {
      if (!errorReported)
          {errs()<<"Syntax error: Invalid decl_listI token "<<CurTok.lexeme<<" found at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
      errorReported = true;
      return false;
    }
  }
}


//decl ::= var_decl |  fun_decl
bool decl() {
  if (isIn(CurTok.type, first_var_decl)){
    return var_decl();
  }
  else {
    if (isIn(CurTok.type, first_fun_decl)){
      return fun_decl();
    }
    else {
      if (!errorReported)
          {errs()<<"Syntax error: Invalid decl token "<<CurTok.lexeme<<" found at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
      errorReported = true;
      return false;
    }
  }
}

//var_decl ::= var_type IDENT ";" 
bool var_decl() {
  if (isIn(CurTok.type, first_var_type)){
    var_type();
  }else {
    if (!errorReported)
          {errs()<<"Syntax error: Expected var_type token at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return false;
  }
  if (!match(IDENT)){
    if(!errorReported)
        {errs()<<"Syntax error: Expected an Identifier at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return false;
  }

  if (!match(SC)){
    if(!errorReported)
        {errs()<<"Syntax error: Expected ';' at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return false;
  }
  return true;
}
//COULD CHANGE 
// to do match void, else push back and match vartype
//type_spec ::= "void" |  var_type     
bool type_spec() {
  if (isIn(CurTok.type, first_type_spec)){
    getNextToken();
    return true;
  }
  else {
    if(!errorReported)
        {errs()<<"Syntax error: Invalid type_spec token at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return false;
  }
}

// var_type  ::= "int" |  "float" |  "bool"
bool var_type() {
  if (match(INT_TOK)){
      return true;
    }
    else {
      if (match(FLOAT_TOK)){
        return true;
      }
      else {
        if (match(BOOL_TOK)){
          return true;
        }
        else {
            if(!errorReported)
                {errs()<<"Syntax error: Expected an Identifier at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
            errorReported = true;
            return false;
        }
      }
    }
}

//fun_decl ::= type_spec IDENT "(" params ")" block
bool fun_decl() {
  if (isIn(CurTok.type, first_type_spec)){
    type_spec();
  }else {
    if(!errorReported)
        {errs()<<"Syntax error: Expected an Identifier at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return false;
  }
  if (!match(IDENT)){
    if(!errorReported)
        {errs()<<"Syntax error: Expected an Identifier at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return false;
  }
  if (!match(LPAR)){
    if(!errorReported)
        {errs()<<"Syntax error: Expected '(' at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return false;
  }
  if (!params()){
      if(!errorReported)
        {errs()<<"Syntax error: Invalid params token" <<CurTok.lexeme<<" at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return false;
  }
  if (!match(RPAR)){
    if(!errorReported)
        {errs()<<"Syntax error: Expected ')' at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return false;
  }
  if (isIn(CurTok.type, first_block)){
    block();
  }else {
    if(!errorReported)
        {errs()<<"Syntax error: Expected block statement starting with '{' at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return false;
  }
  return true;

}

//params ::= param_list | "void" | epsilon
bool params() {
  if (isIn(CurTok.type, first_param_list)){
    return param_list;
  }
  else {
    if (match(VOID_TOK)){
      return true;
    }
    else{
      if (isIn(CurTok.type, Follow_params)){
        return true;
      }
      else{
        if(!errorReported)
          {errs()<<"Syntax error: Invalid params statement at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
        errorReported = true;
        return false;
      }
    }
  }
}

// param_list ::= param param_listI
bool param_list() {
  if (isIn(CurTok.type, first_param)){
    return param() && param_listI();
  }
  else {
    if (!errorReported)
          {errs()<<"Syntax error: Invalid token "<<CurTok.lexeme<<" found at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return false;
  }
}


// program ::= extern_list decl_list
static void parser() {
  // add body
}

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;

//===----------------------------------------------------------------------===//
// AST Printer
//===----------------------------------------------------------------------===//

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                     const ASTnode &ast) {
  os << ast.to_string();
  return os;
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main(int argc, char **argv) {
  if (argc == 2) {
    pFile = fopen(argv[1], "r");
    if (pFile == NULL)
      perror("Error opening file");
  } else {
    std::cout << "Usage: ./code InputFile\n";
    return 1;
  }

  // initialize line number and column numbers to zero
  lineNo = 1;
  columnNo = 1;

  // get the first token
  getNextToken();
  while (CurTok.type != EOF_TOK) {
    fprintf(stderr, "Token: %s with type %d\n", CurTok.lexeme.c_str(),
            CurTok.type);
    getNextToken();
  }
  fprintf(stderr, "Lexer Finished\n");

  // Make the module, which holds all the code.
  TheModule = std::make_unique<Module>("mini-c", TheContext);

  // Run the parser now.
  parser();
  fprintf(stderr, "Parsing Finished\n");

  //********************* Start printing final IR **************************
  // Print out all of the generated code into a file called output.ll
  auto Filename = "output.ll";
  std::error_code EC;
  raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

  if (EC) {
    errs() << "Could not open file: " << EC.message();
    return 1;
  }
  // TheModule->print(errs(), nullptr); // print IR to terminal
  TheModule->print(dest, nullptr);
  //********************* End printing final IR ****************************

  fclose(pFile); // close the file that contains the code that was parsed
  return 0;
}
