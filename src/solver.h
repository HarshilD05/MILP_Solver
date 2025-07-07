#pragma once

#include "parser.h"
#include <glpk.h>
#include <string>
#include <unordered_map>

/**
 * @class GLPKSolver
 * @brief A class to map and solve MILP/LP problems using the GLPK library.
 *
 * This class provides functionality to:
 * - Map a parsed LPModel (from the parser) to a GLPK problem instance.
 * - Solve the problem using GLPK's simplex or integer optimization methods.
 * - Retrieve the solution, including objective value and variable assignments.
 */
class GLPKSolver {
  glp_prob* lp; // GLPK problem object
  std::unordered_map<std::string, int> varNameToCol; // Map variable name to GLPK column index

public:
  /**
   * @brief Constructor: Initializes the GLPK problem object.
   */
  GLPKSolver();

  /**
   * @brief Destructor: Frees the GLPK problem object.
   */
  ~GLPKSolver();

  /**
   * @brief Loads the parsed LPModel into the GLPK problem object.
   * 
   * @param model The LPModel object containing the parsed MILP/LP problem.
   * 
   * This function maps variables, constraints, bounds, and the objective
   * function from the LPModel structure into the GLPK problem instance.
   */
  void loadModel(const LPModel& model);

  /**
   * @brief Solves the loaded problem using GLPK.
   * 
   * @param useDualSimplex If true, uses the dual simplex method; otherwise, uses primal simplex.
   * @param isMIP If true, solves the problem as a MILP using branch-and-bound.
   * 
   * This function solves the problem using either simplex (for LP) or
   * branch-and-bound (for MILP), depending on the flags provided.
   */
  void solve(bool useDualSimplex = false, bool isMIP = false);

  /**
   * @brief Retrieves the objective value of the solved problem.
   * 
   * @return The objective value of the solution.
   * 
   * For MILP, this retrieves the integer solution's objective value.
   * For LP, it retrieves the optimal objective value.
   */
  double getObjectiveValue() const;

  /**
   * @brief Retrieves the values of the decision variables in the solution.
   * 
   * @return A map of variable names to their corresponding solution values.
   * 
   * For MILP, this retrieves the integer solution values.
   * For LP, it retrieves the optimal continuous values.
   */
  std::unordered_map<std::string, double> getVariableValues() const;
};