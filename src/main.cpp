#include "data_loader.hpp"
#include "solveur.hpp"
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "❌ Usage : " << argv[0] << " <zone_directory>" << std::endl;
        return 1;
    }

    std::string zone_dir = argv[1];

    // Fichiers spécifiques à la zone (habitants et emplacements locaux)
    std::string path_habitants = zone_dir + "/habitants.csv";
    std::string path_emplacements = zone_dir + "/emplacements_final.csv";

    // Fichiers globaux pour toutes les zones (a_ijc et d_ijc)
    std::string path_aijc = "../data/Exports/a_ijc.csv";
    std::string path_dijc = "../data/Exports/d_ijc.csv";

    // Paramètres du modèle
    std::vector<int> T = {0}; // si inutilisé, laisse comme ça
    double lambda = 0.5;
    int I_max = 50;
    double alpha = 0.2;
    double B_min = 0;
    double B_max = 1e8;

    // Chargement des données
    Data data = load_data(path_habitants, path_emplacements, path_aijc, path_dijc,
                          T, lambda, I_max, alpha, B_min, B_max);

    // Résolution du modèle avec écriture dans le dossier de la zone
    resoudre(data, zone_dir);

    return 0;
}
