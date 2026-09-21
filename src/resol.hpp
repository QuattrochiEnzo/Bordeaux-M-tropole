#ifndef RESOL_HPP
#define RESOL_HPP

#include <string>
#include <vector>

// ================== STRUCTURES PARTAGÉES ==================

struct Habitant
{
    int id;
    double x, y; // EPSG:2154 (coord. projetées en mètres)
    int nb_habitants;
    int classe;
    std::string iris_code;
};

struct Emplacement
{
    int id;
    double x, y; // EPSG:2154
    double surface;
    std::vector<double> surfaces_j; // tailles autorisées
};

struct Couverture
{
    int i;        // id emplacement
    int j;        // index dans J(i)
    int c;        // id habitant
    double d_ijc; // impact thermique
};

#endif // RESOL_HPP
