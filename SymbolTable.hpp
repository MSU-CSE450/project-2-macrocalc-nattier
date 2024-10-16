#pragma once

#include <assert.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

#include "lexer.hpp"

// Using
using std::string;
using std::endl;

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

class SymbolTable {
private:
  // CODE TO STORE SCOPES AND VARIABLES HERE.
      // Structure to store variable information (EG)
    struct VarData {
        std::string name;
        double value;
        size_t line_num;  // Line number for error reporting

        VarData(std::string name, size_t line_num)
      : name(name), line_num(line_num) { }
    };
    // Stack of scopes, each scope is a map from variable name to VarData
    //std::vector<std::unordered_map<std::string, VarData>> scopes;

    //Vector of all variables in program
    std::vector<VarData> var_info;

    //Stack of scopes with each scope being a map from variable name to it's index in var_info
    using scope_t = std::unordered_map<std::string, size_t>;
    std::vector<scope_t> scopes{1};
  
  // HINT: YOU CAN CONVERT EACH VARIABLE NAME TO A UNIQUE ID TO CLEANLY DEAL
  //       WITH SHADOWING AND LOOKING UP VARIABLES LATER.

public:

  static constexpr size_t NO_ID = static_cast<size_t>(-1);

  size_t GetNumVars() const { return var_info.size(); }

  //Returns the variable ID of the innermost scope
  //Returns -1 if no variable was found
  size_t GetVarID(std::string name) const {
    for (auto it = scopes.rbegin();
         it != scopes.rend();
         ++it)
    {
      if (it->count(name)) return it->find(name)->second;
    }

    return NO_ID;
  }

  //Checks if a variable exists in any scope and returns true or false
  bool HasVar(std::string name) const {
    return (GetVarID(name) != NO_ID);
  }

  // Adds a variable to var_info vector and sets it in the scope (SP)
  size_t AddVar(std::string name, size_t line_num) {
    auto & scope = scopes.back();
    if (scope.count(name)) {
      Error(line_num, "Redeclaration of variable '", name, "'.");
    }
    size_t var_id = var_info.size();
    var_info.emplace_back(name, line_num);
    scope[name] = var_id;
    return var_id;
  }

  //Returns a VarData struct using it's id(index) in the var_info vector
  VarData & VarValue(size_t id) {
    assert(id < var_info.size());
    return var_info[id];
  }

  void SetVarValue(size_t id, double val) {
    var_info[id].value = val;
  }

  // Push a new scope onto the stack (EG)
  void PushScope() {
    scopes.emplace_back();
  }

  // Pop the top scope off the stack (EG)
  void PopScope() {
    assert(scopes.size() > 1);
    scopes.pop_back();
  }



  //OLD FUNCTIONS, MAY NOT NEED
  // /**
  //  * Function that searches all scopes to find a var (EG)
  //  * Searches from the top scope down to the bottom scope
  //  * @return True if var is found. 
  //  */
  // bool HasVar(std::string name) const { 
  //   // iterate through the levels of scopes
  //   for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
  //     // try to count the var
  //     if (it->count(name) > 0) {
  //       return true;
  //     }
  //   }
  //   return false; 
  // }

  /**
   * Check if a variable exists only in the current scope (EG)
   * @return True if var is found. 
   */
  bool HasVarInCurrentScope(const std::string &name) const {
      if (scopes.empty()) return false;
      return scopes.back().count(name) > 0;
  }

  // Get the value of a variable by searching from top to bottom scopes (EG)
  double GetValue(const std::string &name) const {
    assert(HasVar(name));
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto varIt = it->find(name);
      if (varIt != it->end()) {
        return var_info[varIt->second].value;
      }
    }
    throw std::runtime_error("ERROR: Undefined variable '" + name + "'.");
  }

  // Set the value of a variable in the top-most scope where it exists (EG)
  void SetValue(const std::string &name, double value) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto varIt = it->find(name);
      if (varIt != it->end()) {
        var_info[varIt->second].value = value;
        return;
      }
    }
    throw std::runtime_error("ERROR: Cannot assign to undeclared variable '" + name + "'.");
  }
};
