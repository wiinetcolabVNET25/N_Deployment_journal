# In v2, the number of rsus not deployed yet in one cluster are added in the next clusters deployments
# (because GRASP could not improve the solutions or the cluster size was too small);

# In this way, one can specify the total number of RSUs as the input parameter, 
# which are divided equally in the clusters;

import pandas as pd
import numpy as np
from pathlib import Path, PurePath

from . import clustering, infra_eval, abs_deployment_param, n_deployment_functions, run_deployment_utils

class clusters_deployment_param:

    def __init__(self, clusters_iterator: list[int],
                 deployments_param: list[abs_deployment_param.deployment_param],
                 progressive_coverage_metrics: list[infra_eval.metric]):
    
        if len(clusters_iterator) != len(deployments_param):
            raise ValueError("Each cluster deployment must have their respective parameters")

        self.clusters_iterator = clusters_iterator
        self.deployments_param = deployments_param
        self.progressive_coverage_metrics = progressive_coverage_metrics

    def __str__(self):
        
        str_repr = "PARAMETERS:\n"
        for param in self.deployments_param:
            str_repr += f"{str(param)}\n"

        str_repr += "METRICS:\n"
        for metric in self.progressive_coverage_metrics:
            str_repr += f"{str(metric)}\n"

        return str_repr

class clusters_deployment_results:

    def __init__(self, cluster_iterator: list,
                 clusters_deployment_infras: dict[int, list],
                 clusters_deployment_covered_vehicles: dict[int, int]):
        self.cluster_iterator = cluster_iterator
        self.clusters_deployment_infras = clusters_deployment_infras
        self.clusters_deployment_covered_vehicles = clusters_deployment_covered_vehicles

    def __str__(self):
        
        str_repr = f"[+] infrastructures sizes:\n"
        for cluster in self.cluster_iterator:
            str_repr += f"\tcluster {cluster}: {len(self.clusters_deployment_infras[cluster])}\n"

        str_repr += "[+] each cluster's coverage\n"
        for cluster in self.cluster_iterator:
            str_repr += f"\tcluster {cluster}: {len(self.clusters_deployment_covered_vehicles[cluster])}\n"

        return str_repr

class progressive_clustering_deployment_results:

    def __init__(self, results_file_path: PurePath = None):
        if results_file_path is None:
            self.results = {}
        else:
            self.results = self._load_results_binary(results_file_path)

    def add_clustering_iterator(self, iterator):
        if "cluster_iterator" in self.results:
            return
        self.results["cluster_iterator"] = iterator

    def add_clustering_results(self, results: clustering.clustering_results):
        if "clustering" in self.results:
            return
        self.results["clustering"] = results

    def add_clusters_deployment_results(self, results: clusters_deployment_results):
        if "clusters_n_deployment" in self.results:
            return
        self.results["clusters_deployment"] = results

    def add_overall_infra_evaluation_results(self, results: infra_eval.overall_infra_evaluation_results):
        if "overall_infra_evaluation" in self.results:
            return
        self.results["overall_infra_evaluation"] = results

    def _load_results_binary(self, file_path: PurePath):
        raise NotImplementedError()

    def store_results_binary(self, file_path: PurePath):
        raise NotImplementedError()

    def print(self):
        print("---------- PROGRESSIVE CLUSTERING DEPLOYMENT RESULTS ----------\n\n")
        print("========== CLUSTERING RESULTS ==========\n")
        print(str(self.results["clustering"]))
        print("\n========== PROGRESSIVE DEPLOYMENT RESULTS =========\n")
        print(str(self.results["clusters_deployment"]))
        print("========== OVERALL INFRASTRUCTURE EVALUATION ===========\n")
        print(str(self.results["overall_infra_evaluation"]))
        print("========================================\n")

from functools import reduce

# if log, shows execution details;

# if delete generated files, deletes generated traces and deployments outputs;
# clustering must've been calculated before;

