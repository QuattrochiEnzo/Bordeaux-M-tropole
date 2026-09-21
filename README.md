

### 📊 État des éléments de modélisation

| Élément       | Description | Statut |
|---------------|-------------|--------|
| `I`           | Emplacements potentiels avec coordonnées GPS | ✅ extraits depuis l’indice de plantabilité catégorisé (valeurs 40 et 50), regroupés par zones contiguës, filtrés spatialement (zones boisées + fraîcheur), fusionnés par contact, et classés par surface |
| `C`           | Habitants avec coordonnées GPS | ✅ générés à partir des polygones du fichier `UF_AVEC_POP`, chaque polygone étant localisé par son centroïde |
| `Classe_c`    | Classe thermique (1 à 6) des habitants | ✅ assignée via jointure spatiale entre les UF et le fichier `ri_vulnerabilite_s.geojson` utilisant `i_vulnerabilite_ictu` |
| `C_5,6`       | Sous-ensemble des habitants en classe 5 ou 6 | ✅ dérivable automatiquement depuis `Classe_c` |
| `Z`           | Partition des `I` en zones/quartiers | ✅ disponible via le champ `iris_code` des habitants, ou générable par KMeans |
| `a_ijc`       | Binaire : habitant c couvert par une installation (i, j) | ✅ généré via KDTree sur un rayon de 300 m entre chaque `(i,j)` et les habitants `c`, exporté dans `a_ijc.csv` |
| `b_ij`        | Coût d'une installation j sur i = surface × 550 € | ✅ règle définie par le cahier des charges |
| `d_ijc`       | Impact d’une installation sur un habitant c | ✅ modélisé comme `d_ijc = max(0, 1 - d(i,c)/300)`, exporté dans `d_ijc.csv` |
| `d_c`         | Amélioration thermique cible pour chaque c ∈ C_5,6 | 🧩 à fixer selon un seuil thermique cible (ex : seuil = 4 → `d_c = classe_actuelle - 4`) |
| `J(i)`        | Ensemble des types/surfaces autorisés à i | ✅ généré par typologie, avec variantes (100, 500, surface max) selon la taille, exporté dans `J(i).csv` |
| `Imax`        | Nb max d’installations par an | ✅ valeur fixée à 50 installations/an |
| `α`           | Taux max d’installations par zone Z par an | ✅ à définir (ex. α = 0.2 → 10 installations max par zone si `Imax` = 50) |
| `Bmin, Bmax`  | Budgets min et max par zone | ✅ à fixer selon les priorités politiques et territoriales |
| `λ`           | Ratio d’équilibrage coût/couverture | ✅ paramètre utilisateur pour pondérer les deux objectifs dans la fonction objectif |

---

### 📁 Données sources utilisées

- `UF_AVEC_POP(PAR_NB_LOG).shp` : polygones d’habitation avec population par logement
- `ri_vulnerabilite_s.geojson` : classes thermiques par maille de 200 m (`i_vulnerabilite_ictu`)
- `EXPORT_FRAICHEUR_EN_VILLE/` : espaces de fraîcheur déjà existants → à exclure des futurs candidats `I`
- `Bordeaux_Metropole_calque_plantabilite_valeurs_palette.tiff` : raster catégorisé servant à identifier les `I`
- `to_bois_s.csv` : géométries d'espaces boisés à exclure

---

### 🧠 Ce qu’il reste à faire ou affiner côté données

1. **Fixer les `d_c`** :
   - Règle proposée : `d_c = classe_actuelle - seuil` pour les `c ∈ C_5,6`
   - Seuil à définir selon ambition politique (ex : 3.5 ou 4.0)

2. **Finaliser ou affiner `Z`** :
   - La partition IRIS est fonctionnelle mais peut être affinée (KMeans, regroupements adjacents, etc.)

3. **Exploration visuelle et cartographique** :
   - Intégrée via l’application Streamlit
   - Possibilité d'ajouter des indicateurs de couverture par zone ou classe thermique

4. **Mise en forme des données pour le solveur** :
   - Construction des variables de décision à partir de `J(i)`
   - Table de coûts (`b_ij`)
   - Fichiers `.lp`, `.dat`, `.mod` à générer

---

### ⚙️ Commandes de build

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

---

### 📌 **Point d'étape – 17 avril 2025**

#### ✅ Habitants (`C`) traités
- 📍 Extraction à partir de `UF_AVEC_POP(PAR_NB_LOG).shp`, centroïdes utilisés pour positionner chaque polygone
- 📊 Classe thermique associée via `ri_vulnerabilite_s.geojson` (`i_vulnerabilite_ictu`)
- 🧮 Population estimée via `nb_log`
- 📤 Fichier généré : `habitants.csv` dans `../Exports/Habitants/`

#### ✅ Emplacements (`I`) traités
- 🌱 Pixels favorables (valeurs 40 et 50) extraits du raster catégorisé
- 🔍 Regroupement spatial par connectivité (zones contiguës)
- ❌ Filtrage spatial via exclusion des boisés (`to_bois_s.csv`) et des espaces de fraîcheur (`EXPORT_FRAICHEUR_EN_VILLE`)
- 🔗 Fusion des polygones contigus post-filtrage
- 🧮 Surface recalculée et centroïde géolocalisé
- 🏷️ Classification en `petit`, `moyen`, `grand` selon la surface
- 📤 Fichiers générés :
  - `emplacements_filtres.geojson` : version filtrée polygonale
  - `emplacements_clean.csv` : export tabulaire propre (id, lon, lat, surface, catégorie)

#### ✅ Choix d’installations (`J(i)`) traités
- 🧮 Générés selon la catégorie : 1 à 3 variantes par polygone
- 💾 Fichier généré : `J(i).csv`

#### ✅ Matrices $a_{ijc}$ et $d_{ijc}$
- 📍 Calcul via KDTree
- 💾 Fichiers générés : `a_ijc.csv`, `d_ijc.csv` (plus de 9 millions de lignes)

#### ✅ Interface interactive
- 🎛️ Application Streamlit fonctionnelle
- 🔍 Exploration possible :
  - par habitant
  - avec carte dynamique $(i,j) \rightarrow c$
  - tableau complet des installations couvrantes


  ---

### ⚙️ Commandes d'éxé

```bash
streamlit run app.py
```
