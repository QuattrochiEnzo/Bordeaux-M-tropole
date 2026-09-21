import pandas as pd
import geopandas as gpd
import os
import glob
from tqdm import tqdm

# --- 1. Chargement des données ---
print("📥 Chargement des données...")
df_habitants = pd.read_csv("../data/Exports/Habitants/habitants.csv")
df_emplacements = pd.read_csv("../data/Exports/Emplacements/emplacements_final.csv")
df_j = pd.read_csv("../data/Exports/J(i).csv")
df_a = pd.read_csv("../data/Exports/a_ijc.csv")



# --- 2. Scores des habitants ---
print("🧮 Calcul des scores habitants (classes 5–6)...")
df_habitants["score"] = df_habitants.apply(lambda row: row["nb_habitants"] if row["classe"] >= 1 else 0, axis=1)
df_habitants["couvert"] = 0

# --- 2bis. Filtrage des habitants déjà couverts ---
print("🧽 Filtrage des habitants déjà couverts (zones existantes)...")
geoms = []
for file in glob.glob("../data/Exports/EXPORT_FRAICHEUR_GEOJSON/*.geojson"):
    try:
        gdf = gpd.read_file(file).to_crs("EPSG:2154")
        geoms.append(gdf)
    except Exception as e:
        print(f"⚠️ Erreur avec {file} : {e}")

gdf_union = gpd.GeoDataFrame(pd.concat(geoms, ignore_index=True), crs="EPSG:2154")
gdf_union["geometry"] = gdf_union.buffer(300)

gdf_habitants = gpd.GeoDataFrame(df_habitants, geometry=gpd.points_from_xy(df_habitants["lon"], df_habitants["lat"]), crs="EPSG:4326").to_crs("EPSG:2154")
gdf_habitants["id_habitant"] = df_habitants["id"]
covered = gpd.sjoin(gdf_habitants, gdf_union, how="left", predicate="within")
habitants_couverts = covered[~covered["index_right"].isna()]["id_habitant"].unique()

df_habitants = df_habitants[~df_habitants["id"].isin(habitants_couverts)].copy()
print(f"❌ {len(habitants_couverts):,} habitants supprimés car déjà couverts")

# --- 3. Fusion emplacements + J(i) ---
df_j = df_j.merge(df_emplacements[["id", "surface_m2", "categorie"]], left_on="id_i", right_on="id", how="left")
df_j["prix"] = df_j["surface_m2"] * 550

# --- 4. Score par (i,j) ---
print("🔗 Calcul du score de chaque (i,j)...")
df_impact = df_a.merge(df_habitants[["id", "score"]], left_on="id_c", right_on="id", how="left")
df_score_ij = df_impact.groupby(["id_i", "id_j"])["score"].sum().reset_index()
df_j = df_j.merge(df_score_ij, on=["id_i", "id_j"], how="left").fillna(0)
df_j["construit"] = 0

# --- 5. Tri des habitants ---
print("📋 Tri des habitants (classe + vulnérabilité)...")
df_habitants["priorite"] = df_habitants["score"] + df_habitants["nb_habitants"]
df_habitants = df_habitants.sort_values(by=["classe", "priorite"], ascending=[False, False]).reset_index(drop=True)

# --- 6. Tri des emplacements ---
df_j = df_j.sort_values(by=["score", "prix"], ascending=[False, True])

# --- 7. Glouton ---
print("⚙️ Lancement de l’algorithme glouton...")
nb_non_couverts = 0

for i in tqdm(range(len(df_habitants)), desc="🚶 Parcours des habitants"):
    row = df_habitants.iloc[i]
    if row["couvert"] == 0 and row["classe"] >= 5:
        id_c = row["id"]
        possibles = df_a[df_a["id_c"] == id_c][["id_i", "id_j"]]
        if len(possibles) > 0:
            candidats = pd.merge(df_j, possibles, on=["id_i", "id_j"], how="inner")
            if len(candidats) > 0:
                meilleur = candidats.iloc[0]
                id_i, id_j = meilleur["id_i"], meilleur["id_j"]
                df_j.loc[(df_j["id_i"] == id_i) & (df_j["id_j"] == id_j), "construit"] = 1
                df_j = df_j[~((df_j["id_i"] == id_i) & (df_j["construit"] == 0))]
                couverts = df_a[(df_a["id_i"] == id_i) & (df_a["id_j"] == id_j)]["id_c"].values
                df_habitants.loc[df_habitants["id"].isin(couverts), "couvert"] = 1
        else:
            nb_non_couverts += row["nb_habitants"]

print("\n✅ Résolution terminée.")
print(f"👥 Nombre d’habitants restés non couverts : {nb_non_couverts}")

# --- 8. Export CSV ---
output_path = "../data/Exports/Resultats/resultats_glouton_all.csv"
df_j[df_j["construit"] == 1].to_csv(output_path, index=False)
print(f"📤 Résultat exporté : {output_path}")

# Recharger depuis le fichier exporté pour les statistiques
df_exported = pd.read_csv(output_path)

