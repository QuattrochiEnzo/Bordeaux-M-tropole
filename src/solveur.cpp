#include "solveur.hpp"
#include "gurobi_c++.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <set>

using namespace std;

void resoudre(const Data &data, const std::string &zone_dir)
{
    GRBEnv env = GRBEnv(true);
    env.start();
    GRBModel model = GRBModel(env);

    // === Variables s_ijt ===
    vector<vector<vector<GRBVar>>> s(data.I.size());
    for (size_t i = 0; i < data.I.size(); i++)
    {
        s[i].resize(data.J[i].size());
        for (size_t j = 0; j < data.J[i].size(); j++)
        {
            s[i][j].resize(data.T.size());
            for (size_t t = 0; t < data.T.size(); t++)
            {
                stringstream ss;
                ss << "s(" << i << "," << j << "," << t << ")";
                s[i][j][t] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, ss.str());
            }
        }
    }

    // === Variable T ===
    GRBVar T = model.addVar(0.0, data.T.size(), 0.0, GRB_INTEGER, "T");

    // === Variables x_c (slack) ===
    vector<GRBVar> x(data.C_autres.size());
    for (size_t c = 0; c < data.C_autres.size(); c++)
    {
        stringstream ss;
        ss << "x(" << c << ")";
        x[c] = model.addVar(0.0, 1.0, 0.0, GRB_CONTINUOUS, ss.str());
    }

    // === Objectif (1) ===
    GRBLinExpr obj = T;
    for (size_t i = 0; i < data.I.size(); i++)
    {
        for (size_t j = 0; j < data.J[i].size(); j++)
        {
            for (size_t t = 0; t < data.T.size(); t++)
            {
                obj += data.lambda * data.b[i][j] * s[i][j][t];
            }
        }
    }
    for (size_t c = 0; c < data.C_autres.size(); c++)
    {
        obj += (1.0 - data.lambda) * x[c];
    }
    model.setObjective(obj, GRB_MINIMIZE);

    // === Contraintes (2) ===
    for (size_t i = 0; i < data.I.size(); i++)
    {
        for (size_t j = 0; j < data.J[i].size(); j++)
        {
            for (size_t t = 0; t < data.T.size(); t++)
            {
                model.addConstr(T >= data.T[t] * s[i][j][t]);
            }
        }
    }

    // === Contraintes (3) ===
    for (size_t i = 0; i < data.I.size(); i++)
    {
        GRBLinExpr sum_s = 0;
        for (size_t j = 0; j < data.J[i].size(); j++)
        {
            for (size_t t = 0; t < data.T.size(); t++)
            {
                sum_s += s[i][j][t];
            }
        }
        model.addConstr(sum_s <= 1);
    }

    // === Contraintes (4) Amélioration thermique ===
    for (size_t c = 0; c < data.C_56.size(); c++)
    {
        GRBLinExpr sum_d = 0;
        for (size_t i = 0; i < data.I.size(); i++)
        {
            for (size_t j = 0; j < data.J[i].size(); j++)
            {
                for (size_t t = 0; t < data.T.size(); t++)
                {
                    sum_d += data.D[c][i][j] * s[i][j][t];
                }
            }
        }
        model.addConstr(sum_d >= data.d[c]);
    }

    // === Contraintes (5) Couverture obligatoire pour C_5,6 ===
    for (size_t c = 0; c < data.C_56.size(); c++)
    {
        GRBLinExpr sum_a = 0;
        for (size_t i = 0; i < data.I.size(); i++)
        {
            for (size_t j = 0; j < data.J[i].size(); j++)
            {
                for (size_t t = 0; t < data.T.size(); t++)
                {
                    sum_a += data.A[c][i][j] * s[i][j][t];
                }
            }
        }
        model.addConstr(sum_a >= 1);
    }

    // === Contraintes (6) Couverture souple pour C\C_5,6 ===
    for (size_t c = 0; c < data.C_autres.size(); c++)
    {
        GRBLinExpr sum_a = 0;
        for (size_t i = 0; i < data.I.size(); i++)
        {
            for (size_t j = 0; j < data.J[i].size(); j++)
            {
                for (size_t t = 0; t < data.T.size(); t++)
                {
                    sum_a += data.A[c][i][j] * s[i][j][t];
                }
            }
        }
        model.addConstr(sum_a + x[c] >= 1);
    }

    // === Contraintes (7) & (8) Budgets par zone ===
    for (size_t z = 0; z < data.Z.size(); z++)
    {
        GRBLinExpr budget = 0;
        for (int i_local : data.Z[z])
        {
            int i = data.Z_to_I(z, i_local);
            for (size_t j = 0; j < data.J[i].size(); j++)
            {
                for (size_t t = 0; t < data.T.size(); t++)
                {
                    budget += data.b[i][j] * s[i][j][t];
                }
            }
        }
        model.addConstr(budget >= data.B_min);
        model.addConstr(budget <= data.B_max);
    }

    // === Contraintes (9) Nombre max de constructions par année ===
    for (size_t t = 0; t < data.T.size(); t++)
    {
        GRBLinExpr total = 0;
        for (size_t i = 0; i < data.I.size(); i++)
        {
            for (size_t j = 0; j < data.J[i].size(); j++)
            {
                total += s[i][j][t];
            }
        }
        model.addConstr(total <= data.I_max);
    }

    // === Contraintes (10) Répartition par zone ===
    for (size_t z = 0; z < data.Z.size(); z++)
    {
        for (size_t t = 0; t < data.T.size(); t++)
        {
            GRBLinExpr zone_sum = 0;
            for (int i : data.Z[z])
            {
                for (size_t j = 0; j < data.J[i].size(); j++)
                {
                    zone_sum += s[i][j][t];
                }
            }
            model.addConstr(zone_sum <= data.alpha * data.I_max);
        }
    }

    // === Résolution ===
    model.set(GRB_DoubleParam_TimeLimit, 30.0);
    model.set(GRB_IntParam_Threads, 1);
    model.optimize();

    int status = model.get(GRB_IntAttr_Status);
    if (status == GRB_OPTIMAL || (status == GRB_TIME_LIMIT && model.get(GRB_IntAttr_SolCount) > 0))
    {
        cout << "✅ Solution trouvée (Status: " << status << ")" << endl;
        cout << "Valeur objective : " << model.get(GRB_DoubleAttr_ObjVal) << endl;
        cout << "Nombre d'années utilisées : " << T.get(GRB_DoubleAttr_X) << endl;

        for (size_t i = 0; i < data.I.size(); i++)
        {
            for (size_t j = 0; j < data.J[i].size(); j++)
            {
                for (size_t t = 0; t < data.T.size(); t++)
                {
                    if (s[i][j][t].get(GRB_DoubleAttr_X) >= 0.5)
                    {
                        cout << "📍 Construction à i=" << i << ", type j=" << j << ", année t=" << t << endl;
                    }
                }
            }
        }
    }
    else
    {
        cerr << "❌ Aucune solution trouvée. Status Gurobi : " << status << endl;
    }
}
