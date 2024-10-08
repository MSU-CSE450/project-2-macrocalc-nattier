#pragma once

#include <assert.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

class SymbolTable {
private:
  // CODE TO STORE SCOPES AND VARIABLES HERE.
      // Structure to store variable information (EG)
    struct VarData {
        double value;
        size_t line_num;  // Line number for error reporting
    };
    // Stack of scopes, each scope is a map from variable name to VarData
    std::vector<std::unordered_map<std::string, VarData>> scopes;
  
  // HINT: YOU CAN CONVERT EACH VARIABLE NAME TO A UNIQUE ID TO CLEANLY DEAL
  //       WITH SHADOWING AND LOOKING UP VARIABLES LATER.

public:
  // CONSTRUCTOR
  // Inits to the "Global scope"
  SymbolTable() {
    PushScope();  // Start with a global scope
  }

  // Push a new scope onto the stack (EG)
  void PushScope() {
    scopes.emplace_back();
  }

  // Pop the top scope off the stack (EG)
  void PopScope() {
    if (!scopes.empty()) {
      scopes.pop_back();
    } 
    else {
      throw std::runtime_error("ERROR: No scope to pop.");
    }
  }

  // MANAGING VARS
  /**
   * Function that searches all scopes to find a var (EG)
   * Searches from the top scope down to the bottom scope
   * @return True if var is found. 
   */
  bool HasVar(std::string name) const { 
    // iterate through the levels of scopes
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      // try to count the var
      if (it->count(name) > 0) {
        return true;
      }
    }
    return false; 
  }

  /**
   * Check if a variable exists only in the current scope (EG)
   * @return True if var is found. 
   */
  bool HasVarInCurrentScope(const std::string &name) const {
      if (scopes.empty()) return false;
      return scopes.back().count(name) > 0;
  }

  // HAVE NOT SET UP
  size_t AddVar(std::string name, size_t line_num) { return 0; }

  // Get the value of a variable by searching from top to bottom scopes (EG)
  double GetValue(const std::string &name) const {
    assert(HasVar(name));
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto varIt = it->find(name);
      if (varIt != it->end()) {
        return varIt->second.value;
      }
    }
    throw std::runtime_error("ERROR: Undefined variable '" + name + "'.");
  }

  // Set the value of a variable in the top-most scope where it exists (EG)
  void SetValue(const std::string &name, double value) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto varIt = it->find(name);
      if (varIt != it->end()) {
        varIt->second.value = value;
        return;
      }
    }
    throw std::runtime_error("ERROR: Cannot assign to undeclared variable '" + name + "'.");
  }
};
