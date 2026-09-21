#ifndef DATA_LOADER_HPP
#define DATA_LOADER_HPP

#include <string>
#include <vector>
#include "resol.hpp" // Habitant, Emplacement, Couverture
#include "data.hpp"  // Structure Data

// === Chargement des données de base ===

// ✅ habitants.csv → vecteur d’Habitant
std::vector<Habitant> chargerHabitants(const std::string &chemin_csv);

// ✅ emplacements_final.csv → vecteur d’Emplacement
std::vector<Emplacement> chargerEmplacements(const std::string &chemin_csv);

// === Chargement complet du modèle ===

// 🔁 Lis les CSV (habitants, emplacements, a_ijc, d_ijc) et construit la structure Data
Data load_data(
    const std::string &path_habitants,
    const std::string &path_emplacements,
    const std::string &path_aijc,
    const std::string &path_dijc,
    const std::vector<int> &T,
    double lambda,
    int I_max,
    double alpha,
    double B_min,
    double B_max);

// === Utilitaires ===

// 📏 Distance euclidienne entre deux points (EPSG:2154)
double distance_euclidienne(double x1, double y1, double x2, double y2);
void afficher_resume(const Data &data);

#endif
