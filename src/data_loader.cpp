#include "data_loader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <set>
#include <cmath>

using namespace std;

// === Distance euclidienne en EPSG:2154 ===
double distance_euclidienne(double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

// === Chargement des habitants ===
vector<Habitant> chargerHabitants(const string &chemin_csv)
{
    vector<Habitant> habitants;
    ifstream fichier(chemin_csv);
    if (!fichier.is_open())
    {
        cerr << "Erreur ouverture " << chemin_csv << endl;
        return habitants;
    }

    string ligne;
    getline(fichier, ligne); // skip header

    while (getline(fichier, ligne))
    {
        stringstream ss(ligne);
        string champ;
        Habitant h;

        getline(ss, champ, ',');
        h.id = stoi(champ);
        getline(ss, champ, ',');
        h.x = stod(champ);
        getline(ss, champ, ',');
        h.y = stod(champ);
        getline(ss, champ, ',');
        h.nb_habitants = stoi(champ);
        getline(ss, champ, ',');
        h.classe = stoi(champ);
        getline(ss, champ, ',');
        h.iris_code = champ;

        habitants.push_back(h);
    }

    cout << "✅ Habitants chargés : " << habitants.size() << endl;
    return habitants;
}

// === Chargement des emplacements ===
vector<Emplacement> chargerEmplacements(const string &chemin_csv)
{
    vector<Emplacement> emplacements;
    ifstream fichier(chemin_csv);
    if (!fichier.is_open())
    {
        cerr << "Erreur ouverture " << chemin_csv << endl;
        return emplacements;
    }

    string ligne;
    getline(fichier, ligne); // skip header

    while (getline(fichier, ligne))
    {
        stringstream ss(ligne);
        string champ;
        Emplacement e;

        getline(ss, champ, ',');
        e.id = stoi(champ);
        getline(ss, champ, ',');
        e.x = stod(champ);
        getline(ss, champ, ',');
        e.y = stod(champ);
        getline(ss, champ, ',');
        e.surface = stod(champ);

        // J(i) = surfaces autorisées
        e.surfaces_j.push_back(e.surface);
        if (e.surface >= 500)
            e.surfaces_j.push_back(500);
        if (e.surface >= 100)
            e.surfaces_j.push_back(100);

        emplacements.push_back(e);
    }

    cout << "✅ Emplacements chargés : " << emplacements.size() << endl;
    return emplacements;
}
void afficher_resume(const Data &data)
{
    using std::cout;
    using std::endl;

    cout << "\n🧾 Résumé des données chargées :" << endl;
    cout << "→ Emplacements I : " << data.I.size() << endl;

    size_t total_j = 0;
    for (const auto &ji : data.J)
        total_j += ji.size();
    cout << "→ Types d’installations (J(i)) totaux : " << total_j << endl;

    cout << "→ Habitants vulnérables (C_5,6) : " << data.C_56.size() << endl;
    cout << "→ Habitants non vulnérables : " << data.C_autres.size() << endl;

    cout << "→ Matrice A : " << data.A.size() << " x "
         << (data.A.empty() ? 0 : data.A[0].size()) << " x "
         << (data.A.empty() || data.A[0].empty() ? 0 : data.A[0][0].size()) << endl;

    cout << "→ Matrice D : " << data.D.size() << " x "
         << (data.D.empty() ? 0 : data.D[0].size()) << " x "
         << (data.D.empty() || data.D[0].empty() ? 0 : data.D[0][0].size()) << endl;

    cout << "→ Horizon T (années) : ";
    for (int t : data.T)
        cout << t << " ";
    cout << endl;

    cout << "→ Paramètres : I_max = " << data.I_max
         << ", alpha = " << data.alpha
         << ", B ∈ [" << data.B_min << ", " << data.B_max << "]"
         << ", lambda = " << data.lambda << endl;

    cout << "→ Zones Z : " << data.Z.size() << " zone(s), " << data.Z[0].size() << " emplacements dans la première." << endl;
}

Data load_data(const string &path_habitants,
               const string &path_emplacements,
               const string &path_aijc,
               const string &path_dijc,
               const vector<int> &T,
               double lambda,
               int I_max,
               double alpha,
               double B_min,
               double B_max)
{
    Data data;

    // 1. Charger habitants
    vector<Habitant> habitants = chargerHabitants(path_habitants);
    int nb_c = habitants.size();

    // Remplir C_56, C_autres et id_to_index en même temps
    for (int c = 0; c < nb_c; ++c)
    {
        int id = habitants[c].id;
        data.id_to_index[id] = c; // Mappe l'ID réel vers son index dans A

        if (habitants[c].classe >= 5)
        {
            data.C_56.push_back(id); // Stocke l'ID réel
            double seuil = 4.0;
            data.d.push_back(seuil - static_cast<double>(habitants[c].classe));
        }
        else
        {
            data.C_autres.push_back(id); // Stocke l'ID réel
        }
    }

    // 2. Charger emplacements
    vector<Emplacement> emplacements = chargerEmplacements(path_emplacements);
    int nb_i = emplacements.size();
    data.I.resize(nb_i);
    data.J.resize(nb_i);
    data.b.resize(nb_i);

    for (int i = 0; i < nb_i; ++i)
    {
        data.I[i] = emplacements[i].id;
        for (double surface_j : emplacements[i].surfaces_j)
        {
            data.J[i].push_back(static_cast<int>(surface_j));
            data.b[i].push_back(surface_j * 550.0);
        }
    }

    // 3. Initialiser les matrices A et D
    int max_j = 3;
    data.A = vector<vector<vector<int>>>(nb_c, vector<vector<int>>(nb_i, vector<int>(max_j, 0)));
    data.D = vector<vector<vector<double>>>(nb_c, vector<vector<double>>(nb_i, vector<double>(max_j, 0.0)));

    // 4. Charger A depuis a_ijc.csv
    ifstream fa(path_aijc);
    if (!fa.is_open())
    {
        cerr << "Erreur ouverture " << path_aijc << endl;
        exit(1);
    }
    string ligne;
    getline(fa, ligne);
    while (getline(fa, ligne))
    {
        stringstream ss(ligne);
        string champ;
        int i, j, c, a;

        getline(ss, champ, ',');
        i = stoi(champ);
        getline(ss, champ, ',');
        j = stoi(champ);
        getline(ss, champ, ',');
        c = stoi(champ);
        getline(ss, champ, ',');
        a = stoi(champ);

        // Vérifie si l'identifiant c existe dans id_to_index
        if (data.id_to_index.find(c) != data.id_to_index.end() && i < nb_i && j < max_j)
        {
            int local_c = data.id_to_index.at(c);
            data.A[local_c][i][j] = a;
        }
    }

    cout << "✅ Matrice A chargée : dimensions = "
         << data.A.size() << " x "
         << (data.A.empty() ? 0 : data.A[0].size()) << " x "
         << (data.A.empty() || data.A[0].empty() ? 0 : data.A[0][0].size()) << endl;

    // 5. Charger D depuis d_ijc.csv
    ifstream fd(path_dijc);
    if (!fd.is_open())
    {
        cerr << "Erreur ouverture " << path_dijc << endl;
        exit(1);
    }
    getline(fd, ligne);
    while (getline(fd, ligne))
    {
        stringstream ss(ligne);
        string champ;
        int i, j, c;
        double d_val;

        getline(ss, champ, ',');
        i = stoi(champ);
        getline(ss, champ, ',');
        j = stoi(champ);
        getline(ss, champ, ',');
        c = stoi(champ);
        getline(ss, champ, ',');
        d_val = stod(champ);

        // Vérifie si l'identifiant c existe dans id_to_index
        if (data.id_to_index.find(c) != data.id_to_index.end() && i < nb_i && j < max_j)
        {
            int local_c = data.id_to_index.at(c);
            data.D[local_c][i][j] = d_val;
        }
    }

    // 6. Charger T et les paramètres
    data.T = T;
    data.I_max = I_max;
    data.alpha = alpha;
    data.B_min = B_min;
    data.B_max = B_max;
    data.lambda = lambda;

    // 7. Z = partition par zone (une seule zone par défaut)
    data.Z.push_back({});
    for (int i = 0; i < nb_i; ++i)
        data.Z[0].push_back(i);

    return data;
}
