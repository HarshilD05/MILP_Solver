#include "parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <stdexcept>
#include <cctype>

using namespace std;

namespace {
  /*
   * Function: trim
   * -------------------------
   * Removes leading and trailing whitespace from the string.
   */
  string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
  }

  /*
   * Function: split
   * -------------------------
   * Splits a string by a given delimiter and trims each token.
   */
  vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    stringstream ss(s);
    string item;
    while (getline(ss, item, delimiter)) {
      tokens.push_back(trim(item));
    }
    return tokens;
  }

  /*
   * Function: isBlank
   * -------------------------
   * Checks if a string is empty or contains only whitespace.
   */
  bool isBlank(const string& s) {
    return trim(s).empty();
  }

  /*
   * Function: isOperator
   * -------------------------
   * Checks if a string is a valid comparison operator (<=, >=, =).
   */
  bool isOperator(const string& s) {
    return s == "<=" || s == ">=" || s == "=";
  }

  /*
   * Function: parseTerm
   * -------------------------
   * Parses a term like "3x" into its coefficient and variable name.
   */
  Term parseTerm(const string& token, int line) {
    smatch match;
    regex termPattern(R"(([+-]?\d*\.?\d*)([a-zA-Z_][a-zA-Z0-9_]*))");
    if (!regex_match(token, match, termPattern)) {
      throw runtime_error("Line " + to_string(line) + ": Invalid term format: '" + token + "'");
    }

    double coeff = 1.0;
    string coeffStr = match[1];
    if (!coeffStr.empty() && coeffStr != "+" && coeffStr != "-") {
      coeff = stod(coeffStr);
    }
    else if (coeffStr == "-") {
      coeff = -1;
    }

    return Term{ coeff, match[2] };
  }

  /*
   * Function: parseExpression
   * -------------------------
   * Parses a linear expression like "3x + 2y - z" into a list of Term objects.
   */
  vector<Term> parseExpression(const string& exprStr, int line) {
    vector<Term> terms;
    regex tokenPattern(R"(([+-]?\s*\d*\.?\d*[a-zA-Z_][a-zA-Z0-9_]*))");
    auto begin = sregex_iterator(exprStr.begin(), exprStr.end(), tokenPattern);
    auto end = sregex_iterator();

    for (auto i = begin; i != end; ++i) {
      string token = regex_replace(i->str(), regex(R"(\s+)"), ""); // remove inner spaces
      terms.push_back(parseTerm(token, line));
    }

    if (terms.empty()) {
      throw runtime_error("Line " + to_string(line) + ": No valid terms found in expression");
    }

    return terms;
  }

  /*
   * Function: parseConstraint
   * -------------------------
   * Parses a constraint like "x + 2y <= 10" into a LinearExpression.
   */
  LinearExpression parseConstraint(const string& lineStr, int line) {
    smatch match;
    regex constraintPattern(R"((.+)(<=|>=|=)(.+))");
    if (!regex_match(lineStr, match, constraintPattern)) {
      throw runtime_error("Line " + to_string(line) + ": Invalid constraint format.");
    }

    string lhs = match[1];
    string op = match[2];
    string rhsStr = match[3];

    vector<Term> terms = parseExpression(lhs, line);
    double rhs = stod(rhsStr);

    return LinearExpression{ terms, rhs, op, line };
  }

} // anonymous namespace



/*
 * Function: parseFile
 * -------------------------
 * Parses a linear programming model from the given file path.
 *
 * The input file must follow this structure:
 * - First non-comment, non-empty line must specify "Max" or "Min".
 * - Next line is the objective function (e.g., 3x + 4y - z).
 * - Followed by constraints, and optionally bounded variables, integers, or binaries.
 *
 * Returns:
 *   An LPModel object populated with the parsed problem.
 *
 * Throws:
 *   runtime_error on invalid format, duplicate sections, or parsing errors.
 */
LPModel Parser::parseFile(const string& path) {
  ifstream file(path);
  if (!file.is_open()) throw runtime_error("Could not open input file: " + path);

  LPModel model;
  string line;
  int lineNo = 0;

  enum Section { NONE, CONSTRAINTS, BOUNDS, INTEGERS, BINARIES };
  Section current = NONE;
  bool objectiveParsed = false;

  while (getline(file, line)) {
    lineNo++;
    line = trim(line);

    // Skip empty lines and comments
    if (isBlank(line) || line.rfind("//", 0) == 0) continue;

    // Parse optimization type (Min or Max)
    if (line == "Max" || line == "Min") {
      if (objectiveParsed) {
        throw runtime_error("Line " + to_string(lineNo) + ": Duplicate optimization type.");
      }
      model.type = (line == "Max") ? OptType::MAXIMIZE : OptType::MINIMIZE;
      objectiveParsed = true;
      continue;
    }

    // Parse objective function
    if (!objectiveParsed) {
      model.objective = { parseExpression(line, lineNo), 0.0, "", lineNo };
      objectiveParsed = true;
      current = CONSTRAINTS;
      continue;
    }

    // Handle section headers
    if (line == "Bounds:") { current = BOUNDS;   continue; }
    if (line == "Integer:") { current = INTEGERS; continue; }
    if (line == "Binary:") { current = BINARIES; continue; }

    // Parse constraints
    if (current == CONSTRAINTS) {
      model.constraints.push_back(parseConstraint(line, lineNo));

      // Parse bounds section
    }
    else if (current == BOUNDS) {
      smatch match;
      regex boundPattern(R"((\w+)\s*(>=|<=|=)\s*(-?\d+\.?\d*))");

      if (line.find("free") != string::npos) {
        string var = trim(line.substr(0, line.find("free")));
        model.bounds[var].isFree = true;
      }
      else if (regex_match(line, match, boundPattern)) {
        string var = match[1];
        string op = match[2];
        double val = stod(match[3]);

        auto& b = model.bounds[var];
        if (op == ">=") b.lower = val;
        else if (op == "<=") b.upper = val;
        else if (op == "=")  b.lower = b.upper = val;
      }
      else {
        throw runtime_error("Line " + to_string(lineNo) + ": Invalid bound format.");
      }

      // Parse integer variable declarations
    }
    else if (current == INTEGERS || current == BINARIES) {
      vector<string> vars = split(line, ',');
      for (const string& var : vars) {
        auto& b = model.bounds[var];
        b.type = (current == INTEGERS) ? VarType::INTEGER : VarType::BINARY;

        if (current == BINARIES) {
          b.lower = 0;
          b.upper = 1;
        }
      }

      // Catch unexpected or misformatted input
    }
    else {
      throw runtime_error("Line " + to_string(lineNo) + ": Unexpected line or misplaced section.");
    }
  }

  return model;
}
