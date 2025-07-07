#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <limits>

constexpr double INFINITY = std::numeric_limits<double>::infinity();

enum class OptType { MAXIMIZE, MINIMIZE };

struct Term {
  double coefficient;
  std::string variable;
};

struct LinearExpression {
  std::vector<Term> terms;
  double rhs = 0.0;
  std::string op; // <=, >=, =
  int lineNumber;
};

enum class VarType {
  CONTINUOUS,
  INTEGER,
  BINARY
};

struct Bound {
  double lower = -INFINITY;
  double upper = INFINITY;
  bool isFree = false;
  VarType type = VarType::CONTINUOUS;
};


struct LPModel {
  OptType type;
  LinearExpression objective;
  std::vector<LinearExpression> constraints;
  std::unordered_map<std::string, Bound> bounds;
};

class Parser {
public:
  static LPModel parseFile(const std::string& path);
};
