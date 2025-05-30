## FUNCTIONS TO COMPUTE GENERIC CLUSTERING AND TO GET CELLS_DF (which must indexes x:y (str)) CLUSTERS;

import numpy as np
import pandas as pd
from sklearn.cluster import KMeans
from sklearn.metrics import silhouette_score

class clustering_param:
    
    def __init__(self, features: list[str], 
                 n_centroids: int):
        self.features = features
        self.n_centroids = n_centroids

class clustering_results:
    
    def __init__(self, silhouette_scr: float, 
                 clusters_summary: pd.DataFrame, 
                 cluster_labels: np.ndarray):
        self.silhouette_scr = silhouette_scr
        self.clusters_summary = clusters_summary
        self.cluster_labels = cluster_labels

    def __str__(self):

        clusters, counts = np.unique(self.cluster_labels, return_counts=True)
        clusters_count = {cluster: count for cluster, count in zip(clusters, counts)}

        clusters_count_str = "[+] sizes:\n"
        for cluster in sorted(clusters_count, key=clusters_count.get):
            clusters_count_str += f"\t\t{cluster}: {clusters_count[cluster]}\n"
        return (f"[+] silhouette score:\n\t\t{self.silhouette_scr}\n" 
                + f"[+] clusters sizes:\n{clusters_count_str}"
                + f"[+] summary:\n\t\t{self.clusters_summary}")

# Runs generic clustering
def run_kmeans_clustering(
    df: pd.DataFrame, clustering_p: clustering_param) -> clustering_results:

    kmeans = KMeans(n_clusters=clustering_p.n_centroids)

    cluster_labels = kmeans.fit_predict(X=df[clustering_p.features])
    df["cluster"] = cluster_labels

    clusters_summary = df[clustering_p.features + ["cluster"]].groupby("cluster").agg(["mean", "std", "min", "max"])

    df.drop(["cluster"], axis=1, inplace=True)

    return clustering_results(
        silhouette_scr=silhouette_score(df[clustering_p.features], cluster_labels),
        clusters_summary=clusters_summary,
        cluster_labels=cluster_labels,
    )

import matplotlib.pyplot as plt
from matplotlib import colormaps

def run_kmeans_cells_clustering_and_draw(
    title: str, 
    figsize: tuple, x_label: str, y_label: str, fontsize: str, cmap_name: str,
    df: pd.DataFrame, clustering_p: clustering_param, save_file_name: str = None
):

    kmeans = KMeans(n_clusters=clustering_p.n_centroids)

    cluster_labels = kmeans.fit_predict(X=df[clustering_p.features])
    df["cluster"] = cluster_labels

    df["x"] = df.index.str.split(":").str[0].astype(int)
    df["y"] = df.index.str.split(":").str[1].astype(int)

    colormap = colormaps.get_cmap(cmap_name).resampled(clustering_p.n_centroids)
    cluster_colors = [colormap(i) for i in range(clustering_p.n_centroids)]
    
    scatter_colors = [cluster_colors[i] for i in df["cluster"]]
    cluster_counts = df["cluster"].value_counts()

    plt.figure(figsize=figsize)
    plt.scatter(df["x"], df["y"], c=scatter_colors, s=5)

    handles = []
    labels = []
    for i, count in cluster_counts.items():
        handles.append(plt.Line2D([0], [0], marker='o', color='w', 
                                markerfacecolor=cluster_colors[i], markersize=10))
        labels.append(f"cluster {i}: {cluster_counts.get(i, 0)}")

    plt.legend(handles=handles, labels=labels, fontsize=fontsize, loc="upper right")
    plt.xlabel(xlabel=x_label, fontsize=fontsize)
    plt.ylabel(ylabel=y_label, fontsize=fontsize)
    plt.title(title)

    if not (save_file_name is None):
        plt.savefig(save_file_name)

    plt.show()

    df.drop("x", axis=1, inplace=True)
    df.drop("y", axis=1, inplace=True)

    clusters_summary = df[clustering_p.features + ["cluster"]].groupby("cluster").agg(["mean", "std", "min", "max"])

    df.drop("cluster", axis=1, inplace=True)

    return clustering_results(
        silhouette_scr=silhouette_score(df[clustering_p.features], 
        cluster_labels),
        clusters_summary=clusters_summary,
        cluster_labels=cluster_labels,
    )

def get_cells_each_cluster(
    cells_df: pd.DataFrame, cluster_labels: np.ndarray) -> dict[int, list]:

    cells_df["cluster"] = cluster_labels
    cells_df["x"] = cells_df.index.str.split(":").str[0].astype(int)
    cells_df["y"] = cells_df.index.str.split(":").str[1].astype(int)

    cells_each_cluster = {}
    for cluster_label in np.unique(cluster_labels):

        cells_coord = list(cells_df[cells_df["cluster"] == cluster_label][["x", "y"]].itertuples(index=False, name=None))
        cells_each_cluster[cluster_label] = cells_coord

    cells_df.drop(["cluster", "x", "y"], axis=1, inplace=True)

    return cells_each_cluster

def get_vehicles_each_cluster(
    vehicles_df: pd.DataFrame, cluster_labels: np.ndarray) -> dict[int, list]:

    vehicles_df["cluster"] = cluster_labels

    vehicles_each_cluster = {}
    for cluster_label in np.unique(cluster_labels):

        vehicles_ids = list(vehicles_df[vehicles_df["cluster"] == cluster_label].index)
        vehicles_each_cluster[cluster_label] = vehicles_ids

    vehicles_df.drop(["cluster"], axis=1, inplace=True)

    return vehicles_each_cluster

from pathlib import PurePath

# Writes lines x,y,cluster starting from the smallest to the largest cluster;
def write_cluster_cells_to_file(
    cells_df: pd.DataFrame, clustering_res: clustering_results, file_path: PurePath):

    with open(file_path, "w") as f:

        cells_each_cluster = get_cells_each_cluster(
            cells_df=cells_df, cluster_labels=clustering_res.cluster_labels
        )
        smallest_to_largest_cluster = sorted(cells_each_cluster.keys(), key=lambda x: len(cells_each_cluster[x]))
        for key in smallest_to_largest_cluster:

            for cell in cells_each_cluster[key]:

                f.write(f"{cell[0]},{cell[1]},{key}\n")

# Writes lines id,cluster starting from the smallest to the largest cluster;
def write_cluster_vehicles_to_file(
    vehicles_df: pd.DataFrame, clustering_res: clustering_results, file_path: PurePath):

    with open(file_path, "w") as f:

        vehicles_each_cluster = get_vehicles_each_cluster(
            vehicles_df=vehicles_df, cluster_labels=clustering_res.cluster_labels
        )
        smallest_to_largest_cluster = sorted(vehicles_each_cluster.keys(), key=lambda x: len(vehicles_each_cluster[x]))
        for key in smallest_to_largest_cluster:

            for vehicle_id in vehicles_each_cluster[key]:

                f.write(f"{vehicle_id},{key}\n")