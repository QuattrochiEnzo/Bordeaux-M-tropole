#ifndef DATA_HPP
#define DATA_HPP

#include <vector>
#include <unordered_map>

struct Data
{
    std::vector<int> I;
    std::vector<std::vector<int>> J;    // J[i] = tailles autorisées
    std::vector<std::vector<double>> b; // Coût b[i][j] = surface × 550 €

    std::vector<int> T; // Années (ex: {0,1,2})

    std::vector<int> C_56;     // indices des habitants vulnérables
    std::vector<int> C_autres; // indices des autres

    std::vector<std::vector<std::vector<int>>> A;    // a_ijc binaire : A[c][i][j]
    std::vector<std::vector<std::vector<double>>> D; // d_ijc impact thermique

    std::vector<double> d; // amélioration thermique cible

    std::vector<std::vector<int>> Z; // Z[z] = indices i de la zone z

    int I_max;
    double alpha;
    double B_min;
    double B_max;
    double lambda;
    std::unordered_map<int, int> id_to_index;
    // utilitaire : retourne l’indice global i depuis z et i_local
    int Z_to_I(int z, int local_i) const
    {
        return Z[z][local_i];
    }
};

#endif
