#include "solver.h"
#include <stdexcept>
#include <iostream>

GLPKSolver::GLPKSolver() {
    lp = glp_create_prob();
}

GLPKSolver::~GLPKSolver() {
    glp_delete_prob(lp);
}

void GLPKSolver::loadModel(const LPModel& model) {
    glp_set_prob_name(lp, "MILP_Model");
    glp_set_obj_dir(lp, model.type == OptType::MAXIMIZE ? GLP_MAX : GLP_MIN);

    // 1. Add variables (columns)
    int numVars = model.bounds.size();
    glp_add_cols(lp, numVars);

    int colIdx = 1;
    for (const auto& [varName, bound] : model.bounds) {
        varNameToCol[varName] = colIdx;
        glp_set_col_name(lp, colIdx, varName.c_str());

        // Set bounds
        if (bound.isFree) {
            glp_set_col_bnds(lp, colIdx, GLP_FR, 0.0, 0.0);
        } else if (bound.lower != -INFINITY && bound.upper != INFINITY) {
            glp_set_col_bnds(lp, colIdx, GLP_DB, bound.lower, bound.upper);
        } else if (bound.lower != -INFINITY) {
            glp_set_col_bnds(lp, colIdx, GLP_LO, bound.lower, 0.0);
        } else if (bound.upper != INFINITY) {
            glp_set_col_bnds(lp, colIdx, GLP_UP, 0.0, bound.upper);
        } else {
            glp_set_col_bnds(lp, colIdx, GLP_FR, 0.0, 0.0);
        }

        // Set variable type
        switch (bound.type) {
            case VarType::CONTINUOUS:
                glp_set_col_kind(lp, colIdx, GLP_CV);
                break;
            case VarType::INTEGER:
                glp_set_col_kind(lp, colIdx, GLP_IV);
                break;
            case VarType::BINARY:
                glp_set_col_kind(lp, colIdx, GLP_BV);
                break;
        }
        ++colIdx;
    }

    // 2. Set objective function
    for (const auto& term : model.objective.terms) {
        int idx = varNameToCol.at(term.variable);
        glp_set_obj_coef(lp, idx, term.coefficient);
    }

    // 3. Add constraints (rows)
    int numCons = model.constraints.size();
    glp_add_rows(lp, numCons);

    for (int i = 0; i < numCons; ++i) {
        const auto& con = model.constraints[i];
        glp_set_row_name(lp, i + 1, ("c" + std::to_string(i + 1)).c_str());

        // Set constraint bounds
        if (con.op == "<=") {
            glp_set_row_bnds(lp, i + 1, GLP_UP, 0.0, con.rhs);
        } else if (con.op == ">=") {
            glp_set_row_bnds(lp, i + 1, GLP_LO, con.rhs, 0.0);
        } else if (con.op == "=") {
            glp_set_row_bnds(lp, i + 1, GLP_FX, con.rhs, con.rhs);
        } else {
            throw std::runtime_error("Unknown constraint operator: " + con.op);
        }
    }

    // 4. Set constraint matrix
    // GLPK expects 1-based arrays for ia, ja, ar
    std::vector<int> ia(1), ja(1);
    std::vector<double> ar(1);
    for (int i = 0; i < numCons; ++i) {
        const auto& con = model.constraints[i];
        for (const auto& term : con.terms) {
            ia.push_back(i + 1);
            ja.push_back(varNameToCol.at(term.variable));
            ar.push_back(term.coefficient);
        }
    }
    glp_load_matrix(lp, ia.size() - 1, ia.data(), ja.data(), ar.data());
}

void GLPKSolver::solve(bool useDualSimplex, bool isMIP) {
    if (isMIP) {
        glp_intopt(lp, nullptr);
    } else {
        glp_smcp parm;
        glp_init_smcp(&parm);
        if (useDualSimplex) parm.meth = GLP_DUAL;
        glp_simplex(lp, &parm);
    }
}

double GLPKSolver::getObjectiveValue() const {
    return glp_mip_obj_val(lp); // For MIP
    // For LP: return glp_get_obj_val(lp);
}

std::unordered_map<std::string, double> GLPKSolver::getVariableValues() const {
    std::unordered_map<std::string, double> result;
    for (const auto& [name, idx] : varNameToCol) {
        result[name] = glp_mip_col_val(lp, idx); // For MIP
        // For LP: glp_get_col_prim(lp, idx);
    }
    return result;
}