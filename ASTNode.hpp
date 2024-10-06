#pragma once

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

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
    EMPTY=0,
    VARIABLE,
    PRINT
  };

private:
  Type type{EMPTY};
  size_t value{0};
  words_t words{};
  std::vector<ASTNode> children{};

// Public member functions
public:
  // Constructors
  ASTNode(Type type=EMPTY) : type(type) { }
  ASTNode(Type type, size_t value) : type(type), value(value) { }
  ASTNode(Type type, words_t words) : type(type), words(words) { }
  ASTNode(Type type, ASTNode child) : type(type) {AddChild(child);}
  ASTNode(Type type, ASTNode child1, ASTNode child2) 
    : type(type) { AddChild(child1); AddChild(child2); }

  // Copy constructor
  ASTNode(const ASTNode &) = default;
  // Move constructor
  ASTNode(ASTNode &&) = default;
  // Copy operator
  ASTNode & operator=(const ASTNode &) = default;
  // Move operator
  ASTNode & operator=(ASTNode &&) = default;
  // Destructor
  ~ASTNode() { }

  // Getters 
  Type GetType() const {return type;}
  size_t GetValue() const {return value;}
  const words_t & GetWords() const {return words;}
  const std::vector<ASTNode> & GetChildren() const {return children;}
  // Get a particular child
  const ASTNode & GetChild(size_t id) const {
    // Ensure a child exists
    assert(id < children.size());
    return children[id];
  }

  void SetValue(size_t in) { value = in; }
  void SetWords(words_t in) { words = in; }
  void AddChild(ASTNode child) {
    // WE SHOULD NEVER INSERT A CHILD that has NOTHING to it
    assert(child.GetType() != EMPTY);
    children.push_back(child);
  }
  
  // CODE TO EXECUTE THIS NODE (AND ITS CHILDREN, AS NEEDED).
  double Run(SymbolTable & symbols) { ; }

};
