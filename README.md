#### 1. datasets/

Contains the datasets for 6-8am and 5-7pm periods. The first period being used for the experiments, while the second was created in contrast to better describe characteristics of the first one, and to demonstrate n-deployment efficiency in section VIII: "Demonstrative Use Cases of the N-Deployment Strategy in Specific Scenarios", subsection A;

#### 2. clustering_results/

Contains a &lt;cells per cluster> file (cluster_cells.csv) used by NSGA-II, and a &lt;clustering_results> class in Python's pickle binary format https://docs.python.org/3/library/pickle.html (clustering_results.pkl) used by sequential deployment via grasp. The 1st, 2nd and 3rd clusters have labels 2, 1 and 0;

#### 3. Methods used for results in section VII: "N-DEPLOYMENT STRATEGY: EXPERIMENTS, RESULTS, AND ANALYSIS", subsections A, B, C and D; and section VIII "DEMONSTRATIVE USE CASES OF THE N-DEPLOYMENT STRATEGY IN SPECIFIC SCENARIOS", subsection B:

- **baseline_solution/**

- **homogenous_v2i_grasp_based_solution/**
  - **sequential/**: for generating one infrastructure;
  - **parallel/**: for multiple runs and coverage evaluation only;

#### 4. Methods used for results in section VII: "N-DEPLOYMENT STRATEGY: EXPERIMENTS, RESULTS, AND ANALYSIS", subsections E and G:

- **heterogeneous_v2i_sequential_deployment_by_grasp/**

	- version 1 (v1): used in subsection E comprising the two scenarios of different distributions of RSUs across clusters as shown in the table;

	- version 2 (v2): used in subsection G, comprising scenarios of different total number of RSUs across the clusters. The difference is this version tries to place all RSUs; if a number of RSUs does not fit in one cluster, the method will try to fit it in another one. This was used in comparison with the NSGA-II, in which the total number of RSUs was the parameter, as figures for each cluster show;

- **heterogeneous_v2i_NSGA-II/**