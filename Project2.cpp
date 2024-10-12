#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <set>
#include <vector>
#include <assert.h>

// Below are some suggestions on how you might want to divide up your project.
// You may delete this and divide it up however you like.
#include "ASTNode.hpp"
#include "lexer.hpp"
#include "SymbolTable.hpp"

// Using
using std::string;
using std::endl;



class MacroCalc {
  private:
    std::vector<emplex::Token> tokens{};
    size_t token_id{0};
    ASTNode root{ASTNode::SCOPE};

    SymbolTable symbols{};

    std::string TokenName(int id) const {
      if (id > 0 && id < 128) {
        return std::string("'") + static_cast<char>(id) + "'";
      }
      return emplex::Lexer::TokenName(id);
    }

    emplex::Token CurToken() const { return tokens[token_id]; }

    emplex::Token UseToken() { return tokens[token_id++]; }

    emplex::Token UseToken(int required_id, std::string err_message="") {
      if (CurToken() != required_id) {
        if (err_message.size()) Error(CurToken(), err_message);
        else {
          Error(CurToken(),
            "Expected token type ", TokenName(required_id),
            ", but found ", TokenName(CurToken())
          );
        }
      }
      return UseToken();
    }

    bool UseTokenIf(int test_id) {
      if (CurToken() == test_id) {
        token_id++;
        return true;
      }
      return false;
    }

    ASTNode MakeVarNode(const emplex::Token & token) {
      size_t var_id = symbols.GetVarID(token.lexeme);
      assert(var_id < symbols.GetNumVars());
      ASTNode out(ASTNode::VARIABLE);
      out.SetVarID(var_id);
      return out;
    }

  public:
    MacroCalc(std::string filename) {
      std::ifstream file(filename);
      emplex::Lexer lexer;
      tokens = lexer.Tokenize(file);

      Parse();
    }

    void Parse() {
      while (token_id < tokens.size()) {
        ASTNode cur_node = ParseStatement();
        if (cur_node.GetType()) root.AddChild(cur_node);
      }
    }

    ASTNode ParseStatement() {
      switch (CurToken()) {
      using namespace emplex;
      case Lexer::ID_BEGINSCOPE : return ParseScope();
      case Lexer::ID_VAR : return ParseDeclare();
      case Lexer::ID_IDENTIFIER : return ParseAssign();
      case Lexer::ID_PRINT : return ParsePrint();
      case Lexer::ID_IF: return ParseIf();
      case Lexer::ID_WHILE: return ParseWhile();
      case Lexer::ID_SEMICOLON: return ASTNode{};
      default:
        return ParseExpression();
      }
    }

  ASTNode ParsePrint() {
    UseToken(emplex::Lexer::ID_PRINT);
    UseToken(emplex::Lexer::ID_OPENPAREN);

    ASTNode print_node{ASTNode::PRINT};

    //If the print argument is a string literal we add a node as a child
    if (CurToken() == emplex::Lexer::ID_STRINGLITERAL) {
      print_node.AddChild(ASTNode{ASTNode::STRING, CurToken().lexeme});
    }
    //If it's not as string literal it's assumed to be an expression and appended as a child
    else {
      print_node.AddChild(ParseExpression());
    }

    UseToken(emplex::Lexer::ID_CLOSEPAREN);
    UseToken(emplex::Lexer::ID_SEMICOLON);

    return print_node;
  }

  ASTNode ParseScope() {
    ASTNode scope(ASTNode::SCOPE);

    symbols.PushScope();

    while(CurToken() != emplex::Lexer::ID_ENDSCOPE) {
      scope.AddChild(ParseStatement());
    }
    symbols.PopScope();

    UseToken();
    return scope;
  }

  //Handles variable declarations ex: var x = 10;
  ASTNode ParseDeclare() {
    UseToken(emplex::Lexer::ID_VAR);
    auto id_token = UseToken(emplex::Lexer::ID_IDENTIFIER);
    symbols.AddVar(id_token.lexeme, id_token.line_id);

    if (UseTokenIf(emplex::Lexer::ID_SEMICOLON)) return ASTNode{};

    UseToken('=', "Expected ';' or '='.");

    auto lhs_node = MakeVarNode(id_token);
    auto rhs_node = ParseExpression();
    UseToken(';');

    return ASTNode{ASTNode::ASSIGN, lhs_node, rhs_node};

  }

  //Handles variable reassignment ex: x = 10;
  ASTNode ParseAssign() {
    auto id_token = UseToken(emplex::Lexer::ID_IDENTIFIER);

    UseToken(emplex::Lexer::ID_ASSIGN, "Expected '='.");

    auto lhs_node = MakeVarNode(id_token);
    auto rhs_node = ParseExpression();
    UseToken(';');

    return ASTNode{ASTNode::ASSIGN, lhs_node, rhs_node};
  }

