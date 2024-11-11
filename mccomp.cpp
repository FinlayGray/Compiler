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

static void clearTokBuffer() {  //clear the buffer at the end
  while(tok_buffer.size() != 0) 
  {
    tok_buffer.pop_front();
  } 
}

//===----------------------------------------------------------------------===//
// AST nodes
//===----------------------------------------------------------------------===//

static int indentDepth = 0;


// void dedent(){
//   indentDepth = (indentDepth > 0) ? indentDepth = indentDepth - indentDepth : indentDepth;
// }
void dedent() {
  if (indentDepth >= 4) indentDepth -= 4;
}


// std::string indent() {
//   indentDepth = indentDepth + 2;
//   std::string out = "";
//   for (int i= 0; i < indentDepth; i++) {
//     out.append(" ");
//   }
//   return out;
// }
// std::string indent() {
//   indentDepth += 4;
//   return std::string(indentDepth, ' ');
// }
std::string indent()
{
  indentDepth += 4;
  std::string out = "";
    for(int i = 0; i < indentDepth; i++)
      if(i % 2 == 0)
        out.append("|");
      else
      {
        if(i == indentDepth - 1)
          out.append("-");
        else
          out.append(" ");
      }
  // else
    // out = std::string(indentLevel,' ');
  return out;
}

std::string startIndent(){
  return std::string(indentDepth, ' ');
}






/// ASTnode - Base class for all AST nodes.
class ASTnode {
public:
  virtual ~ASTnode() {}
  virtual Value *codegen() = 0;
  virtual std::string to_string() const {return "";};
};
// Recursive Descent Parser - Function call for each production
/// IntASTnode - Class for integer literals like 1, 2, 10,


class rootASTnode : public ASTnode {
  std::vector<std::unique_ptr<ASTnode>> TopNodes;

public:
  rootASTnode(std::vector<std::unique_ptr<ASTnode>> topnodes) : TopNodes(std::move(topnodes)) {}
  virtual Value *codegen() override;
  virtual std::string to_string() const override {
    std::string out = "Program: ";
    for (int i = 0; i < TopNodes.size(); i++) {
      out.append("\n" + indent() + TopNodes[i]->to_string());
    }
    dedent();
    return out;
  }
};





class IntASTnode : public ASTnode {
  int Val;


public:
  IntASTnode(int val) : Val(val){}
  virtual Value *codegen() override;
  virtual std::string to_string() const override {
    std::string out = "IntegerLiteral: " + std::to_string(Val);
    dedent();
    return out;
  };
};

/* add other AST nodes as nessasary */

class FloatASTnode : public ASTnode {
  float Val;


public:
  FloatASTnode(float val) : Val(val) {}
  virtual Value *codegen() override;
  virtual std::string to_string() const override {
    std::string out = "FloatLiteral: " + std::to_string(Val);
    dedent();
    return out;
  };

};

class BoolASTnode : public ASTnode {
  bool Val;
  std::string Name;

public:
  BoolASTnode(bool val) : Val(val) {}
  virtual Value *codegen() override;
  virtual std::string to_string() const override {
    std::string out = "BoolLiteral: " + std::to_string(Val);
    dedent();
    return out;
  };

};


class VariableASTnode : public ASTnode {
  std::string Val;
  std::string Type;

public:
  VariableASTnode(std::string type, std::string val) : Val(val), Type(type) {}
  virtual Value *codegen() override;
  const std::string getType() const { return Type; }
  const std::string getName() const { return Val; }
  virtual std::string to_string() const override {
    std::string out = "VarDeclaration: " + Type + " " + Val;
    dedent();
    return out;
  };

};

class GlobalVariableASTnode : public ASTnode {
  std::string Val;
  std::string Type;

public:
  GlobalVariableASTnode(std::string type, std::string val) : Val(val), Type(type) {}
  virtual Value *codegen() override;
  const std::string getType() const { return Type; }
  const std::string getName() const { return Val; }
  virtual std::string to_string() const override {
    std::string out = "GlobalVarDeclaration: " + Type + " " + Val;
    dedent();
    return out;
  };

};


class VariableRefASTnode : public ASTnode{
  std::string Name;

  public:
  VariableRefASTnode(std::string name) : Name(name) {}
  const std::string getName() const { return Name; }
  virtual Value *codegen() override;
  // virtual TOKEN getTok() const override{
  //   return Tok;
  // }
  // std::string getName() const override{ return Name; }
  virtual std::string to_string() const override {
  //return a string representation of this AST node
    std::string out = "VarReference: " + Name;
    dedent();
    return out;
  };
};


class UnaryExprASTnode : public ASTnode {
  std::string Opcode;
  std::unique_ptr<ASTnode> Operand;

public:
  UnaryExprASTnode(std::string opcode, std::unique_ptr<ASTnode> operand) : Opcode(opcode), Operand(std::move(operand)) {}
  virtual Value *codegen() override;
  virtual std::string to_string() const override {
  //return a string representation of this AST node
    std::string out = "UnaryExpr: " + Opcode + "\n" + indent() + Operand->to_string();
    dedent();
    return out;
  };

};

class BinaryExprASTnode : public ASTnode {
  std::string Opcode;
  std::unique_ptr<ASTnode> LHS, RHS;

public:
  BinaryExprASTnode(std::string opcode, std::unique_ptr<ASTnode> LHS, std::unique_ptr<ASTnode> RHS) : Opcode(opcode), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
  virtual Value *codegen() override;
  virtual std::string to_string() const override {
  //return a string representation of this AST node
    std::string out = "BinaryExpr: " + Opcode + "\n" + indent() + LHS->to_string() + "\n" + indent() + RHS->to_string();
    dedent();
    return out;
  };

};

class CallExprAST : public ASTnode {
  std::string Callee;
  std::vector<std::unique_ptr<ASTnode>> Args;

public:
  CallExprAST(const std::string &callee,
              std::vector<std::unique_ptr<ASTnode>> args)
    : Callee(callee), Args(std::move(args)) {}
    virtual Value *codegen() override;
    virtual std::string to_string() const override {
  //return a string representation of this AST node
    std::string arguments_string = "";
    for (int i = 0; i < Args.size(); i++){
      arguments_string.append("\n" + indent() + "Param: " + Args[i]->to_string());
    }
    std::string out = "FuncCall: " + Callee + arguments_string;
    dedent();
    return out;
  };
};

class IfExprAST : public ASTnode {
  std::unique_ptr<ASTnode> Cond;
  std::unique_ptr<ASTnode> Then, Else;

public:
  IfExprAST(std::unique_ptr<ASTnode> Cond, std::unique_ptr<ASTnode> Then,
            std::unique_ptr<ASTnode> Else)
      : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}
      virtual Value *codegen() override;
      virtual std::string to_string() const override {
  std::string out = "IfExpr:\n" + indent() + "Condition: " + Cond->to_string();
  out.append("\n" + indent() + "Then:" + Then->to_string() + "\n"+indent()+"Else:" + Else->to_string());  
  dedent();
  return out;
};

};

class WhileExprAST : public ASTnode {
  std::unique_ptr<ASTnode> Cond;
  std::unique_ptr<ASTnode> Then;

public:
  WhileExprAST(std::unique_ptr<ASTnode> Cond, std::unique_ptr<ASTnode> Then)
      : Cond(std::move(Cond)), Then(std::move(Then)) {}
      virtual Value *codegen() override;
       virtual std::string to_string() const override {
  //return a string representation of this AST node
    std::string ThenStr = "";
    std::string out = "WhileExpr:\n" + indent() + Cond->to_string() + "\n" + indent() + "Then: " + Then->to_string();
    // for(int i = 0; i < Then.size(); i++)
    // {
    //   if(Then[i] != nullptr)  
    //     ThenStr.append("\n" + indent() + "--> " + Then[i]->to_string());
    // }
    // out.append(ThenStr);
    dedent();
    return out;
  };
};


class ReturnExprAST : public ASTnode {
  std::unique_ptr<ASTnode> ReturnExpr;
  // std::string Type;

public:
  ReturnExprAST(std::unique_ptr<ASTnode> returnexpr) : ReturnExpr(std::move(returnexpr)) {}
  virtual Value *codegen() override;
  virtual std::string to_string() const override {
  //return a string representation of this AST node
    std::string returnExpr = "";
    std::string out = "";
    if(ReturnExpr != nullptr){
      out = "ReturnStmt\n" + indent() + ReturnExpr->to_string();
    }else{
       out = "ReturnStmt: Null";}
    dedent();
    return out;
  };
};

class PrototypeAST : public ASTnode{
  std::string Name;
  std::vector<std::unique_ptr<VariableASTnode>> Args;
  std::string Type;

public:
  PrototypeAST(const std::string name, std::vector<std::unique_ptr<VariableASTnode>> args, const std::string type)
    : Name(name), Type(type), Args(std::move(args)) {}

  virtual Function *codegen() override;

  const std::string getName() const { return Name; }
  // const std::vector<std::string> &getParamNames() const {return Args;}
  virtual std::string to_string() const override {
  //return a string representation of this AST node
  std::string args = "";

  for(int i = 0; i < std::move(Args).size(); i++)
  {
      args.append("\n" + indent() + "Param: " + std::move(Args)[i]->to_string());
  } 
  dedent();
  return "FunctionDecl: " +Type + " "+ Name + args;
  // dedent();
  // dedent();
  };
  
};

class FunctionAST : public ASTnode {
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ASTnode> Body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> proto,
              std::unique_ptr<ASTnode> body)
    : Proto(std::move(proto)), Body(std::move(body)) {}
    virtual Function *codegen() override;
    virtual std::string to_string() const override {
  std::string out = "Function:\n" + indent() + Proto->to_string() + "\n" + indent() + "Body:" + Body->to_string();
  dedent();
  return out;
};

};

// A BlockASTnode to represent blocks of code, containing local declarations and a list of statements
class BlockASTnode : public ASTnode {
    // std::vector<std::unique_ptr<ASTnode>> localDecls;
    // std::vector<std::unique_ptr<ASTnode>> stmtList;
    std::vector<std::unique_ptr<ASTnode>> localDecls;
    std::vector<std::unique_ptr<ASTnode>> stmtList;
public:
  

  BlockASTnode(std::vector<std::unique_ptr<ASTnode>> localDecls, std::vector<std::unique_ptr<ASTnode>> stmtList)
      : localDecls(std::move(localDecls)), stmtList(std::move(stmtList)) {}
  virtual Function *codegen() override;