# if clusters_generated_files_directory is not None, 
# creates within the directory directories cluster_i (i = cluster label) 
# with the files generated on deployment, even if delete generated files is on;
def run_progressive_cells_clustering_deployment(
    trace_df: pd.DataFrame,
    cells_features_df: pd.DataFrame,

    clustering_precomp_res: clustering.clustering_results,

    clusters_deployment_p: clusters_deployment_param,

    overall_infra_evaluation_p: infra_eval.overall_infra_evaluation_param,

    log: bool = False,
    delete_generated_files: bool = False,
    clusters_generated_files_directory: Path = None,
) -> progressive_clustering_deployment_results:
    
    main_res = progressive_clustering_deployment_results()

    # ========== STEP 1 - GET CLUSTERS =================
    # ==================================================
    if log:
        print("==================== STEP 1 - CLUSTERING ==========")
        print("==================================================\n")
        print("----==== using precomputed clusters")

    main_res.add_clustering_results(results=clustering_precomp_res)

    if log:
        print(f"\t[+] clustering results:\n{str(clustering_precomp_res)}")

    # ========== STEP 2 - DEPLOY ON EACH CLUSTER =======
    # ==================================================

    main_res.add_clustering_iterator(iterator=clusters_deployment_p.clusters_iterator)

    if log:
        print("\n==================== STEP 2 - DEPLOYMENTS ======")
        print("==================================================")
        print(f"\n----==== deploying on order: {clusters_deployment_p.clusters_iterator}\n")

    # start deployment;
    cells_each_cluster = clustering.get_cells_each_cluster(
        cells_df=cells_features_df, 
        cluster_labels=clustering_precomp_res.cluster_labels
    )

    # cov_fleet_with_each_new_cluster_deployment: K -> V;
    # 
    # K: id of cluster in which a new deployment was made (order can later be accessed in the cluster_iterator)
    # V: vehicles covered by the new deployment plus the other deployments (saved on infrastructures_each_cluster).
    # (considering the current cluster n_deployment_metric)
    # 
    # The vehicles already covered (other V values) are not counted:
    # 
    # 1. if k1 != k2, intersection(cov_fleet[k1], cov_fleet[k2]) == empty;
    # 
    # Also:
    #
    # 2. all_vehicles set includes union(cov_fleet[ki], for every i); 
    covered_fleet_with_each_new_cluster_deployment = {}
    infrastructures_each_cluster = {}
    
    # Number of RSUs not deployed in previous clusters as specified in the input,
    # (because GRASP could not improve the solutions or the cluster size was too small)
    rho_balance = 0
    for cluster, deployment_p, metric in zip(
        clusters_deployment_p.clusters_iterator, 
        clusters_deployment_p.deployments_param,
        clusters_deployment_p.progressive_coverage_metrics):

        if log:
            print(f"----==== working on cluster {cluster}\n")

        covered_fleet_ids_all_clusters = reduce(
            lambda x, y: x + y, covered_fleet_with_each_new_cluster_deployment.values(), []
        )

        if log:
            print(f"\t[+] number of vehicles covered by any metric: {len(covered_fleet_ids_all_clusters)}")
        
        vehicles_not_covered_yet_indexes = run_deployment_utils.get_trace_with_no_vehicles_in_list_indexes(
            trace_df=trace_df, vehicles=covered_fleet_ids_all_clusters
        )

        if log:
        # checking if each vehicle is counted once and then excluded;

            # if only one cluster was used yet, no test is done 
            # (otherwise the intersection of the coverages for each cluster
            # would not be empty as it should be because of the universal set);
            if len(covered_fleet_with_each_new_cluster_deployment) == 1:
                print(f"\t[+] no vehicle is covered twice correctly")

            else:

                class UniversalSet(set):
                    def __and__(self, other):
                        return other
                    def __rand__(self, other):
                        return other

                covered_fleet_each_cluster_intersection = reduce(
                    lambda x, y: x & y, 
                    [set(v) for v in covered_fleet_with_each_new_cluster_deployment.values()],
                    UniversalSet()
                )

                if len(covered_fleet_each_cluster_intersection) == 0:
                    print(f"\t[+] no vehicle is covered twice correctly")
                else:
                    print("\t[+] some vehicle was covered twice")

        # Writing cluster trace file: intersection between cluster cells and remaining trajectories;
        # vehicles ids are mapped to 0, n - 1, for n-deployment execution;
        if log:
            print(f"\t[+] writing trace file for cluster {cluster}")

        trace_df["(x, y)"] = list(zip(trace_df["x"], trace_df["y"]))
        remaining_fleet_trace = trace_df.iloc[vehicles_not_covered_yet_indexes]
        cluster_trace_indexes = remaining_fleet_trace[remaining_fleet_trace["(x, y)"].isin(cells_each_cluster[cluster])].index
        trace_df.drop(labels=["(x, y)"], axis=1, inplace=True)

        cluster_trace_path = Path(".") / f"cluster_{cluster}_trace.csv"
        run_deployment_utils.write_trace_file_with_ids_mapped_to_first_naturals(
            trace_df=trace_df.iloc[cluster_trace_indexes], file_path=cluster_trace_path
        )

        deployment_p.trace_path = cluster_trace_path
        deployment_p.num_rsus += rho_balance
        rho_balance = 0
        input_num_rsus = deployment_p.num_rsus

        if log:
            print(f"\t[+] infra size to apply:", input_num_rsus)
            print(f"\t[+] infra size balance:", rho_balance)

        run_deployment_utils.run_program_and_wait(file_path=deployment_p.executable_path, cmdline_args=deployment_p.to_cmdline_args())

        if log:
            print(f"\t[+] deploying on cluster {cluster} ", end="")

            if isinstance(deployment_p, n_deployment_functions.n_deployment_param):
                print(" (N-Deployment method) ")
            else:
                cluster_trace_path.unlink()
                raise ValueError(f"invalid deployment method ({type(deployment_p).__name__})")

            print(f"parameters: {str(deployment_p)}")

        # Reading results and getting new coverage;
        if isinstance(deployment_p, n_deployment_functions.n_deployment_param):
            infrastructure_path = n_deployment_functions.get_infrastructure_path(n_deployment_p=deployment_p)
        else:
            cluster_trace_path.unlink()
            raise ValueError(f"invalid deployment method ({type(deployment_p).__name__})")

        new_infrastructure = run_deployment_utils.read_infrastructure_csv(file_path=infrastructure_path)
        if (0, 0) in new_infrastructure:
            new_infrastructure.remove((0, 0))

        if log:
            print(f"\t[+] infra applied size:", len(new_infrastructure))

        rho_balance += (input_num_rsus - len(new_infrastructure))

        if log:
            print(f"\t[+] infra size balance:", rho_balance)

        infrastructures_each_cluster[cluster] = new_infrastructure

        if log:
            print(f"\t[+] evaluating new fleet covered with accumulative deployments (metric {metric.name()})")
            
        cumulative_infrastructure = reduce(
            lambda x, y: x + y, infrastructures_each_cluster.values(), []
        )
        remaining_fleet_trace = trace_df.iloc[vehicles_not_covered_yet_indexes]
        metric.evaluate_infra_and_save_result(
            trace_df=remaining_fleet_trace, infrastructure=cumulative_infrastructure)

        if log:
            print(f"\t[+] evaluation results:\n{metric.get_last_eval_str()}")

        covered_vehicles = metric.get_last_eval()["covered_vehicles"]
        covered_fleet_with_each_new_cluster_deployment[cluster] = covered_vehicles

        if log:
            print(f"\n\t[+] num. new vehicles covered after new deployment: {len(covered_vehicles)}")
            print((f"\t[+] total num. vehicles covered after new deployment: " 
                   + f"{len(reduce(lambda x, y: x + y, covered_fleet_with_each_new_cluster_deployment.values(), []))}"))
            print(f"\n----==== ended working on cluster {cluster} ====-\n")

        if not (clusters_generated_files_directory is None):

            target_dir = clusters_generated_files_directory / f"cluster_{cluster}"
            target_dir.mkdir(parents=True, exist_ok=True)

            import shutil

            infrastructure_path = n_deployment_functions.get_infrastructure_path(n_deployment_p=deployment_p)
            shutil.move(str(infrastructure_path), str(target_dir / infrastructure_path.name))
            best_cov_log_path = n_deployment_functions.get_best_coverage_log_path(n_deployment_p=deployment_p)
            shutil.move(str(best_cov_log_path), str(target_dir / best_cov_log_path.name))
            summary_path = n_deployment_functions.get_summary_path(n_deployment_p=deployment_p)
            shutil.move(str(summary_path), str(target_dir / summary_path.name))

        if delete_generated_files:
            cluster_trace_path.unlink()
            n_deployment_functions.delete_n_deployment_output_files(n_deployment_p=deployment_p)

    main_res.add_clusters_deployment_results(
        results=clusters_deployment_results(
            cluster_iterator=clusters_deployment_p.clusters_iterator,
            clusters_deployment_infras=infrastructures_each_cluster,
            clusters_deployment_covered_vehicles=covered_fleet_with_each_new_cluster_deployment
        )
    )

    # ========== STEP 3 - FINAL EVALUATION =============
    # ==================================================

    if log:
        print("\n==================== STEP 3 ======================")
        print("==================================================")
        print("\n----==== running overall infra evaluation\n")
        print("\t[+] metrics to evaluate:")
        for metric in overall_infra_evaluation_p.metrics:
            print(f"\t{metric.name()}")

    overall_infra = reduce(lambda x, y: set(x) | set(y), infrastructures_each_cluster.values(), [])

    infra_eval.evaluate_infrastructure_with_metrics(
        metrics=overall_infra_evaluation_p.metrics, 
        trace_df=trace_df, 
        infrastructure=overall_infra
    )

    overall_infra_evaluation_res = infra_eval.overall_infra_evaluation_results(
        metrics=overall_infra_evaluation_p.metrics
    )
    main_res.add_overall_infra_evaluation_results(
        results=overall_infra_evaluation_res
    )

    if log:
        print(f"\t[+] overall infra size: {len(overall_infra)}")
        print("\t[+] evaluation results:")
        overall_infra_evaluation_res.print_results()

    return main_res