  ASTNode ParseIf() {
    ASTNode if_node{ASTNode::IF};

    UseToken(emplex::Lexer::ID_IF);
    UseToken(emplex::Lexer::ID_OPENPAREN);

    //Parse the expression within the parenthesis
    if_node.AddChild(ParseExpression());
    UseToken(emplex::Lexer::ID_CLOSEPAREN);

    //If the if-statement has a begin scope we add a scope node
    if (UseTokenIf(emplex::Lexer::ID_BEGINSCOPE)) {
      if_node.AddChild(ParseScope());
    }
    //Otherwise we just parse the single statement
    else {
      if_node.AddChild(ParseStatement());
    }

    //If the statement has an else clause we apply the same logic as above
    if (UseTokenIf(emplex::Lexer::ID_ELSE)) {
      if (UseTokenIf(emplex::Lexer::ID_BEGINSCOPE)) {
        if_node.AddChild(ParseScope());
      }
      else {
        if_node.AddChild(ParseStatement());
      }
    }

    return if_node; 
  }

  ASTNode ParseWhile() {
    UseToken(emplex::Lexer::ID_WHILE);
    UseToken(emplex::Lexer::ID_OPENPAREN);

    ASTNode while_node{ASTNode::WHILE};

    while_node.AddChild(ParseExpression());

    UseToken(emplex::Lexer::ID_CLOSEPAREN);

    //Since while loops don't need to have a body or statement we can return just the parsed expression child
    if (UseTokenIf(emplex::Lexer::ID_SEMICOLON)) return while_node;

    //If it does not end in a semi-colon we check if there is a beginscope token
    //If there is we add a beginscope node
    if (UseTokenIf(emplex::Lexer::ID_BEGINSCOPE)) {
      while_node.AddChild(ParseScope());
    }
    //Otherwise we just parse the single statement
    else {
      while_node.AddChild(ParseStatement());
    }

    return while_node;
  }

  ASTNode ParseExpression() {
    
  }

  double Run(const ASTNode& node) {
  switch (node.GetType()) {
    case ASTNode::NUMBER:
      return node.GetValue();

    case ASTNode::VARIABLE: {
      const std::string &name = node.GetStrValue();
      if (!symbols.HasVar(name)) {
        Error(0, "Undefined variable '", name, "'.");
      }
      return symbols.GetValue(name);
    }

    case ASTNode::VAR: {
      const std::string &name = node.GetStrValue();
      if (symbols.HasVar(name)) {
        Error(0, "Variable '", name, "' already declared in this scope.");
      }
      symbols.AddVar(name, 0.0);  // Default initialization
      return 0.0;
    }

    case ASTNode::ASSIGN: {
      double rhs_value = Run(node.GetChild(1));
      const ASTNode& lhs = node.GetChild(0);
      if (lhs.GetType() != ASTNode::VARIABLE) {
        Error(0, "Assignment target must be a variable.");
      }
      symbols.SetValue(lhs.GetStrValue(), rhs_value);
      return rhs_value;
    }

    case ASTNode::PRINT: {
      for (const auto &child : node.GetChildren()) {
        if (child.GetType() == ASTNode::STRING) {
          std::cout << child.GetStrValue();
        } else {
          std::cout << Run(child);
        }
      }
      std::cout << std::endl;
      return 0.0;
    }

    case ASTNode::IF: {
      if (Run(node.GetChild(0)) != 0.0) {
        return Run(node.GetChild(1));
      } else if (node.GetChildren().size() > 2) {
        return Run(node.GetChild(2));
      }
      return 0.0;
    }

    case ASTNode::WHILE: {
      while (Run(node.GetChild(0)) != 0.0) {
        Run(node.GetChild(1));
      }
      return 0.0;
    }

    case ASTNode::EXPR:
      // Expression handling logic can go here, handling operations based on children. (EG)
      return 0.0;  // Placeholder for now

    case ASTNode::EMPTY:
    default:
      return 0.0;
    }
  }

  void Run() { Run(root); }
};


int main(int argc, char * argv[])
{
  if (argc != 2) {
    std::cout << "Format: " << argv[0] << " [filename]" << std::endl;
    exit(1);
  }

  std::string filename = argv[1];
  
  std::ifstream in_file(filename);              // Load the input file
  if (in_file.fail()) {
    std::cout << "ERROR: Unable to open file '" << filename << "'." << std::endl;
    exit(1);
  }

  MacroCalc mc(argv[1]);
  mc.Run();
  
}