  // Override virtual methods from ASTnode as needed
  virtual std::string to_string() const override {
    // Example implementation for debugging
    std::string out = "";
    for (const auto& decl : localDecls) {
      out.append("\n" + indent() + decl->to_string());
    }
    for (const auto& stmt : stmtList) {
      out.append("\n" + indent() + stmt->to_string());
    }
    dedent();
    return out;
  };
  
};

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

std::vector<TOKEN_TYPE> Follow_rvalI = {SC,RPAR,COMMA};
std::vector<TOKEN_TYPE> Follow_rval2I = {SC, RPAR, COMMA, OR};
std::vector<TOKEN_TYPE> Follow_rval3I = {SC, RPAR, COMMA, OR, AND};
std::vector<TOKEN_TYPE> Follow_rval4I = {SC, RPAR, COMMA, OR, AND, EQ, NE};
std::vector<TOKEN_TYPE> Follow_rval5I = {SC, RPAR, COMMA, OR, AND, EQ, NE, LE, LT, GE, GT};
std::vector<TOKEN_TYPE> Follow_rval6I = {SC, RPAR, COMMA, OR, AND, EQ, NE, LE, LT, GE, GT, PLUS, MINUS};
std::vector<TOKEN_TYPE> Follow_args = {RPAR};
std::vector<TOKEN_TYPE> Follow_arg_listI = {RPAR};
std::vector<TOKEN_TYPE> Follow_stmt_list = {RBRA};
std::vector<TOKEN_TYPE> Follow_else_stmt = {MINUS, NOT, LPAR, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT, SC, LBRA, IF, WHILE, RETURN, RBRA};
std::vector<TOKEN_TYPE> Follow_local_decls = {MINUS, NOT, LPAR, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT, SC, LBRA, IF, WHILE, RETURN, RBRA};
std::vector<TOKEN_TYPE> Follow_params = {RPAR};
std::vector<TOKEN_TYPE> Follow_param_listI = {RPAR};
std::vector<TOKEN_TYPE> Follow_decl_listI = {EOF_TOK};
std::vector<TOKEN_TYPE> Follow_extern_listI = {INT_TOK, FLOAT_TOK, BOOL_TOK, VOID_TOK};



//===----------------------------------------------------------------------===//
// Recursive Descent Parser - Function call for each production
//===----------------------------------------------------------------------===//

static bool isIn(int type, std::vector<TOKEN_TYPE> l)
{
  for(int i = 0; i < l.size(); i++)
  {
    if(l[i] == type)
      return true;
  }
  return false;
}

bool match(TOKEN_TYPE token)
{
  if(CurTok.type == token)
  {
    getNextToken(); //consume token
    return true;
  }
  else
    return false;
}

/* Add function calls for each production */
std::unique_ptr<rootASTnode> program();
std::vector<std::unique_ptr<ASTnode>> extern_list();
std::vector<std::unique_ptr<ASTnode>> extern_listI();
std::unique_ptr<ASTnode> pas_extern();
std::vector<std::unique_ptr<ASTnode>> decl_list();
std::vector<std::unique_ptr<ASTnode>> decl_listI();
std::unique_ptr<ASTnode> decl();
std::unique_ptr<ASTnode> var_decl();
std::string var_type();
std::string type_spec();
std::unique_ptr<ASTnode> fun_decl();
std::vector<std::unique_ptr<VariableASTnode>> params();
std::vector<std::unique_ptr<VariableASTnode>> param_list();
std::vector<std::unique_ptr<VariableASTnode>> param_listI();
std::unique_ptr<VariableASTnode> param();
std::unique_ptr<ASTnode> block();
std::vector<std::unique_ptr<ASTnode>>  local_decls();
std::unique_ptr<ASTnode> local_decl();
std::vector<std::unique_ptr<ASTnode>> stmt_list();
std::unique_ptr<ASTnode> stmt();
std::unique_ptr<ASTnode> expr_stmt();
std::unique_ptr<ASTnode> while_stmt();
std::unique_ptr<ASTnode> if_stmt();
std::unique_ptr<ASTnode> else_stmt();
std::unique_ptr<ASTnode> return_stmt();
std::unique_ptr<ASTnode> expr();
std::unique_ptr<ASTnode> rval();
std::unique_ptr<ASTnode> rvalI(std::unique_ptr<ASTnode>);
std::unique_ptr<ASTnode> rval2();
std::unique_ptr<ASTnode> rval2I(std::unique_ptr<ASTnode>);
std::unique_ptr<ASTnode> rval3();
std::unique_ptr<ASTnode> rval3I(std::unique_ptr<ASTnode>);
std::unique_ptr<ASTnode> rval4();
std::unique_ptr<ASTnode> rval4I(std::unique_ptr<ASTnode>);
std::unique_ptr<ASTnode> rval5();
std::unique_ptr<ASTnode> rval5I(std::unique_ptr<ASTnode>);
std::unique_ptr<ASTnode> rval6();
std::unique_ptr<ASTnode> rval6I(std::unique_ptr<ASTnode>);
std::unique_ptr<ASTnode> rval7();
std::unique_ptr<ASTnode> rval8();
std::vector<std::unique_ptr<ASTnode>> args();
std::vector<std::unique_ptr<ASTnode>> arg_list();
std::vector<std::unique_ptr<ASTnode>> arg_listI();




