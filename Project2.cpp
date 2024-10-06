#include <assert.h>
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
  
}

