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
// Used to represent a collection of words (literal string)
using words_t = std::set<std::string>;


/** 
*  Error function. (EG)
*  Pass in an error message with any number of arguments
*  Example use: Error(17, "Invalid value of x; x = ", x, ".");
*/
template <typename... Ts>
void Error(size_t line_num, Ts... message) 
{
  std::cerr << "ERROR (Line " << line_num << "): ";
  (std::cerr << ... << message);
  std::cerr << endl;
  // exit with non-zero 
  exit(1);
}

/** 
*  Error function with token. (EG)
*  Pass in an error message with any number of arguments
*  Example use: Error(tokens[token_id], "Invalid value of y; y = ", y, ".");
*/
template <typename... Ts>
void Error(emplex::Token token, Ts... message) 
{
  Error(token.line_id, message...);
}

/**
 * Function used to EXECUTE an ASTNode (EG)
 * NOTE: Expressions DO NOT WORK
 *     we need to write the logic for that
 */
double Run(const ASTNode& node, SymbolTable& symbols) {
  switch (node.GetType()) {
    case ASTNode::LITERAL:
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
      double rhs_value = Run(node.GetChild(1), symbols);
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
          std::cout << Run(child, symbols);
        }
      }
      std::cout << std::endl;
      return 0.0;
    }

    case ASTNode::IF: {
      if (Run(node.GetChild(0), symbols) != 0.0) {
        return Run(node.GetChild(1), symbols);
      } else if (node.GetChildren().size() > 2) {
        return Run(node.GetChild(2), symbols);
      }
      return 0.0;
    }

    case ASTNode::WHILE: {
      while (Run(node.GetChild(0), symbols) != 0.0) {
        Run(node.GetChild(1), symbols);
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

  // TO DO:  
  // PARSE input file to create Abstract Syntax Tree (AST).
  // EXECUTE the AST to run your program.
      // Step 1: Lexing
    emplex::Lexer lexer;
    std::vector<emplex::Token> tokens = lexer.Tokenize(in_file);

    // Step 2: Parsing Placeholder (EG)
    //ASTNode root = Parse(tokens); 

    // Step 3: Running
    SymbolTable symbols;
    // NO ROOT YET becasue of Parse not yet happening
    //Run(root, symbols);
  
}

