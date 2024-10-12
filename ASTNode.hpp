#pragma once

#include <cmath>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <cassert>

#include "SymbolTable.hpp"


/**
 * Class representing our nodes in the tree. (EG)
 * Need to add a Type for every type in the tree
 */
class ASTNode 
{
public:
  // All types of nodes we have in our tree
  enum Type {
    EMPTY = 0,
    SCOPE,
    VARIABLE,
    LITERAL,
    STRING,
    VAR,
    ASSIGN,
    PRINT,
    IF,
    WHILE,
    EXPR,
  };

private:
  Type type{EMPTY};
  double value; // For string literals
  std::string str_value;  // For string literals and variable names
  std::vector<ASTNode> children{};

// Public member functions
public:
  // Constructors
  ASTNode(Type type = EMPTY) 
    : type(type), value(0.0), str_value("") {}

  ASTNode(Type type, double value) 
    : type(type), value(value), str_value("") {}

  ASTNode(Type type, const std::string &str_value) 
    : type(type), value(0.0), str_value(str_value) {}

  // Constructor with one child
  ASTNode(Type type, ASTNode child) : type(type), value(0.0), str_value("") 
  {
    AddChild(child);
  }

  // Constructor with two children
  ASTNode(Type type, ASTNode child1, ASTNode child2) 
      : type(type), value(0.0), str_value("") 
  {
      AddChild(child1);
      AddChild(child2);
  }

  // Copy constructor
  ASTNode(const ASTNode &) = default;
  // Move constructor
  ASTNode(ASTNode &&) = default;
  // Copy operator
  ASTNode & operator=(const ASTNode &) = default;
  // Move operator
  ASTNode & operator=(ASTNode &&) = default;
  // Destructor
  ~ASTNode() = default;

  // Type getter
  Type GetType() const { return type; }
  // Value getters
  double GetValue() const { return value; }
  // String getter
  const std::string &GetStrValue() const { return str_value; }

  // Children management (daycare)
  const std::vector<ASTNode> & GetChildren() const {return children;}

  // Add child
  void AddChild(ASTNode child) {
    assert(child.GetType() != EMPTY);
    children.push_back(child);
  }

  // Get specific child
  const ASTNode &GetChild(size_t id) const {
    assert(id < children.size());
    return children[id];
  }

  // value setters
  void SetValue(double in) { value = in; }
  void SetStrValue(const std::string &in) { str_value = in; }

};