// extern externlistI
std::vector<std::unique_ptr<ASTnode>> extern_list() {
  std::vector<std::unique_ptr<ASTnode>> externNodes;

  auto externNode = pas_extern();
  if (externNode) {
    externNodes.push_back(std::move(externNode));
    auto restExterns = extern_listI();
    externNodes.insert(externNodes.end(), std::make_move_iterator(restExterns.begin()), std::make_move_iterator(restExterns.end()));
  } else {
    if (!errorReported) {
      errs() << "Syntax error: Invalid extern list at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
  }
  
  return externNodes;
}


// extern extern_listI | epsilon
std::vector<std::unique_ptr<ASTnode>> extern_listI() {
  std::vector<std::unique_ptr<ASTnode>> externNodes;

  if (isIn(CurTok.type, first_extern)) {
    auto externNode = pas_extern();
    if (externNode) {
      externNodes.push_back(std::move(externNode));
      auto restExterns = extern_listI();
      externNodes.insert(externNodes.end(), std::make_move_iterator(restExterns.begin()), std::make_move_iterator(restExterns.end()));
    } else {
      if (!errorReported) {
        errs() << "Syntax error: Invalid extern list continuation at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
      }
      errorReported = true;
    }
  } else if (!isIn(CurTok.type, Follow_extern_listI)) {
    if (!errorReported) {
      errs() << "Syntax error: Unexpected token in extern list at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
  }

  return externNodes;
}

// "extern" type_spec IDENT "(" params ")" ";"
std::unique_ptr<ASTnode> pas_extern() {
  if (!match(EXTERN)) {
    if (!errorReported) {
      errs() << "Syntax error: Expected 'extern' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return nullptr;
  }

  auto typeNode = type_spec();
  if (typeNode =="") return nullptr;
  std::string identifier = CurTok.lexeme;
  if (!match(IDENT)) {
    if (!errorReported) {
      errs() << "Syntax error: Expected identifier at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return nullptr;
  }
  

  if (!match(LPAR)) {
    if (!errorReported) {
      errs() << "Syntax error: Expected '(' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return nullptr;
  }

  auto paramsNode = params();
  // if (!paramsNode) return nullptr;

  if (!match(RPAR)) {
    if (!errorReported) {
      errs() << "Syntax error: Expected ')' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return nullptr;
  }

  if (!match(SC)) {
    if (!errorReported) {
      errs() << "Syntax error: Expected ';' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return nullptr;
  }

  return std::make_unique<PrototypeAST>(identifier, std::move(paramsNode),typeNode);
}


// decl_list ::= decl decl_listI
std::vector<std::unique_ptr<ASTnode>> decl_list() {
  std::vector<std::unique_ptr<ASTnode>> declarations;

  if (isIn(CurTok.type, first_decl)) {
    auto declNode = decl();
    if (declNode) {
      declarations.push_back(std::move(declNode));
      auto restDecls = decl_listI();
      declarations.insert(declarations.end(), std::make_move_iterator(restDecls.begin()), std::make_move_iterator(restDecls.end()));
    } else {
      if (!errorReported) {
        errs() << "Syntax error: Invalid declaration at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
      }
      errorReported = true;
    }
  }

  return declarations;
}


// decl_listI ::= decl decl_listI | epsilon
std::vector<std::unique_ptr<ASTnode>> decl_listI() {
  std::vector<std::unique_ptr<ASTnode>> declarations;

  if (isIn(CurTok.type, first_decl)) {
    auto declNode = decl();
    if (declNode) {
      declarations.push_back(std::move(declNode));
      auto restDecls = decl_listI();
      declarations.insert(declarations.end(), std::make_move_iterator(restDecls.begin()), std::make_move_iterator(restDecls.end()));
    } else {
      if (!errorReported) {
        errs() << "Syntax error: Invalid continuation of declaration list at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
      }
      errorReported = true;
    }
  } else if (!isIn(CurTok.type, Follow_decl_listI)) {
    if (!errorReported) {
      errs() << "Syntax error: Unexpected token in declaration list at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
  }

  return declarations;
}



//decl ::= var_decl |  fun_decl
std::unique_ptr<ASTnode> decl() {
  TOKEN look1 = CurTok;
  getNextToken();
  TOKEN look2 = CurTok;
  getNextToken();

  if (isIn(look1.type, first_var_decl) && CurTok.type == SC) {
    putBackToken(CurTok);
    putBackToken(look2);
    CurTok = look1;
    return var_decl();
  } else {
    putBackToken(CurTok);
    putBackToken(look2);
    CurTok = look1;

    if (isIn(CurTok.type, first_fun_decl)) {
      return fun_decl();
    } else {
      if (!errorReported) {
        errs() << "Syntax error: Invalid declaration at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
      }
      errorReported = true;
      return nullptr;
    }
  }
}


//var_decl ::= var_type IDENT ";" 
std::unique_ptr<ASTnode> var_decl() {
  auto typeNode = var_type();
  if (typeNode=="") return nullptr;
  std::string identifier = CurTok.lexeme;
  if (!match(IDENT)) {
    if (!errorReported) {
      errs() << "Syntax error: Expected an identifier at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return nullptr;
  }
  

  if (!match(SC)) {
    if (!errorReported) {
      errs() << "Syntax error: Expected ';' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return nullptr;
  }

  return std::make_unique<GlobalVariableASTnode>(typeNode, identifier);
}

//COULD CHANGE 
// to do match void, else push back and match vartype
//type_spec ::= "void" |  var_type     
std::string type_spec() {
  if (isIn(CurTok.type, first_type_spec)){
    if (match(VOID_TOK)){
      return "void";
    }
    else{
      return var_type();
    }
  }
  else {
    if(!errorReported)
        {errs()<<"Syntax error: Invalid type_spec token at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    return "";
  }
}

// var_type  ::= "int" |  "float" |  "bool"
std::string var_type() {
  if (match(INT_TOK)){
      return "int";
    }
    else {
      if (match(FLOAT_TOK)){
        return "float";
      }
      else {
        if (match(BOOL_TOK)){
          return "bool";
        }
        else {
            if(!errorReported)
                {errs()<<"Syntax error: Expected an Identifier at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
            errorReported = true;
            return "";
        }
      }
    }
}

//fun_decl ::= type_spec IDENT "(" params ")" block
std::unique_ptr<ASTnode> fun_decl() {
  // Parse the type specification
  if (!isIn(CurTok.type, first_type_spec)) {
    if (!errorReported)
      errs() << "Syntax error: Expected an Identifier at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    errorReported = true;
    return nullptr;
  }
  auto returnTypeNode = type_spec();

  // Parse the function name (identifier)
  std::string functionName = CurTok.lexeme;
  if (!match(IDENT)) {
    if (!errorReported)
      errs() << "Syntax error: Expected an Identifier at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    errorReported = true;
    return nullptr;
  }

  // Parse the opening parenthesis
  if (!match(LPAR)) {
    if (!errorReported)
      errs() << "Syntax error: Expected '(' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    errorReported = true;
    return nullptr;
  }

  // Parse parameters and collect them in a vector
  auto paramsNode = params();

  // Parse the closing parenthesis
  if (!match(RPAR)) {
    if (!errorReported)
      errs() << "Syntax error: Expected ')' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    errorReported = true;
    return nullptr;
  }

  // Parse the function body (block)
  auto bodyNode = block();
  if (!bodyNode) return nullptr;

  // Construct and return the FunctionAST node
  return std::make_unique<FunctionAST>(
    std::make_unique<PrototypeAST>(functionName, std::move(paramsNode),returnTypeNode), 
    std::move(bodyNode)
  );
}



//params ::= param_list | "void" | epsilon
std::vector<std::unique_ptr<VariableASTnode>> params() {
  std::vector<std::unique_ptr<VariableASTnode>> paramList;

  if (isIn(CurTok.type, first_param_list)) {
    paramList = param_list();
    // if (paramList.empty()) return nullptr; // Check if param_list failed
    return paramList;
  } else if (match(VOID_TOK)) {
    auto param_void = std::make_unique<VariableASTnode>("void", "void");
    paramList.push_back(std::move(param_void));
    return paramList;
    // "void" as a special case: function takes no parameters
  } else if (isIn(CurTok.type, Follow_params)) {
    return std::vector<std::unique_ptr<VariableASTnode>>();
  } else {
    if (!errorReported) {
      errs() << "Syntax error: Invalid params statement at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return std::vector<std::unique_ptr<VariableASTnode>>();
  }


}



// param_list ::= param param_listI
std::vector<std::unique_ptr<VariableASTnode>> param_list() {
  std::vector<std::unique_ptr<VariableASTnode>> paramList;

  // Parse the first parameter
  auto firstParam = param();
  if (!firstParam) {
    // Error handling if parsing fails
    return {};
  }
  paramList.push_back(std::move(firstParam));

  // Parse the rest of the parameters
  auto additionalParams = param_listI();
  paramList.insert(paramList.end(), std::make_move_iterator(additionalParams.begin()), std::make_move_iterator(additionalParams.end()));

  return paramList;
}

// param_listI ::= "," param param_listI | epsilon
std::vector<std::unique_ptr<VariableASTnode>> param_listI() {
  std::vector<std::unique_ptr<VariableASTnode>> paramList;

  if (CurTok.type == COMMA) { // Assuming COMMA is the token for ','
    getNextToken(); // Consume the comma

    // Parse the next parameter
    auto nextParam = param();
    if (!nextParam) {
      // Error handling if parsing fails
      return {};
    }
    paramList.push_back(std::move(nextParam));

    // Recursively parse more parameters and add them to paramList
    auto moreParams = param_listI();
    paramList.insert(paramList.end(), std::make_move_iterator(moreParams.begin()), std::make_move_iterator(moreParams.end()));
  } else if (isIn(CurTok.type, Follow_param_listI)) {
    // Epsilon case: no more parameters, return the accumulated list
    return paramList;
  } else {
    if (!errorReported) {
      errs() << "Syntax error: Invalid structure of parameters found at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return {};
  }

  return paramList;
}

// param ::= var_type IDENT
std::unique_ptr<VariableASTnode> param() {
  if (isIn(CurTok.type, first_param)) {
    auto typeNode = var_type();
    if (typeNode=="") return nullptr;

    std::string paramName = CurTok.lexeme;
    if (!match(IDENT)) {
      if (!errorReported) {
        errs() << "Syntax error: Invalid Identifier found at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
      }
      errorReported = true;
      return nullptr;
    }

    return std::make_unique<VariableASTnode>(typeNode, paramName);
  } else {
    if (!errorReported) {
      errs() << "Syntax error: Invalid declaration of parameter found at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return nullptr;
  }
}



// block ::= "{" local_decls stmt_list "}"

std::unique_ptr<ASTnode> block() {
  if (!match(LBRA)) {
    if (!errorReported)
      errs() << "Syntax error: Expected '{' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    errorReported = true;
    return nullptr;
  }

  std::vector<std::unique_ptr<ASTnode>> localdecls = local_decls();
  // if (!local_decls(localDecls)) return nullptr;

  std::vector<std::unique_ptr<ASTnode>> stmtlist = stmt_list();
  // if (!stmt_list(stmtList)) return nullptr;

  if (!match(RBRA)) {
    if (!errorReported)
      errs() << "Syntax error: Expected '}' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    errorReported = true;
    return nullptr;
  }

  return std::make_unique<BlockASTnode>(std::move(localdecls), std::move(stmtlist));
}



//local_decls ::= local_decl local_decls | epsilon
std::vector<std::unique_ptr<ASTnode>> local_decls() {
  std::vector<std::unique_ptr<ASTnode>> decls;
  while (isIn(CurTok.type, first_local_decl)) {
    auto decl = local_decl();
    if (!decl) {
      errorReported = true;
      return {};} // Return an empty vector on error
    decls.push_back(std::move(decl));
  }
  
  if (isIn(CurTok.type, Follow_local_decls)) {
    return decls;
  } else {
    if (!errorReported) {
      errs() << "Syntax error: Invalid local declaration at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return {};
  }
}



// local_decl ::= var_type IDENT ";"
std::unique_ptr<ASTnode> local_decl() {
  if (isIn(CurTok.type, first_local_decl)) {
    auto typeNode = var_type(); // var_type needs to return an AST node
    if (typeNode=="") {
      if (!errorReported)
        errs() << "Syntax error: Invalid var_type found at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
      errorReported = true;
      return nullptr;

    }

    std::string identifier = CurTok.lexeme;
    if (!match(IDENT)) {
      if (!errorReported)
        errs() << "Syntax error: Invalid Identifier found at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
      errorReported = true;
      return nullptr;
    }

    if (!match(SC)) {
      if (!errorReported)
        errs() << "Syntax error: Expected ';' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
      errorReported = true;
      return nullptr;
    }

    return std::make_unique<VariableASTnode>(typeNode, identifier);
  } else {
    if (!errorReported) {
      errs() << "Syntax error: Invalid local declaration found at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return nullptr;
  }
}


// stmt_list ::= stmt stmt_list |  epsilon
std::vector<std::unique_ptr<ASTnode>> stmt_list() {
  std::vector<std::unique_ptr<ASTnode>> stmts;
  while (isIn(CurTok.type, first_stmt_list)) {
    auto stmtNode = stmt();
    if (!stmtNode) return {}; // Return empty vector on error
    stmts.push_back(std::move(stmtNode));
  }

  if (isIn(CurTok.type, Follow_stmt_list)) {
    return stmts;
  } else {
    if (!errorReported) {
      errs() << "Syntax error: Invalid statement list structure found at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return {};
  }
}


//stmt ::= expr_stmt |  block |  if_stmt |  while_stmt |  return_stmt
std::unique_ptr<ASTnode> stmt() {
  if (isIn(CurTok.type, first_expr_stmt)) {
    return expr_stmt();
  } else if (isIn(CurTok.type, first_block)) {
    return block();
  } else if (isIn(CurTok.type, first_if_stmt)) {
    return if_stmt();
  } else if (isIn(CurTok.type, first_while_stmt)) {
    return while_stmt();
  } else if (isIn(CurTok.type, first_return_stmt)) {
    return return_stmt();
  } else {
    if (!errorReported) {
      errs() << "Syntax error: Invalid statement structure found at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return nullptr;
  }
}


// expr_stmt ::= expr ";" |  ";"
std::unique_ptr<ASTnode> expr_stmt() {
  if (isIn(CurTok.type, first_expr)) {
    auto exprNode = expr(); // expr() needs to return an AST node
    if (!exprNode) return nullptr;

    if (!match(SC)) {
      if (!errorReported)
        errs() << "Syntax error: Expected ';' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
      errorReported = true;
      return nullptr;
    }

    return std::move(exprNode);
  } else {
    if (!match(SC)) {
      if (!errorReported)
        errs() << "Syntax error: Expected ';' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
      errorReported = true;
      return nullptr;
    }
    return std::make_unique<VariableRefASTnode>("comma");
  }
}

// while_stmt ::= "while" "(" expr ")" stmt 
std::unique_ptr<ASTnode> while_stmt() {
  if (!match(WHILE)) {
    if (!errorReported)
      errs() << "Syntax error: Expected 'while' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    errorReported = true;
    return nullptr;
  }

  if (!match(LPAR)) {
    if (!errorReported)
      errs() << "Syntax error: Expected '(' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    errorReported = true;
    return nullptr;
  }

  auto condition = expr(); // expr() returns an AST node
  if (!condition) return nullptr;

  if (!match(RPAR)) {
    if (!errorReported)
      errs() << "Syntax error: Expected ')' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    errorReported = true;
    return nullptr;
  }

  auto body = stmt(); // stmt() returns an AST node
  if (!body) return nullptr;

  return std::make_unique<WhileExprAST>(std::move(condition), std::move(body));
}


//if_stmt ::= "if" "(" expr ")" block else_stmt

std::unique_ptr<ASTnode> if_stmt() {
  if (!match(IF)) {
    if (!errorReported)
      errs() << "Syntax error: Expected 'if' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    errorReported = true;
    return nullptr;
  }

  if (!match(LPAR)) {
    if (!errorReported)
      errs() << "Syntax error: Expected '(' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    errorReported = true;
    return nullptr;
  }

  auto condition = expr();
  if (!condition) return nullptr;

  if (!match(RPAR)) {
    if (!errorReported)
      errs() << "Syntax error: Expected ')' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    errorReported = true;
    return nullptr;
  }

  auto thenBlock = block();
  if (!thenBlock) return nullptr;

  auto elseBlock = else_stmt();

  return std::make_unique<IfExprAST>(std::move(condition), std::move(thenBlock), std::move(elseBlock));
}

// else_stmt  ::= "else" block |  epsilon
std::unique_ptr<ASTnode> else_stmt() {
  if (isIn(CurTok.type, first_else_stmt)) {
    getNextToken();
    auto elseBlock = block();
    if (!elseBlock) return nullptr;
    return std::move(elseBlock);
  } else if (isIn(CurTok.type, Follow_else_stmt)) {
    return nullptr; // No else block
  } else {
    if (!errorReported)
      errs() << "Syntax error: Invalid else statement at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    errorReported = true;
    return nullptr;
  }
}


// return_stmt ::= "return" ";" |  "return" expr ";"
std::unique_ptr<ASTnode> return_stmt() {
  if (!match(RETURN)) {
    if (!errorReported)
      errs() << "Syntax error: Expected 'return' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    errorReported = true;
    return nullptr;
  }

  if (CurTok.type == SC) {
    getNextToken();
    return std::make_unique<ReturnExprAST>(nullptr); // Return without expression
  } else {
    auto exprNode = expr();
    if (!exprNode) return nullptr;

    if (!match(SC)) {
      if (!errorReported)
        errs() << "Syntax error: Expected ';' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
      errorReported = true;
      return nullptr;
    }

    return std::make_unique<ReturnExprAST>(std::move(exprNode));
  }
}


// expr ::= IDENT "=" expr | rval 
std::unique_ptr<ASTnode> expr() {
  TOKEN look1 = CurTok;
  getNextToken();

  // If we have IDENT "=" expr
  if (look1.type == IDENT && CurTok.type == ASSIGN) {
    std::string varName = look1.lexeme;
    getNextToken();
    
    auto rhs = expr();
    if (!rhs) {
      if (!errorReported) {
        errs() << "Syntax error: Invalid expression at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
      }
      errorReported = true;
      return nullptr;
    }
    // Return a new BinaryExprASTnode for assignment
    return std::make_unique<BinaryExprASTnode>("=", std::make_unique<VariableRefASTnode>(varName), std::move(rhs));
  }

  // If we fall back to rval
  putBackToken(CurTok);
  CurTok = look1;

  if (isIn(CurTok.type, first_rval7_to_rval)) {
    return rval();
  } else {
    if (!errorReported) {
      errs() << "Syntax error: Invalid expression at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return nullptr;
  }
}


// rval ::= rval2 rvalI
std::unique_ptr<ASTnode> rval() {
  auto left = rval2();
  if (!left) return nullptr;
  return rvalI(std::move(left));
}



// ravlI ::= "||" rval2 rvalI | epsilon
std::unique_ptr<ASTnode> rvalI(std::unique_ptr<ASTnode> left) {
  if (isIn(CurTok.type, first_rvalI)) {
    TOKEN opTok = CurTok;
    getNextToken();
    
    auto right = rval2();
    if (!right) return nullptr;
    
    return rvalI(std::make_unique<BinaryExprASTnode>("||", std::move(left), std::move(right)));
  } else if (isIn(CurTok.type, Follow_rvalI)) {
    return left; // Return the constructed left node
  } else {
    if (!errorReported) {
      errs() << "Syntax error: Expected '||' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return nullptr;
  }
}






// rval2 ::= rval3 rval2I
std::unique_ptr<ASTnode> rval2() {
  auto left = rval3();
  if (!left) return nullptr;
  return rval2I(std::move(left));
}

// // rval2I ::= "&&" rval3 rval2I | epsilon
std::unique_ptr<ASTnode> rval2I(std::unique_ptr<ASTnode> left) {
  if (isIn(CurTok.type, first_rval2I)) {
    TOKEN opTok = CurTok;
    getNextToken();
    
    auto right = rval3();
    if (!right) return nullptr;
    
    return rval2I(std::make_unique<BinaryExprASTnode>("&&", std::move(left), std::move(right)));
  } else if (isIn(CurTok.type, Follow_rval2I)) {
    return left; // Return the constructed left node
  } else {
    if (!errorReported) {
      errs() << "Syntax error: Expected '&&' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return nullptr;
  }
}


// rval3 ::= rval4 rval3I
std::unique_ptr<ASTnode> rval3() {
  auto left = rval4();
  if (!left) return nullptr;
  return rval3I(std::move(left));
}

// rval3I ::= "==" rval4 rval3I | "!=" rval4 rval3I | epsilon
std::unique_ptr<ASTnode> rval3I(std::unique_ptr<ASTnode> left) {
  if (isIn(CurTok.type, first_rval3I)) {
    TOKEN opTok = CurTok; // Save the current token, which will be "==" or "!="
    std::string op = (CurTok.type == EQ) ? "==" : "!="; // Determine if it's "==" or "!="
    getNextToken(); // Move to the next token
    
    auto right = rval4(); // Parse the right-hand side expression
    if (!right) return nullptr; // If rval4 fails, return nullptr
    
    // Create a BinaryExprAST node with the operator and the left and right operands
    return rval3I(std::make_unique<BinaryExprASTnode>(op, std::move(left), std::move(right)));
  } else if (isIn(CurTok.type, Follow_rval3I)) {
    return left; // Return the left node if we've reached the end of the `rval3I` sequence
  } else {
    if (!errorReported) {
      errs() << "Syntax error: Expected '==' or '!=' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return nullptr; // Return nullptr if there was a syntax error
  }
}


// rval4 ::= rval5 rval4I

std::unique_ptr<ASTnode> rval4() {
  auto left = rval5();
  if (!left) return nullptr;
  return rval4I(std::move(left));
}
// rval4I ::= "<=" rval5 rval4I | "<" rval5 rval4I | ">=" rval5 rval4I | ">" rval5 rval4I | epsilon
std::unique_ptr<ASTnode> rval4I(std::unique_ptr<ASTnode> left) {
  if (isIn(CurTok.type, first_rval4I)) {
    TOKEN opTok = CurTok; // Save the current token, which will be one of the operators
    std::string op;
    
    // Determine which operator it is and assign the appropriate string value
    if (CurTok.type == LE) {
      op = "<=";
    } else if (CurTok.type == LT) {
      op = "<";
    } else if (CurTok.type == GE) {
      op = ">=";
    } else if (CurTok.type == GT) {
      op = ">";
    }
    
    getNextToken(); // Move to the next token
    
    // Parse the right-hand side operand (rval5)
    auto right = rval5();
    if (!right) return nullptr; // If rval5 fails, return nullptr
    
    // Construct the BinaryExprAST node for the operator and the operands
    return rval4I(std::make_unique<BinaryExprASTnode>(op, std::move(left), std::move(right)));
  } else if (isIn(CurTok.type, Follow_rval4I)) {
    return left; // Return the left node if we are at the end of the sequence
  } else {
    if (!errorReported) {
      errs() << "Syntax error: Expected '<=', '<', '>=', or '>' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return nullptr; // Return nullptr in case of a syntax error
  }
}


// rval5 ::= rval6 rval5I
std::unique_ptr<ASTnode> rval5() {
  auto left = rval6();
  if (!left) return nullptr;
  return rval5I(std::move(left));
}

// rval5I ::= "+" rval6 rval5I | "-" rval6 rval5I | epsilon
std::unique_ptr<ASTnode> rval5I(std::unique_ptr<ASTnode> left) {
  if (isIn(CurTok.type, first_rval5I)) {
    TOKEN opTok = CurTok; // Save the current token, which will be either '+' or '-'
    std::string op;
    
    // Determine which operator it is and assign the appropriate string value
    if (CurTok.type == PLUS) {
      op = "+";
    } else if (CurTok.type == MINUS) {
      op = "-";
    }
    
    getNextToken(); // Move to the next token
    
    // Parse the right-hand side operand (rval6)
    auto right = rval6();
    if (!right) return nullptr; // If rval6 fails, return nullptr
    
    // Construct the BinaryExprAST node for the operator and the operands
    return rval5I(std::make_unique<BinaryExprASTnode>(op, std::move(left), std::move(right)));
  } else if (isIn(CurTok.type, Follow_rval5I)) {
    return left; // Return the left node if no more operators are present
  } else {
    if (!errorReported) {
      errs() << "Syntax error: Expected '+' or '-' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return nullptr; // Return nullptr in case of a syntax error
  }
}

// rval6 ::= rval7 rval6I
std::unique_ptr<ASTnode> rval6() {
  auto left = rval7();
  if (!left) return nullptr;
  return rval6I(std::move(left));
}

//rval6I ::= "*" rval7 rval6I | "/" rval7 rval6I | "%" rval7 rval6I | epsilon
std::unique_ptr<ASTnode> rval6I(std::unique_ptr<ASTnode> left) {
  if (isIn(CurTok.type, first_rval6I)) {
    TOKEN opTok = CurTok; // Save the current token, which will be '*', '/', or '%'
    std::string op;
    
    // Determine which operator it is and assign the appropriate string value
    if (CurTok.type == ASTERIX) {
      op = "*";
    } else if (CurTok.type == DIV) {
      op = "/";
    } else if (CurTok.type == MOD) {
      op = "%";
    }
    
    getNextToken(); // Move to the next token
    
    // Parse the right-hand side operand (rval7)
    auto right = rval7();
    if (!right) return nullptr; // If rval7 fails, return nullptr
    
    // Construct the BinaryExprAST node for the operator and the operands
    return rval6I(std::make_unique<BinaryExprASTnode>(op, std::move(left), std::move(right)));
  } else if (isIn(CurTok.type, Follow_rval6I)) {
    return left; // Return the left node if no more operators are present
  } else {
    if (!errorReported) {
      errs() << "Syntax error: Expected '*', '/' or '%' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return nullptr; // Return nullptr in case of a syntax error
  }
}

// rval7 ::= "-" rval7 | "!" rval7 | rval8
std::unique_ptr<ASTnode> rval7() {
  if (match(MINUS)) {
    TOKEN opTok = CurTok;
    auto operand = rval7();
    if (!operand) return nullptr;
    
    return std::make_unique<UnaryExprASTnode>("-", std::move(operand));
  } else if (match(NOT)) {
    TOKEN opTok = CurTok;
    auto operand = rval7();
    if (!operand) return nullptr;
    
    return std::make_unique<UnaryExprASTnode>("!", std::move(operand));
  } else {
    return rval8();
  }
}


// rval8 ::= "(" expr ")" | IDENT | IDENT "(" args ")" | INT_LIT | FLOAT_LIT | BOOL_LIT 
std::unique_ptr<ASTnode> rval8() {
  if (match(LPAR)) {
    auto innerExpr = expr();
    if (!innerExpr) return nullptr;
    
    if (!match(RPAR)) {
      if (!errorReported) {
        errs() << "Syntax error: Expected ')' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
      }
      errorReported = true;
      return nullptr;
    }
    return innerExpr;
  } else {
    TOKEN look1 = CurTok;
    getNextToken();
    
    if (look1.type == IDENT && CurTok.type == LPAR) {
      std::string funcName = look1.lexeme;
      getNextToken();
      
      auto arguments = args();
      // if (!arguments) return nullptr;
      
      if (!match(RPAR)) {
        if (!errorReported) {
          errs() << "Syntax error: Expected ')' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
        }
        errorReported = true;
        return nullptr;
      }
      return std::make_unique<CallExprAST>(funcName, std::move(arguments));
    } else {
      putBackToken(CurTok);
      CurTok = look1;
      auto val = CurTok.lexeme;
      if (match(IDENT)) {
        return std::make_unique<VariableRefASTnode>(val);
      } else if (match(INT_LIT)) {
        return std::make_unique<IntASTnode>(std::stoi(val));
      } else if (match(FLOAT_LIT)) {
        return std::make_unique<FloatASTnode>(std::stof(val));
      } else if (match(BOOL_LIT)) {
        bool b_val;
        if (val == "true"){
          b_val = true;
        }else{
          b_val = false;
        }
        return std::make_unique<BoolASTnode>(b_val);
      } else {
        if (!errorReported) {
          errs() << "Syntax error: Expected '(' or Identifier or INT_LIT or BOOL_LIT or FLOAT_LIT at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
        }
        errorReported = true;
        return nullptr;
      }
    }
  }
}


// args ::= arg_list |  epsilon
std::vector<std::unique_ptr<ASTnode>> args() {
  std::vector<std::unique_ptr<ASTnode>> arguments;
  
  if (isIn(CurTok.type, first_arg_list)) {
    auto argList = arg_list();
    if (!argList.empty()) {
      arguments = std::move(argList);
    } else {
      if (!errorReported) {
        errs() << "Syntax error: Invalid argument list at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
      }
      errorReported = true;
    }
  }
  
  return arguments;
}


//arg_list ::= expr arg_listI
std::vector<std::unique_ptr<ASTnode>> arg_list() {
  std::vector<std::unique_ptr<ASTnode>> arguments;

  // Parse the first expression and add it to the arguments list.
  auto exprNode = expr();
  if (!exprNode) return arguments;  // Return an empty list if expr() fails.
  
  arguments.push_back(std::move(exprNode));

  // Process additional arguments with arg_listI.
  auto additionalArgs = arg_listI();
  if (!additionalArgs.empty()) {
    // If there are more arguments, add them to the list.
    arguments.insert(arguments.end(), std::make_move_iterator(additionalArgs.begin()), std::make_move_iterator(additionalArgs.end()));
  }

  return arguments;
}




//arg_listI ::= "," expr arg_listI | epsilon
std::vector<std::unique_ptr<ASTnode>> arg_listI() {
  std::vector<std::unique_ptr<ASTnode>> arguments;

  // If the current token is part of the list of additional argument rules.
  if (isIn(CurTok.type, first_arg_listI)) {
    if (!match(COMMA)) {
      if (!errorReported) {
        errs() << "Syntax error: Expected ',' at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
      }
      errorReported = true;
      return {};  // Return an empty vector on error.
    }

    // Parse the next expression and add it to the arguments list.
    auto exprNode = expr();
    if (!exprNode) return {};  // Return an empty list if expr() fails.

    arguments.push_back(std::move(exprNode));

    // Recursively process further arguments.
    auto moreArgs = arg_listI();
    if (!moreArgs.empty()) {
      arguments.insert(arguments.end(), std::make_move_iterator(moreArgs.begin()), std::make_move_iterator(moreArgs.end()));
    }
  } else if (isIn(CurTok.type, Follow_arg_listI)) {
    // If the token is in the follow set, return an empty list (epsilon).
    return {};
  } else {
    // Handle unexpected tokens.
    if (!errorReported) {
      errs() << "Syntax error: Invalid token in argument list at line " << CurTok.lineNo << " column " << CurTok.columnNo << ".\n";
    }
    errorReported = true;
    return {};  // Return an empty vector on error.
  }

  return arguments;
}


// program ::= extern_list decl_list | decl_list
std::unique_ptr<rootASTnode> program() {
  if (isIn(CurTok.type, first_extern_list)) {
    auto externs = extern_list();
    auto decls = decl_list();
    std::vector<std::unique_ptr<ASTnode>> topNodes;
    // Move each element from externs and decls into topNodes
    topNodes.insert(topNodes.end(),
                    std::make_move_iterator(externs.begin()),
                    std::make_move_iterator(externs.end()));

    topNodes.insert(topNodes.end(),
                    std::make_move_iterator(decls.begin()),
                    std::make_move_iterator(decls.end()));
    return std::make_unique<rootASTnode>(std::move(topNodes));
  } else {
    return std::make_unique<rootASTnode>(decl_list());
  }
}


static void parser() {
  getNextToken();
  // fprintf(stderr, "Token: %s with type %d\n", CurTok.lexeme.c_str(),
  //           CurTok.type);
  auto root = program();
  if (root && CurTok.type == EOF_TOK && !errorReported){
    // llvm::outs() << root << "\n";
    std::cout<<root->to_string()<<std::endl;
    std::cout<<"Parsing successful."<<std::endl;
    root->codegen();
    // return true;

  }
  else{
    if(!errorReported)
            {errs()<<"Syntax error: Invalid token1 at line "<<CurTok.lineNo<<" column "<<CurTok.columnNo<<".\n";}
    errorReported = true;
    std::cout<<"Parsing Failed"<<std::endl;
    // return false;
  }
}

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;
static std::vector<std::map<std::string,AllocaInst*>> NamedValuesList;
static std::map<std::string,GlobalVariable*> GlobalVariables;

// helper functions

Value* loadValue(Value* val) {
  if (auto *AI = dyn_cast<AllocaInst>(val)) {
    if (AI->getAllocatedType()->isFloatTy())
      return Builder.CreateLoad(Type::getFloatTy(TheContext), AI, "load_temp_float");
    else if (AI->getAllocatedType()->isIntegerTy(32))
      return Builder.CreateLoad(Type::getInt32Ty(TheContext), AI, "load_temp_int");
    else if (AI->getAllocatedType()->isIntegerTy(1))
      return Builder.CreateLoad(Type::getInt1Ty(TheContext), AI, "load_temp_bool");
  } else if (auto *GV = dyn_cast<GlobalVariable>(val)) {
    if (GV->getValueType()->isFloatTy())
      return Builder.CreateLoad(Type::getFloatTy(TheContext), GV, "load_global_temp_gloat");
    else if (GV->getValueType()->isIntegerTy(32))
      return Builder.CreateLoad(Type::getInt32Ty(TheContext), GV, "load_global_temp_float");
    else if (GV->getValueType()->isIntegerTy(1))
      return Builder.CreateLoad(Type::getInt1Ty(TheContext), GV, "load_global_temp_bool");
  }
  return val;
}

std::string getTypeString(Type* type) {
  if (type->isFloatTy())
    return "float";
  else if (type->isIntegerTy(32))
    return "int";
  else if (type->isIntegerTy(1))
    return "bool";
  return "unknown";
}

int getOperandType(Value* val) {
  if (val->getType()->isIntegerTy(1))
    return 0;
  else if (val->getType()->isIntegerTy(32))
    return 1;
  else if (val->getType()->isFloatTy())
    return 2;
  return -1; // Unknown type
}

Value* performWideningConversion(Value* val, int fromType, int toType) {
  if (fromType < toType) { // Widening conversion needed
    if (toType == 2) { // Convert to float
      if (fromType == 0) // bool to float
        val = Builder.CreateIntCast(val, Type::getInt32Ty(TheContext), false);
      return Builder.CreateCast(Instruction::SIToFP, val, Type::getFloatTy(TheContext), "cast_to_float");
    } else if (toType == 1) { // Convert to int
      return Builder.CreateIntCast(val, Type::getInt32Ty(TheContext), false, "cast_to_int");
    }
  }
  return val;
}


static AllocaInst* CreateEntryBlockAlloca(Function *TheFunction, std::string &VarName, std::string type) {
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
  TheFunction->getEntryBlock().begin());
  if(type == "int")
    return TmpB.CreateAlloca(Type::getInt32Ty(TheContext), 0, VarName.c_str()); 
  else if(type == "float")
    return TmpB.CreateAlloca(Type::getFloatTy(TheContext), 0, VarName.c_str());
  else if(type == "bool")
    return TmpB.CreateAlloca(Type::getInt1Ty(TheContext), 0, VarName.c_str());
  else
    return nullptr;
}


Value *IntASTnode::codegen() {
  return ConstantInt::get(TheContext, APInt(32,Val,true)); //int32 type
}

Value *FloatASTnode::codegen() {
  return ConstantFP::get(TheContext, APFloat(float(Val)));
}

Value *BoolASTnode::codegen() {
  return ConstantInt::get(TheContext, APInt(1,int(Val),false));
}

Value *VariableRefASTnode::codegen() {
  AllocaInst *V;

  std::map<std::string, AllocaInst*> NamedValues;
  for(int i = NamedValuesList.size() - 1; i >= 0; i--)
  {
    NamedValues = NamedValuesList[i];
    V = NamedValues[Name];
    if(!V)
    {
      continue;
    }
    else
    {
      return V;
    }

  }
  
  GlobalVariable *GV = GlobalVariables[Name];
  if(!GV)
  {
    errs()<<"Semantic error: Unknown variable name: "<<Name<<".\n";
    return nullptr;
  }
  else 
  {
    return GV;
  }
}


Value *VariableASTnode::codegen() {
    Function *TheFunction = Builder.GetInsertBlock()->getParent();
    AllocaInst *varAlloc = CreateEntryBlockAlloca(TheFunction, Val, Type);

    // Get the most recent symbol table (current scope)
    std::map<std::string, AllocaInst*> NamedValues = NamedValuesList.back();
    NamedValuesList.pop_back();

    // Check if the variable already exists in the current scope
    if (NamedValues.find(Val) != NamedValues.end()) {
        // Get the type of the existing variable
        std::string existingType = getTypeString(NamedValues[Val]->getAllocatedType());

        // Report a semantic error for redefinition
        // errs() << "Semantic error: Redefinition of variable " << Val
        //        << " at column no. " << Tok.columnNo
        //        << ", line no. " << Tok.lineNo << ".\n"
        //        << "Variable " << Val << " of type " << existingType
        //        << " already exists within the current scope.\n";
        errs() << "Semantic error: Redefinition of variable " << Val
               << " at column no., line no. .\nVariable"  << Val << " of type " << existingType
               << " already exists within the current scope.\n";
        return nullptr;
    }

    // Insert the new variable into the symbol table
    NamedValues[Val] = varAlloc;

    // Push the updated symbol table back to the vector
    NamedValuesList.push_back(NamedValues);

    return varAlloc;
}



Value* UnaryExprASTnode::codegen() {
  Value* operand = Operand->codegen();
  if (!operand)
    return nullptr;

  operand = loadValue(operand);
  int operandType = getOperandType(operand);
  std::string typeStr = getTypeString(operand->getType());

  if (Opcode == "!") {
    if (operandType == 0)
      return Builder.CreateNot(operand, "not_temp");
    else {
      errs() << "Semantic error: Cannot cast from `" << typeStr << "` to `bool`.\n";
      return nullptr;
    }
  } else if (Opcode == "-") {
    if (operandType == 0) // bool to int
      operand = Builder.CreateIntCast(operand, Type::getInt32Ty(TheContext), false);
    if (operandType == 2) // float
      return Builder.CreateFNeg(operand, "fneg_temp");
    return Builder.CreateNeg(operand, "neg_temp");
  }

  return nullptr;
}


// Value* BinaryExprASTnode::codegen() {
//   Value* lhs = loadValue(LHS->codegen());
//   if (!lhs) return nullptr;

//   if (Opcode == "&&" && lhs == ConstantInt::get(TheContext, APInt(1, false)))
//     return ConstantInt::get(TheContext, APInt(1, false));
//   if (Opcode == "||" && lhs == ConstantInt::get(TheContext, APInt(1, true)))
//     return ConstantInt::get(TheContext, APInt(1, true));

//   Value* rhs = loadValue(RHS->codegen());
//   if (!rhs) return nullptr;

//   int lhsType = getOperandType(lhs);
//   int rhsType = getOperandType(rhs);
//   lhs = performWideningConversion(lhs, lhsType, rhsType);
//   rhs = performWideningConversion(rhs, rhsType, lhsType);

//   if (Opcode == "+")
//     return lhs->getType()->isFloatTy() ? Builder.CreateFAdd(lhs, rhs, "fadd_tmp") : Builder.CreateAdd(lhs, rhs, "add_tmp");
//   else if (Opcode == "-")
//     return lhs->getType()->isFloatTy() ? Builder.CreateFSub(lhs, rhs, "fsub_tmp") : Builder.CreateSub(lhs, rhs, "sub_tmp");
//   else if (Opcode == "*")
//     return lhs->getType()->isFloatTy() ? Builder.CreateFMul(lhs, rhs, "fmul_tmp") : Builder.CreateMul(lhs, rhs, "mul_tmp");
//   else if (Opcode == "/") {
//     if (rhs == ConstantInt::get(TheContext, APInt(32, 0)) || rhs == ConstantFP::get(TheContext, APFloat(0.0f))) {
//       errs() << "Semantic error: Division by zero.\n";
//       return nullptr;
//     }
//     return lhs->getType()->isFloatTy() ? Builder.CreateFDiv(lhs, rhs, "fdiv_tmp") : Builder.CreateSDiv(lhs, rhs, "div_tmp");
//   }

//   return nullptr;
// }
// Value* BinaryExprASTnode::codegen() {
//   // Generate and load the left-hand side
//   Value* lhs = loadValue(LHS->codegen());
//   if (!lhs) return nullptr;

//   // Short-circuiting for logical AND (&&) and OR (||) operations
//   if (Opcode == "&&") {
//     if (lhs == ConstantInt::get(TheContext, APInt(1, false))) // Short-circuit for `false &&`
//       return ConstantInt::get(TheContext, APInt(1, false));
//     // Create a block to evaluate `rhs` only if `lhs` is true
//     BasicBlock *rhsBlock = BasicBlock::Create(TheContext, "and_rhs", Builder.GetInsertBlock()->getParent());
//     BasicBlock *endBlock = BasicBlock::Create(TheContext, "and_end", Builder.GetInsertBlock()->getParent());
//     Builder.CreateCondBr(lhs, rhsBlock, endBlock);
//     Builder.SetInsertPoint(rhsBlock);
//     Value* rhs = loadValue(RHS->codegen());
//     if (!rhs) return nullptr;
//     Builder.CreateBr(endBlock);
//     Builder.SetInsertPoint(endBlock);
//     PHINode *phiNode = Builder.CreatePHI(Type::getInt1Ty(TheContext), 2, "and_tmp");
//     phiNode->addIncoming(lhs, Builder.GetInsertBlock());
//     phiNode->addIncoming(rhs, rhsBlock);
//     return phiNode;
//   } else if (Opcode == "||") {
//     if (lhs == ConstantInt::get(TheContext, APInt(1, true))) // Short-circuit for `true ||`
//       return ConstantInt::get(TheContext, APInt(1, true));
//     // Create a block to evaluate `rhs` only if `lhs` is false
//     BasicBlock *rhsBlock = BasicBlock::Create(TheContext, "or_rhs", Builder.GetInsertBlock()->getParent());
//     BasicBlock *endBlock = BasicBlock::Create(TheContext, "or_end", Builder.GetInsertBlock()->getParent());
//     Builder.CreateCondBr(lhs, endBlock, rhsBlock);
//     Builder.SetInsertPoint(rhsBlock);
//     Value* rhs = loadValue(RHS->codegen());
//     if (!rhs) return nullptr;
//     Builder.CreateBr(endBlock);
//     Builder.SetInsertPoint(endBlock);
//     PHINode *phiNode = Builder.CreatePHI(Type::getInt1Ty(TheContext), 2, "or_tmp");
//     phiNode->addIncoming(lhs, Builder.GetInsertBlock());
//     phiNode->addIncoming(rhs, rhsBlock);
//     return phiNode;
//   }

//   // Generate and load the right-hand side
//   Value* rhs = loadValue(RHS->codegen());
//   if (!rhs) return nullptr;

//   // Type conversion to ensure compatibility
//   int lhsType = getOperandType(lhs);
//   int rhsType = getOperandType(rhs);
//   lhs = performWideningConversion(lhs, lhsType, rhsType);
//   rhs = performWideningConversion(rhs, rhsType, lhsType);

//   // Arithmetic and comparison operations
//   if (Opcode == "+")
//     return lhs->getType()->isFloatTy() ? Builder.CreateFAdd(lhs, rhs, "fadd_tmp") : Builder.CreateAdd(lhs, rhs, "add_tmp");
//   else if (Opcode == "-")
//     return lhs->getType()->isFloatTy() ? Builder.CreateFSub(lhs, rhs, "fsub_tmp") : Builder.CreateSub(lhs, rhs, "sub_tmp");
//   else if (Opcode == "*")
//     return lhs->getType()->isFloatTy() ? Builder.CreateFMul(lhs, rhs, "fmul_tmp") : Builder.CreateMul(lhs, rhs, "mul_tmp");
//   else if (Opcode == "/") {
//     if (rhs == ConstantInt::get(TheContext, APInt(32, 0)) || rhs == ConstantFP::get(TheContext, APFloat(0.0f))) {
//       errs() << "Semantic error: Division by zero.\n";
//       return nullptr;
//     }
//     return lhs->getType()->isFloatTy() ? Builder.CreateFDiv(lhs, rhs, "fdiv_tmp") : Builder.CreateSDiv(lhs, rhs, "div_tmp");
//   }

//   return nullptr; // Unsupported operation
// }
Value* BinaryExprASTnode::codegen() {
  // Generate and load the left-hand side operand
  Value* lhs = loadValue(LHS->codegen());
  if (!lhs) return nullptr;

  // Boolean short-circuiting for logical operators || and &&
  if (Opcode == "&&") {
    if (lhs == ConstantInt::get(TheContext, APInt(1, false))) // false && ...
      return ConstantInt::get(TheContext, APInt(1, false));
  } else if (Opcode == "||") {
    if (lhs == ConstantInt::get(TheContext, APInt(1, true))) // true || ...
      return ConstantInt::get(TheContext, APInt(1, true));
  }

  // Generate and load the right-hand side operand
  Value* rhs = loadValue(RHS->codegen());
  if (!rhs) return nullptr;

  // Retrieve operand types
  int lhsType = getOperandType(lhs);
  int rhsType = getOperandType(rhs);

  // Widen operands if needed to ensure both are of the same type
  lhs = performWideningConversion(lhs, lhsType, rhsType);
  rhs = performWideningConversion(rhs, rhsType, lhsType);

  // Handling assignment operator `=`
  if (Opcode == "=") {
    // Only proceed if `LHS` is assignable (an `AllocaInst` or `GlobalVariable`)
    if (auto* AI = dyn_cast<AllocaInst>(LHS->codegen())) {
      // Perform any necessary type widening on `rhs`
      if (lhsType < rhsType) {
        errs() << "Error: Cannot widen from RHS type " << getTypeString(rhs->getType())
               << " to LHS type " << getTypeString(lhs->getType()) << ".\n";
        return nullptr;
      }
      return Builder.CreateStore(rhs, AI);
    } else if (auto* GV = dyn_cast<GlobalVariable>(LHS->codegen())) {
      return Builder.CreateStore(rhs, GV);
    }
    errs() << "Error: LHS of assignment must be a variable.\n";
    return nullptr;
  }

  // Enforce boolean types for logical operators && and ||
  if (Opcode == "&&" || Opcode == "||") {
    if (lhsType != 0 || rhsType != 0) {
      errs() << "Error: Logical operators && and || require boolean operands.\n";
      return nullptr;
    }
    return (Opcode == "&&") ? Builder.CreateAnd(lhs, rhs, "and_tmp") : Builder.CreateOr(lhs, rhs, "or_tmp");
  }

  // Arithmetic and comparison operations
  if (Opcode == "+") {
    return lhs->getType()->isFloatTy() ? Builder.CreateFAdd(lhs, rhs, "fadd_tmp") : Builder.CreateAdd(lhs, rhs, "add_tmp");
  } else if (Opcode == "-") {
    return lhs->getType()->isFloatTy() ? Builder.CreateFSub(lhs, rhs, "fsub_tmp") : Builder.CreateSub(lhs, rhs, "sub_tmp");
  } else if (Opcode == "*") {
    return lhs->getType()->isFloatTy() ? Builder.CreateFMul(lhs, rhs, "fmul_tmp") : Builder.CreateMul(lhs, rhs, "mul_tmp");
  } else if (Opcode == "/") {
    if (rhs == ConstantInt::get(TheContext, APInt(32, 0)) || rhs == ConstantFP::get(TheContext, APFloat(0.0f))) {
      errs() << "Error: Division by zero.\n";
      return nullptr;
    }
    return lhs->getType()->isFloatTy() ? Builder.CreateFDiv(lhs, rhs, "fdiv_tmp") : Builder.CreateSDiv(lhs, rhs, "div_tmp");
  } else if (Opcode == "%") {
    if (rhs == ConstantInt::get(TheContext, APInt(32, 0))) {
      errs() << "Error: Modulo by zero.\n";
      return nullptr;
    }
    return lhs->getType()->isFloatTy() ? Builder.CreateFRem(lhs, rhs, "fmod_tmp") : Builder.CreateSRem(lhs, rhs, "mod_tmp");
  } else if (Opcode == "==") {
    return lhs->getType()->isFloatTy() ? Builder.CreateFCmpOEQ(lhs, rhs, "feq_tmp") : Builder.CreateICmpEQ(lhs, rhs, "eq_tmp");
  } else if (Opcode == "!=") {
    return lhs->getType()->isFloatTy() ? Builder.CreateFCmpONE(lhs, rhs, "fne_tmp") : Builder.CreateICmpNE(lhs, rhs, "ne_tmp");
  } else if (Opcode == "<") {
    return lhs->getType()->isFloatTy() ? Builder.CreateFCmpOLT(lhs, rhs, "flt_tmp") : Builder.CreateICmpSLT(lhs, rhs, "lt_tmp");
  } else if (Opcode == "<=") {
    return lhs->getType()->isFloatTy() ? Builder.CreateFCmpOLE(lhs, rhs, "fle_tmp") : Builder.CreateICmpSLE(lhs, rhs, "le_tmp");
  } else if (Opcode == ">") {
    return lhs->getType()->isFloatTy() ? Builder.CreateFCmpOGT(lhs, rhs, "fgt_tmp") : Builder.CreateICmpSGT(lhs, rhs, "gt_tmp");
  } else if (Opcode == ">=") {
    return lhs->getType()->isFloatTy() ? Builder.CreateFCmpOGE(lhs, rhs, "fge_tmp") : Builder.CreateICmpSGE(lhs, rhs, "ge_tmp");
  }

  // Unsupported operator error
  errs() << "Error: Unsupported binary operator '" << Opcode << "'.\n";
  return nullptr;
}



// Value* CallExprAST::codegen() {
//   // Look up the function in the module.
//   Function *CalleeF = TheModule->getFunction(Callee);
//   if (!CalleeF) {
//     // errs() << "Semantic error: Unknown function " << Callee << " referenced at line no. "
//     //        << Tok.lineNo << " column no. " << Tok.columnNo << ".\n";
//     errs() << "Semantic error: Unknown function " << Callee;
//     return nullptr;
//   }

//   // Check if the number of arguments matches.
//   if (CalleeF->arg_size() != Args.size()) {
//     // errs() << "Semantic error: Incorrect number of arguments for function " << Callee
//     //        << " at line no. " << Tok.lineNo << " column no. " << Tok.columnNo << ".\n";
//         errs() << "Semantic error: Incorrect number of arguments for function " << Callee;
//     return nullptr;
//   }

//   std::vector<Value *> ArgsV;
//   for (unsigned i = 0, e = Args.size(); i != e; ++i) {
//     Value* arg = loadValue(Args[i]->codegen());
//     if (!arg) return nullptr;

//     int argType = getOperandType(arg);
//     Type* expectedType = CalleeF->getArg(i)->getType();
//     int expectedTypeInt = getOperandType(CalleeF->getArg(i));

//     // Perform widening conversion if necessary.
//     if (argType != expectedTypeInt) {
//       arg = performWideningConversion(arg, argType, expectedTypeInt);
//       if (!arg) {
//         errs() << "Semantic error: Cannot cast from `" << getTypeString(arg->getType())
//                << "` to `" << getTypeString(expectedType) << "` at line no. ";
//               //  << Tok.lineNo << " column no. " << Tok.columnNo << ".\n";
//         return nullptr;
//       }
//     }

//     ArgsV.push_back(arg);
//   }

//   // Create the function call.
//   if (CalleeF->getReturnType()->isVoidTy()) {
//     return Builder.CreateCall(CalleeF, ArgsV);
//   } else {
//     return Builder.CreateCall(CalleeF, ArgsV, "call_tmp");
//   }
// }



Value* CallExprAST::codegen() {
  // Look up the function in the module.
  Function *CalleeF = TheModule->getFunction(Callee);
  if (!CalleeF) {
    errs() << "Semantic error: Unknown function " << Callee << "\n";
    return nullptr;
  }

  // Check if the number of arguments matches.
  if (CalleeF->arg_size() != Args.size()) {
    errs() << "Semantic error: Incorrect number of arguments for function " << Callee << "\n";
    return nullptr;
  }

  std::vector<Value *> ArgsV;
  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
    Value* arg = loadValue(Args[i]->codegen());
    if (!arg) return nullptr;

    int argType = getOperandType(arg);
    Type* expectedType = CalleeF->getArg(i)->getType();
    int expectedTypeInt = getOperandType(CalleeF->getArg(i));

    // Perform widening conversion if necessary.
    if (argType != expectedTypeInt) {
      arg = performWideningConversion(arg, argType, expectedTypeInt);
      if (!arg) {
        errs() << "Semantic error: Cannot cast from `" << getTypeString(arg->getType())
               << "` to `" << getTypeString(expectedType) << "`\n";
        return nullptr;
      }
    }

    ArgsV.push_back(arg);
  }

  // Create the function call.
  if (CalleeF->getReturnType()->isVoidTy()) {
    Builder.CreateCall(CalleeF, ArgsV);
    return nullptr; // No value for void-returning function
  } else {
    return Builder.CreateCall(CalleeF, ArgsV, "call_tmp");
  }
}

Value* IfExprAST::codegen() {
  // Check if else block exists
  bool elseExists = (Else != nullptr);

  // Get the parent function
  Function* TheFunction = Builder.GetInsertBlock()->getParent();

  // Create necessary basic blocks and associate them with the function
  BasicBlock* trueBlock = BasicBlock::Create(TheContext, "if_then", TheFunction);
  BasicBlock* falseBlock = elseExists ? BasicBlock::Create(TheContext, "if_else", TheFunction) : nullptr;
  BasicBlock* endBlock = BasicBlock::Create(TheContext, "if_end", TheFunction);

  // Generate the condition expression
  Value* cond = Cond->codegen();
  if (!cond) {
    errs() << "Error: Condition of if statement could not be generated.\n";
    return nullptr;
  }

  // Load the condition value if necessary
  cond = loadValue(cond);

  // Ensure the condition is a boolean type
  if (!cond->getType()->isIntegerTy(1)) {
    errs() << "Error: Condition of if statement must be of boolean type.\n";
    return nullptr;
  }

  // Create a comparison for the boolean condition
  Value* comp = Builder.CreateICmpNE(cond, ConstantInt::get(TheContext, APInt(1, 0)), "if_cond");

  // Create the conditional branch
  if (elseExists)
    Builder.CreateCondBr(comp, trueBlock, falseBlock);
  else
    Builder.CreateCondBr(comp, trueBlock, endBlock);

  // Generate code for the 'then' block
  Builder.SetInsertPoint(trueBlock);
  std::map<std::string, AllocaInst*> NamedValues_Then;
  NamedValuesList.push_back(NamedValues_Then);

  Value* thenVal = Then->codegen();
  if (!thenVal)
    return nullptr;

  // Branch to end block if no return statement was encountered
  if (!isa<ReturnInst>(thenVal)) {
    Builder.CreateBr(endBlock);
  }
  NamedValuesList.pop_back();

  // Generate code for the 'else' block if it exists
  if (elseExists) {
    Builder.SetInsertPoint(falseBlock);
    std::map<std::string, AllocaInst*> NamedValues_Else;
    NamedValuesList.push_back(NamedValues_Else);

    Value* elseVal = Else->codegen();
    if (!elseVal)
      return nullptr;

    if (!isa<ReturnInst>(elseVal)) {
      Builder.CreateBr(endBlock);
    }
    NamedValuesList.pop_back();
  }

  // Set the insertion point to the end block
  Builder.SetInsertPoint(endBlock);

  // Return nullptr for void functions
  if (TheFunction->getReturnType()->isVoidTy()) {
    return nullptr;
  }

  // Return an undefined value for other return types
  return UndefValue::get(TheFunction->getReturnType());
}


// Value* IfExprAST::codegen() {
//   // Check if else block exists
//   bool elseExists = (Else != nullptr);

//   // Get the parent function
//   Function* TheFunction = Builder.GetInsertBlock()->getParent();

//   // Create necessary basic blocks and associate them with the function
//   BasicBlock* trueBlock = BasicBlock::Create(TheContext, "if_then", TheFunction);
//   BasicBlock* falseBlock = elseExists ? BasicBlock::Create(TheContext, "if_else", TheFunction) : nullptr;
//   BasicBlock* endBlock = BasicBlock::Create(TheContext, "if_end", TheFunction);

//   // Generate the condition expression
//   Value* cond = Cond->codegen();
//   if (!cond)
//     return nullptr;

//   // Load the condition value if necessary
//   cond = loadValue(cond);

//   // Ensure the condition is a boolean type
//   if (!cond->getType()->isIntegerTy(1)) {
//     errs() << "Error: Condition of if statement must be of boolean type.\n";
//     return nullptr;
//   }

//   // Create a comparison for the boolean condition
//   Value* comp = Builder.CreateICmpNE(cond, ConstantInt::get(TheContext, APInt(1, 0)), "if_cond");

//   // Create the conditional branch
//   if (elseExists)
//     Builder.CreateCondBr(comp, trueBlock, falseBlock);
//   else
//     Builder.CreateCondBr(comp, trueBlock, endBlock);

//   // Generate code for the 'then' block
//   Builder.SetInsertPoint(trueBlock);
//   std::map<std::string, AllocaInst*> NamedValues_Then;
//   NamedValuesList.push_back(NamedValues_Then);

//   Value* thenVal = Then->codegen();
//   if (!thenVal)
//     return nullptr;

//   // Branch to end block if no return statement was encountered
//   if (!isa<ReturnInst>(thenVal)) {
//     Builder.CreateBr(endBlock);
//   }
//   NamedValuesList.pop_back();

//   // Generate code for the 'else' block if it exists
//   if (elseExists) {
//     Builder.SetInsertPoint(falseBlock);
//     std::map<std::string, AllocaInst*> NamedValues_Else;
//     NamedValuesList.push_back(NamedValues_Else);

//     Value* elseVal = Else->codegen();
//     if (!elseVal)
//       return nullptr;

//     if (!isa<ReturnInst>(elseVal)) {
//       Builder.CreateBr(endBlock);
//     }
//     NamedValuesList.pop_back();
//   }

//   // Set the insertion point to the end block
//   Builder.SetInsertPoint(endBlock);

//   // Return nullptr for void functions
//   if (TheFunction->getReturnType()->isVoidTy()) {
//     return nullptr;
//   }

//   // Return an undefined value for other return types
//   return UndefValue::get(TheFunction->getReturnType());
// }



Value* WhileExprAST::codegen() {
  Function* TheFunction = Builder.GetInsertBlock()->getParent();
  BasicBlock* cond_ = BasicBlock::Create(TheContext, "while_cond", TheFunction);
  BasicBlock* body_ = BasicBlock::Create(TheContext, "while_body", TheFunction);
  BasicBlock* end_ = BasicBlock::Create(TheContext, "while_end");

  // Jump to condition block
  Builder.CreateBr(cond_);
  Builder.SetInsertPoint(cond_);

  // Generate condition expression
  Value* cond = Cond->codegen();
  if (!cond)
    return nullptr;
  cond = loadValue(cond);

  // Ensure the condition is of boolean type
  std::string currType = getTypeString(cond->getType());
  if (currType != "bool") {
    errs() << "Semantic error: Expected type `bool` for the condition statement at line no. ";
          //  << Cond->getTok().lineNo << " column no. " << Cond->getTok().columnNo
          //  << ". Cannot cast from type `" << currType << "` to `bool`.\n";
    return nullptr;
  }

  // Create comparison for boolean condition
  Value* comp = Builder.CreateICmpNE(cond, ConstantInt::get(TheContext, APInt(1, 0, false)), "while_cond");

  // Create conditional branch
  Builder.CreateCondBr(comp, body_, end_);

  // Generate `while_body` block
  Builder.SetInsertPoint(body_);
  std::map<std::string, AllocaInst*> NamedValues_Body;
  NamedValuesList.push_back(NamedValues_Body);

  bool generateBranchForBody = true;
  Value* bodyVal = Then->codegen();
  if (!bodyVal)
      return nullptr;
  if (dyn_cast<ReturnInst>(bodyVal)) {
      generateBranchForBody = false;
  }

  if (generateBranchForBody)
    Builder.CreateBr(cond_);
  NamedValuesList.pop_back();

  // Generate `while_end` block
  TheFunction->insert(TheFunction->end(), end_);
  Builder.SetInsertPoint(end_);
  return ConstantPointerNull::get(PointerType::getUnqual(Type::getVoidTy(TheContext)));
}

Value* ReturnExprAST::codegen() {
  if (!ReturnExpr)
    return Builder.CreateRetVoid();

  Value* returnExpr = ReturnExpr->codegen();
  if (!returnExpr)
    return nullptr;

  returnExpr = loadValue(returnExpr);

  std::string actualType = getTypeString(returnExpr->getType());
  // std::string correctType = FuncReturnType;

  // Handle type mismatch with widening conversion if possible
  // FIX THIS ------------------------------------------------------------------------------------------------------
  // if (correctType != actualType) {
  //   int fromType = getOperandType(returnExpr);
  //   int toType = (correctType == "int") ? 1 : (correctType == "float") ? 2 : 0;

  //   if (toType < fromType) {
  //     errs() << "Semantic Error: Incorrect return type `" << actualType
  //            << "` used at line no: " << Tok.lineNo << " column no: " << Tok.columnNo
  //            << ". Cannot cast to expected return type `" << correctType << "`.\n";
  //     return nullptr;
  //   }

    // Perform widening conversion
  //   returnExpr = performWideningConversion(returnExpr, fromType, toType);
  // }

  return Builder.CreateRet(returnExpr);
}

Value* GlobalVariableASTnode::codegen() {
  std::string ty = getType();
  llvm::Type* t = nullptr;
  int alignSize = 4;

  if (ty == "int")
    t = Type::getInt32Ty(TheContext);
  else if (ty == "float")
    t = Type::getFloatTy(TheContext);
  else if (ty == "bool") {
    t = Type::getInt1Ty(TheContext);
    alignSize = 1;
  } else {
    return nullptr;
  }

  GlobalVariable* g = new GlobalVariable(
    *(TheModule.get()), t, false, GlobalValue::CommonLinkage, Constant::getNullValue(t), Val);
  g->setAlignment(MaybeAlign(alignSize));

  if (!GlobalVariables.insert({Val, g}).second) {
    std::string existTy = getTypeString(GlobalVariables[Val]->getValueType());
    errs() << "Semantic error: Redefinition of global variable `" << Val << "` with type `" << ty;
          //  << "` at line " << Tok.lineNo << " column " << Tok.columnNo << ". Variable of type `" << existTy
          //  << "` already exists.\n";
    return nullptr;
  }

  return g;
}


Function* PrototypeAST::codegen() {
  // Use the member `Type` directly for return type
  std::string ReturnType = Type;
  std::string origName = Name;

  // Determine the return LLVM type
  llvm::Type* RetType = nullptr;
  if (ReturnType == "int")
    RetType = llvm::Type::getInt32Ty(TheContext);
  else if (ReturnType == "float")
    RetType = llvm::Type::getFloatTy(TheContext);
  else if (ReturnType == "bool")
    RetType = llvm::Type::getInt1Ty(TheContext);
  else if (ReturnType == "void")
    RetType = llvm::Type::getVoidTy(TheContext);

  if (!RetType) {
    errs() << "Error: Unknown return type '" << ReturnType << "' for function '" << origName << "'.\n";
    return nullptr;
  }

  // Prepare argument types
  std::vector<llvm::Type*> ArgTypes;
  for (const auto& Arg : Args) {
    std::string ArgType = Arg->getType();
    if (ArgType == "int")
      ArgTypes.push_back(llvm::Type::getInt32Ty(TheContext));
    else if (ArgType == "float")
      ArgTypes.push_back(llvm::Type::getFloatTy(TheContext));
    else if (ArgType == "bool")
      ArgTypes.push_back(llvm::Type::getInt1Ty(TheContext));
    else {
      errs() << "Error: Unknown argument type '" << ArgType << "' in function '" << origName << "'.\n";
      return nullptr;
    }
  }

  // Create the function type
  llvm::FunctionType* FT = llvm::FunctionType::get(RetType, ArgTypes, false);
  llvm::Function* F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, origName, TheModule.get());

  // Set names for all arguments
  unsigned Idx = 0;
  for (auto& Arg : F->args()) {
    Arg.setName(Args[Idx]->getName());
    Idx++;
  }

  return F;
}

Function* FunctionAST::codegen() {
  // Retrieve or create the function definition from the prototype
  Function* TheFunction = TheModule->getFunction(Proto->getName());
  if (!TheFunction)
    TheFunction = Proto->codegen();
  if (!TheFunction)
    return nullptr;

  // Create the entry basic block and set the insertion point
  BasicBlock* BB = BasicBlock::Create(TheContext, "entry", TheFunction);
  Builder.SetInsertPoint(BB);

  // Symbol table for named values in this function scope
  std::map<std::string, AllocaInst*> NamedValues;

  for (auto& Arg : TheFunction->args()) {
    std::string type = getTypeString(Arg.getType());
    std::string varName = Arg.getName().str(); // Create a named std::string
    AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, varName, type); // Pass the named variable
    Builder.CreateStore(&Arg, Alloca);
    NamedValues[Arg.getName().str()] = Alloca;
}


  // Push the symbol table context for this function
  NamedValuesList.push_back(NamedValues);

  // Generate code for the function body using the BlockASTnode's codegen
  Value* RetVal = Body ? Body->codegen() : nullptr;
  if (!RetVal) {
    NamedValuesList.pop_back();
    return nullptr;
  }

  // Check if we need a return statement for a void function
  std::string returnType = getTypeString(TheFunction->getReturnType());
  if (returnType == "void" && !isa<ReturnInst>(RetVal)) {
    Builder.CreateRetVoid();
  }

  // Verify and finalize the function
  verifyFunction(*TheFunction);
  NamedValuesList.pop_back();

  return TheFunction;
}

Function* BlockASTnode::codegen() {
  // For local declarations, add allocas to the current basic block
  for (const auto& decl : localDecls) {
    if (!decl->codegen())
      return nullptr;
  }

  // Generate code for each statement in the block
  bool returnSet = false;
  for (const auto& stmt : stmtList) {
    if (returnSet) 
      break;

    Value* StmtVal = stmt->codegen();
    if (!StmtVal)
      return nullptr;

    // If the statement is a return, mark that we've set the return
    if (isa<ReturnInst>(StmtVal)) {
      returnSet = true;
    }
  }

  // Return the last evaluated statement value
  return llvm::cast<Function>(Builder.GetInsertBlock()->getParent());
}

Value* rootASTnode::codegen() {
  for (int i = 0; i<TopNodes.size(); i++){
    TopNodes[i]->codegen();
  }
  return nullptr;
}






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
  // getNextToken();
  // while (CurTok.type != EOF_TOK) {
  //   fprintf(stderr, "Token: %s with type %d\n", CurTok.lexeme.c_str(),
  //           CurTok.type);
  //   getNextToken();
  // }
  fprintf(stderr, "Lexer Finished\n");
  
 

  // Make the module, which holds all the code.
  TheModule = std::make_unique<Module>("mini-c", TheContext);

  // fseek(pFile,0,SEEK_SET);
  // clearTokBuffer(); //clear token buffer before re-reading file and starting parsing

  // lineNo = 1;
  // columnNo = 1;
//  fprintf(stderr, "Token: %s with type %d\n", CurTok.lexeme.c_str(),
//             CurTok.type);
  // Run the parser now.
  // getNextToken();
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
