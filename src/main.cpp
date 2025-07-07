#include "parser.h"
#include "solver.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <cstring>

#include <inttypes.h>

/**
 * @brief Prints the usage instructions for the CLI tool.
 */
void printUsage() {
  std::cout << "Usage: MILP_Solver -f <input_file> -o <output_file> [--dual] [--log]\n"
    << "Options:\n"
    << "  -f <input_file>   Path to the input MILP file.\n"
    << "  -o <output_file>  Path to the output log file.\n"
    << "  --dual            Use the dual simplex method (default is primal).\n"
    << "  --log             Enable logging of intermediate simplex states.\n";
}

int main(int argc, char* argv[]) {
  // Check for minimum required arguments
  if (argc < 5) {
    printUsage();
    return 1;
  }

  std::string inputFile;
  std::string outputFile;
  bool useDualSimplex = false;
  bool enableLogging = false;

  // Parse command-line arguments
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
      inputFile = argv[++i];
    }
    else if (std::strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
      outputFile = argv[++i];
    }
    else if (std::strcmp(argv[i], "--dual") == 0) {
      useDualSimplex = true;
    }
    else if (std::strcmp(argv[i], "--log") == 0) {
      enableLogging = true;
    }
    else {
      std::cerr << "Unknown argument: " << argv[i] << "\n";
      printUsage();
      return 1;
    }
  }

  // Validate required arguments
  if (inputFile.empty() || outputFile.empty()) {
    std::cerr << "Error: Input and output file paths are required.\n";
    printUsage();
    return 1;
  }

  try {
    // Parse the input file
    LPModel model = Parser::parseFile(inputFile);

    // Initialize the solver
    GLPKSolver solver;
    solver.loadModel(model);

    // Solve the problem
    solver.solve(useDualSimplex, /* isMIP */ true);

    // Open the output file for logging
    std::ofstream logFile(outputFile);
    if (!logFile.is_open()) {
      throw std::runtime_error("Could not open output file: " + outputFile);
    }

    // Log the results
    logFile << "Objective Value: " << solver.getObjectiveValue() << "\n";
    logFile << "Variable Values:\n";
    for (const auto& [varName, value] : solver.getVariableValues()) {
      logFile << "  " << varName << " = " << value << "\n";
    }

    // Log intermediate simplex states if enabled
    if (enableLogging) {
      logFile << "\nIntermediate Simplex States:\n";
      // Add logic to log intermediate states here if implemented
    }

    logFile.close();
    std::cout << "Solution logged to: " << outputFile << "\n";

  }
  catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
  }

  return 0;
}