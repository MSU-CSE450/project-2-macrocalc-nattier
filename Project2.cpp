#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
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
      UseToken();
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
    UseToken(emplex::Lexer::ID_BEGINSCOPE);

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

    UseToken(emplex::Lexer::ID_ASSIGN, "Expected ';' or '='.");

    auto lhs_node = MakeVarNode(id_token);
    auto rhs_node = ParseExpression();
    UseToken(emplex::Lexer::ID_SEMICOLON);

    return ASTNode{ASTNode::ASSIGN, lhs_node, rhs_node};

  }

  //Handles variable reassignment ex: x = 10;
  ASTNode ParseAssign() {
    auto id_token = UseToken(emplex::Lexer::ID_IDENTIFIER);

    UseToken(emplex::Lexer::ID_ASSIGN, "Expected '='.");

    auto lhs_node = MakeVarNode(id_token);
    auto rhs_node = ParseExpression();
    UseToken(emplex::Lexer::ID_SEMICOLON);

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
    if (CurToken() == emplex::Lexer::ID_BEGINSCOPE) {
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
    if (CurToken() == emplex::Lexer::ID_BEGINSCOPE) {
      while_node.AddChild(ParseScope());
    }
    //Otherwise we just parse the single statement
    else {
      while_node.AddChild(ParseStatement());
    }

    return while_node;
  }

  ASTNode ParseExpression() {
    ASTNode expressionNode =  ParseExpressionAssign();//ASTNode{ASTNode::EXPR};
    /*while (token_id < tokens.size())
    {
      auto rhs = ParseExpressionAssign();
      expressionNode.AddChild(rhs);
    }*/
    //std::cout << "debug tree printing: " << std::endl;
    //DebugASTCheck(expressionNode, 0); //Remove when printing is ready
    //std::cout << "_________" << std::endl;
    return expressionNode;
  }

  void DebugASTCheck(ASTNode test_node, int number)
  {
    //Should be able to display the tree when called

    auto children = test_node.GetChildren();
    int num2 = number + 1;

    for (int i = 0; i < number; i++)
    {
      std::cout << " . ";
    }
    std::cout << test_node.GetStrValue() << std::endl;
    for (int i = 0; i < (int)children.size(); i++)
    {
      DebugASTCheck(children.at(i), num2);
    }
  }

  ASTNode ParseExpressionValue() {
    // Check for unary minus (negative number)
    bool isNegative = false;
    if (CurToken().lexeme == "-") {
      isNegative = true;
      UseToken(); // Consume the '-'
    }

    // Get the current token to determine if it's an identifier or a number
    auto old_node = CurToken();
    ASTNode cur_node;

    if (old_node.id == emplex::Lexer::ID_IDENTIFIER) {
      // The token is an identifier, so treat it as a VARIABLE node
      cur_node = ASTNode{ASTNode::VARIABLE};
      UseToken();  // Consume the identifier token

      // Set the name of the variable in the AST node
      cur_node.SetStrValue(old_node.lexeme);

      // Check if the variable is declared in the symbol table
      if (!symbols.HasVar(old_node.lexeme)) {
        // Throw an error if the variable is undeclared (hopefully)
        Error(old_node.line_id, "Undeclared variable '", old_node.lexeme, "' used in expression.");
      }
      
      // Store the variable's ID for further reference if needed?
      cur_node.SetVarID(symbols.GetVarID(old_node.lexeme));
    } else if (old_node.id == emplex::Lexer::ID_INT || old_node.id == emplex::Lexer::ID_FLOAT) {
      // The token is a numeric literal, so treat it as a NUMBER node
      cur_node = ASTNode{ASTNode::NUMBER};
      UseToken();  // Consume the number token

      // Parse the lexeme as a double, applying the negative sign if necessary
      double value = std::stod(old_node.lexeme);
      cur_node.SetValue(isNegative ? -value : value);
    } else {
      // If the token is neither an identifier nor a number, it's an error
      Error(old_node.line_id, "Expected a variable or number but found '", old_node.lexeme, "'.");
    }

    return cur_node;  // Return the constructed node
  }

  ASTNode ParseExpressionExponentiate() {
    //This Needs some fixing
    
    ASTNode lhs = ParseExpressionValue();
    //Right
    if (CurToken().lexeme == "**")
    {
      int token = UseToken();
      ASTNode rhs = ParseExpressionExponentiate(); // Recurse down the right
      ASTNode resultNode{ASTNode::MATH_OP, lhs, rhs};
      resultNode.SetValue(token);  // Optional, depending on how SetValue is used in your code
      resultNode.SetStrValue("**");
      
      return resultNode;
      //DebugPrint("Exponential");
    }
    return lhs;
  }

  ASTNode ParseExpressionMultDivMod() {
    //Left
    ASTNode lhs = ParseExpressionExponentiate();
    while (CurToken().lexeme == "*" || CurToken().lexeme == "/" || CurToken().lexeme == "%")
    {
      std::string lexeme_old = CurToken().lexeme;
      int token = UseToken();
      ASTNode rhs = ParseExpressionExponentiate();
      lhs = ASTNode{ASTNode::MATH_OP, lhs, rhs};
      lhs.SetValue(token);
      lhs.SetStrValue(lexeme_old);
      //DebugPrint(lexeme_old);
    }
    return lhs;
  }

  ASTNode ParseExpressionAddSub() {
    //Left
    ASTNode lhs = ParseExpressionMultDivMod();
    while (CurToken().lexeme == "+" || CurToken().lexeme == "-")
    {
      std::string lexeme_old = CurToken().lexeme;
      int token = UseToken();
      
      ASTNode rhs = ParseExpressionMultDivMod();
      lhs = ASTNode{ASTNode::MATH_OP, lhs, rhs};
      lhs.SetValue(token);
      lhs.SetStrValue(lexeme_old);
      //DebugPrint(lexeme_old);
    }
    return lhs;
  }

  ASTNode ParseExpressionCompare() {
    //This might need some fixing
    //None
    ASTNode lhs = ParseExpressionAddSub();
    while (CurToken().lexeme == ">" || CurToken().lexeme == "<" || CurToken().lexeme == ">=" || CurToken().lexeme == "<=") {
      std::string lexeme_old = CurToken().lexeme;
      int token = UseToken();
      ASTNode rhs = ParseExpressionAddSub();

      if (CurToken().lexeme == ">" || CurToken().lexeme == "<" || CurToken().lexeme == ">=" || CurToken().lexeme == "<=") {
        Error(0, "Chaining of non-associative comparison operators is not allowed.");
      }

      lhs = ASTNode{ASTNode::COMP_OP, lhs, rhs};
      lhs.SetValue(token);
      lhs.SetStrValue(lexeme_old);
      //DebugPrint(lexeme_old);
    }
    return lhs;
  }
  
  ASTNode ParseExpressionEquality() {
    //This may need some fixing
    //None
    ASTNode lhs = ParseExpressionCompare();
    while (CurToken().lexeme == "!=" || CurToken().lexeme == "==") {
      std::string lexeme_old = CurToken().lexeme;
      int token = UseToken();
      ASTNode rhs = ParseExpressionCompare();

      if (CurToken().lexeme == "!=" || CurToken().lexeme == "==") {
        Error(0, "Chaining of equality operators is not allowed.");
      }

      lhs = ASTNode{ASTNode::COMP_OP, lhs, rhs};
      lhs.SetValue(token);
      lhs.SetStrValue(lexeme_old);
      //DebugPrint(lexeme_old);
    }
    return lhs;
  }

  ASTNode ParseExpressionAnd() {
    //This might need some fixing
    //Left
    ASTNode lhs = ParseExpressionEquality();
    if (CurToken().lexeme == "&&") {
      int token = UseToken();
      ASTNode rhs = ParseExpressionEquality();
      lhs = ASTNode{ASTNode::EXPR, lhs, rhs};
      lhs.SetValue(token);
      lhs.SetStrValue("&&");
      //DebugPrint("left and");
    }
    return lhs;
  }

  ASTNode ParseExpressionOr() {
    //Left
    ASTNode lhs = ParseExpressionAnd();

    if (CurToken().lexeme == "||") {
      int token = UseToken();
      ASTNode rhs = ParseExpressionAnd();
      lhs = ASTNode{ASTNode::EXPR, lhs, rhs};
      lhs.SetValue(token);
      lhs.SetStrValue("||");
      //DebugPrint("left or");
    }
    return lhs;
  }

  ASTNode ParseExpressionAssign() {
    //Right
    ASTNode lhs = ParseExpressionOr();
    if (CurToken().lexeme == "=")
    {      
      ASTNode rhs = ParseExpressionAssign();  // Right associative.
      //DebugPrint("right assign");
      rhs.SetStrValue("=");
      return ASTNode(ASTNode::ASSIGN, lhs, rhs);
    }
    return lhs;
  }


  void DebugTokenCheck()
  {
    //DebugPrint(CurToken().lexeme);
  }
  void DebugPrint(std::string type)
  {
    std::cout << type << std::endl;
  }

// No longer used and marked for deletion
double EvaluateExpression(const ASTNode& node) {
  switch (node.GetType()) {
    // Return the value of a number
    case ASTNode::NUMBER:
      return node.GetValue();
    // Return value from symbol table
    case ASTNode::VARIABLE:
      return symbols.GetValue(node.GetStrValue());
    // Handle math_op separately 
    case ASTNode::MATH_OP:
      return EvaluateMathOp(node);
    // Handle comp_op separately
    case ASTNode::COMP_OP:
      return EvaluateCompOp(node);
    // Handle Logic ops separately
    case ASTNode::EXPR:
      return EvaluateLogicalOp(node);

    default:
      Error(0, "Unexpected expression node type.");
      return 0.0;
    }
}

// No longer used and marked for deletion
double EvaluateMathOp(const ASTNode& node) {
  // Get both child nodes
  const double lhs = EvaluateExpression(node.GetChild(0));
  const double rhs = EvaluateExpression(node.GetChild(1));

  if (node.GetStrValue() == "+") {
    return lhs + rhs;
  } else if (node.GetStrValue() == "-") {
    return lhs - rhs;
  } else if (node.GetStrValue() == "*") {
    return lhs * rhs;
  } else if (node.GetStrValue() == "/") {
    if (rhs == 0.0) Error(0, "Division by zero.");
    return lhs / rhs;
  } else if (node.GetStrValue() == "%") {
    if (rhs == 0.0) Error(0, "Modulus by zero.");
    return static_cast<int>(lhs) % static_cast<int>(rhs);
  } else if (node.GetStrValue() == "**") {
    return pow(lhs, rhs);
  }

  Error(0, "Unknown math operator '", node.GetStrValue(), "'.");
  return 0.0;
  }

  // No longer used and marked for deletion?
  double EvaluateCompOp(const ASTNode& node) {
    const double lhs = EvaluateExpression(node.GetChild(0));
    const double rhs = EvaluateExpression(node.GetChild(1));

    if (node.GetStrValue() == "<") {
      return lhs < rhs ? 1.0 : 0.0;
    } else if (node.GetStrValue() == "<=") {
      return lhs <= rhs ? 1.0 : 0.0;
    } else if (node.GetStrValue() == ">") {
      return lhs > rhs ? 1.0 : 0.0;
    } else if (node.GetStrValue() == ">=") {
      return lhs >= rhs ? 1.0 : 0.0;
    } else if (node.GetStrValue() == "==") {
      return lhs == rhs ? 1.0 : 0.0;
    } else if (node.GetStrValue() == "!=") {
      return lhs != rhs ? 1.0 : 0.0;
    }

    Error(0, "Unknown comparison operator '", node.GetStrValue(), "'.");
    return 0.0;
  }

  // No longer used and marked for deletion
  double EvaluateLogicalOp(const ASTNode& node) {
    // Only grab lhs incase we short-circuit
    const double lhs = EvaluateExpression(node.GetChild(0));
    
    if (node.GetStrValue() == "&&") {
      if (lhs == 0.0) return 0.0;  // Short-circuit if lhs is false
      return EvaluateExpression(node.GetChild(1)) != 0.0 ? 1.0 : 0.0;
    } else if (node.GetStrValue() == "||") {
      if (lhs != 0.0) return 1.0;  // Short-circuit if lhs is true
      return EvaluateExpression(node.GetChild(1)) != 0.0 ? 1.0 : 0.0;
    }

    Error(0, "Unknown logical operator '", node.GetStrValue(), "'.");
    return 0.0;
  }

  double Run(const ASTNode& node) {
    switch (node.GetType()) {
      case ASTNode::SCOPE: {
        for (auto & child : node.GetChildren()) {
          Run(child);
        }
      }

      // Return numeric values directly
      case ASTNode::NUMBER: {
        return node.GetValue();
      }

      // Retrieve and return the value of a var from the symbol table
      case ASTNode::VARIABLE: {
        const size_t var_id = node.GetVarID();
        auto test = symbols.VarValue(var_id).value;
        return symbols.VarValue(var_id).value;
      }

      // Handle variable declarations by adding to symbol table
      case ASTNode::VAR: {
        const std::string &name = node.GetStrValue();
        symbols.AddVar(name, 0.0);  // Default initialization
        return 0.0;
      }

      // Assign the result of an EXPR to a variable
      case ASTNode::ASSIGN: {
        double rhs_value = Run(node.GetChild(1)); // Get RHS
        const ASTNode& lhs = node.GetChild(0);
        symbols.SetVarValue(lhs.GetVarID(), rhs_value);
        return rhs_value;
      }

      // Handle both Strings and expressions in print (EG) 
      // I need to test this to see if it makes sense.
      case ASTNode::PRINT: {
        // Loop through all the children of the node.
        // A PRINT node can have multiple children, representing either strings or expressions.
        for (const auto &child : node.GetChildren()) {
            
            // If the child is a STRING node, we need to process it as a LITERAL string.
            if (child.GetType() == ASTNode::STRING) {
                std::string output = child.GetStrValue(); // Get the string value of the child
                
                // We now check for any variables within the string that are marked by braces, like: "{something}".
                size_t pos = 0;
                
                // While there are still open braces '{' in the string....
                while ((pos = output.find('{', pos)) != std::string::npos) {
                    // Find the closing brace '}'.
                    size_t endPos = output.find('}', pos);
                    
                    // If there is no closing brace, we stop processing 
                    if (endPos == std::string::npos) break; 
                    
                    // Get just the name from the brackets
                    std::string var_name = output.substr(pos + 1, endPos - pos - 1);
                    
                    // Get the value from symbol table
                    double var_value = symbols.GetValue(var_name);

                    std::ostringstream oss;
                    oss << var_value;
                    
                    // Replace the "{variable}" in the string with the actual value 
                    output.replace(pos, endPos - pos + 1, oss.str());
                    
                    // Move the position forward to continue checking for more variables in the string.
                    // Move it forward the length of the string we replaced with
                    pos += oss.str().length();
                }
                //Remove "" from string
                std::erase(output, '"');

                //Output processes string
                std::cout << output;
            } 
            else {
                // If the child is not a string (it's an expression then), evaluate it 
                std::cout << Run(child);
            }
         }
    
        // After printing all children, newline
        std::cout << std::endl;
        return 0.0;
      }

      case ASTNode::IF: {
        if (Run(node.GetChild(0)) != 0.0) {
          return Run(node.GetChild(1)); // Run "IF" branch
        } else if (node.GetChildren().size() > 2) {
          return Run(node.GetChild(2)); // Run "Else" branch
        }
        return 0.0;
      }

      // Run while loop with repeated checks on condition
      case ASTNode::WHILE: {
        while (Run(node.GetChild(0)) != 0.0) { // Check condition 
          Run(node.GetChild(1)); // Execute body
        }
        return 0.0;
      }

      case ASTNode::EXPR:
        return EvaluateExpression(node);


      case ASTNode::MATH_OP: {
        // Evaluate left and right sub-expressions first (recursion)
        double lhs_value = Run(node.GetChild(0));
        double rhs_value = Run(node.GetChild(1));
        // Get the OP
        const std::string &op = node.GetStrValue();
        // Perform the operation based on the operator string
          if (op == "+") return lhs_value + rhs_value;
          else if (op == "-") return lhs_value - rhs_value;
          else if (op == "*") return lhs_value * rhs_value;
          else if (op == "/") {
            if (rhs_value == 0) {
                std::cerr << "ERROR: Division by zero." << std::endl;
                exit(1);
            }
            return lhs_value / rhs_value;
          }
          else if (op == "%") {
            if (rhs_value == 0) {
                std::cerr << "ERROR: Modulus by zero." << std::endl;
                exit(1);
            }
            return std::fmod(lhs_value, rhs_value);
          }
          else if (op == "**") return std::pow(lhs_value, rhs_value);
          else {
            std::cerr << "ERROR: Unknown operator '" << op << "'." << std::endl;
            exit(1);
          }
        }

      case ASTNode::COMP_OP: {
        // Evaluate left and right sub-expressions first (recursion)
        double lhs_value = Run(node.GetChild(0));
        double rhs_value = Run(node.GetChild(1));
        // Get the op
        const std::string &op = node.GetStrValue();
        // Run T/F based on lhs and rhs, return 1.0 (T) or 0.0 (F)
        if (op == "<") return lhs_value < rhs_value ? 1.0 : 0.0;
        else if (op == "<=") return lhs_value <= rhs_value ? 1.0 : 0.0;
        else if (op == ">") return lhs_value > rhs_value ? 1.0 : 0.0;
        else if (op == ">=") return lhs_value >= rhs_value ? 1.0 : 0.0;
        else if (op == "==") return lhs_value == rhs_value ? 1.0 : 0.0;
        else if (op == "!=") return lhs_value != rhs_value ? 1.0 : 0.0;
        else {
          std::cerr << "ERROR: Unknown operator '" << op << "'." << std::endl;
          exit(1);
        }
      }

      // Shouldn't have any EMPTY
      case ASTNode::EMPTY:
        std::cerr << "ERROR: Detected EMPTY node" << std::endl;

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
