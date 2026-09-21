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

    // === Variables s_ij ===
    vector<vector<GRBVar>> s(data.I.size());
    for (size_t i = 0; i < data.I.size(); i++)
    {
        s[i].resize(data.J[i].size());
        for (size_t j = 0; j < data.J[i].size(); j++)
        {
            stringstream ss;
            ss << "s(" << i << "," << j << ")";
            s[i][j] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, ss.str());
        }
    }

    // === Variables slack x_c pour C_autres ===
    vector<GRBVar> x(data.C_autres.size());
    for (size_t c = 0; c < data.C_autres.size(); c++)
    {
        stringstream ss;
        ss << "x(" << c << ")";
        x[c] = model.addVar(0.0, 1.0, 0.0, GRB_CONTINUOUS, ss.str());
    }

    // === Objectif : lambda * coût total + (1 - lambda) * slack ===
    GRBLinExpr obj = 0;
    for (size_t i = 0; i < data.I.size(); i++)
    {
        for (size_t j = 0; j < data.J[i].size(); j++)
        {
            obj += data.lambda * data.b[i][j] * s[i][j];
        }
    }
    for (size_t c = 0; c < data.C_autres.size(); c++)
    {
        obj += (1.0 - data.lambda) * x[c];
    }
    model.setObjective(obj, GRB_MINIMIZE);

    // === Contrainte (3) : une seule installation max par i ===
    for (size_t i = 0; i < data.I.size(); i++)
    {
        GRBLinExpr sum_s = 0;
        for (size_t j = 0; j < data.J[i].size(); j++)
        {
            sum_s += s[i][j];
        }
        model.addConstr(sum_s <= 1);
    }

    // === Debug : Vérification couverture des vulnérables ===
    cout << "📝 Indices C_56 utilisés : ";
    for (int c : data.C_56)
        cout << c << " ";
    cout << endl;

    for (int c : data.C_56)
    {
        bool has_cover = false;
        for (size_t i = 0; i < data.I.size(); i++)
        {
            for (size_t j = 0; j < data.J[i].size(); j++)
            {
                if (data.A[c][i][j] > 0)
                    has_cover = true;
            }
        }
        cout << "Habitant vulnérable " << c << (has_cover ? " a une couverture." : " n'a AUCUNE couverture.") << endl;
    }

    // === Contrainte (5) : chaque habitant vulnérable doit être couvert
    for (size_t c_idx = 0; c_idx < data.C_56.size(); c_idx++)
    {
        int global_id = data.C_56[c_idx];
        GRBLinExpr covered = 0;
        for (size_t i = 0; i < data.I.size(); i++)
        {
            for (size_t j = 0; j < data.J[i].size(); j++)
            {
                covered += data.A[global_id][i][j] * s[i][j];
            }
        }
        model.addConstr(covered >= 1);
    }

    // === Contrainte (6) : couverture souple pour les autres (avec slack)
    for (size_t c_idx = 0; c_idx < data.C_autres.size(); c_idx++)
    {
        int global_id = data.C_autres[c_idx];
        GRBLinExpr covered = 0;
        for (size_t i = 0; i < data.I.size(); i++)
        {
            for (size_t j = 0; j < data.J[i].size(); j++)
            {
                covered += data.A[global_id][i][j] * s[i][j];
            }
        }
        model.addConstr(covered + x[c_idx] >= 1);
    }

    model.set(GRB_DoubleParam_TimeLimit, 30.0);
    model.set(GRB_IntParam_Threads, 1);
    model.optimize();

    int status = model.get(GRB_IntAttr_Status);
    if (status == GRB_OPTIMAL || (status == GRB_TIME_LIMIT && model.get(GRB_IntAttr_SolCount) > 0))
    {
        cout << "✅ Solution trouvée. Valeur : " << model.get(GRB_DoubleAttr_ObjVal) << endl;

        ofstream file(zone_dir + "/solution.csv");
        if (file.is_open())
        {
            file << "i,j,x,y,surface\n";
            for (size_t i = 0; i < data.I.size(); i++)
            {
                for (size_t j = 0; j < data.J[i].size(); j++)
                {
                    if (s[i][j].get(GRB_DoubleAttr_X) >= 0.5)
                    {
                        double x_coord = 0.0;
                        double y_coord = 0.0;
                        double surface = data.J[i][j];
                        file << data.I[i] << "," << j << "," << x_coord << "," << y_coord << "," << surface << "\n";
                    }
                }
            }
            file.close();
            cout << "✅ Solution écrite dans : " << zone_dir + "/solution.csv" << endl;
        }
        else
        {
            cerr << "❌ Impossible d’écrire dans " << zone_dir + "/solution.csv" << endl;
        }
    }
    else
    {
        cerr << "❌ Pas de solution. Status Gurobi : " << status << endl;
    }
}
