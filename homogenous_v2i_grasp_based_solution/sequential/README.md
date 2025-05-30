Deployment method of putting RSUs mainly on entrance points, so that vehicles reach N contacts within a tau time threshold;

- N = 1 -> Primary Deployment Strategy;
- N = 2 -> 2CON Deployment Strategy;

# Compilation:

    make build

# Execution:

    ./n-deployment <num of rsus> <contacts time threshold> <GRASP's rcl length> <n-deploy num iterations> <num of contacts> <GRASP's seed> <trace file path>

- &lt;num of rsus>: number of cells to be chosen for infrastructure positioning;

- &lt;contacts time threshold>: tau time threshold;

- &lt;GRASP's rcl length>: restricted candidates list length for building solutions incrementally;

- &lt;n-deploy num iterations>: number of solutions to build in the search;

- &lt;num of contacts>: number of required contacts;

- &lt;GRASP's seed>: random number generator seed for building the solutions;

- &lt;trace file path>: vehicle trace file, in the datasets format;

# Output:

- files starting with "rsu=rsu_tau=tau_rcl=rcl_len_iter=iter_cont=n_", where each term after "=" is substituted by its input value:

    - execution summary file "_summary.txt" contains execution time, input values and number of vehicles covered;

    - infrastructure file: "_rsus.txt" contains pairs "x,y" separated by ";". If no solution is found, it will contain only (0, 0) cells. If the number of rsus to deploy is too big, the remaining cells will padded with (0, 0). So the trace should not have (0, 0) cells;

    - coverage evolution file "_coverage_log.txt" contains the vehicle coverage with each intermediate solution in the search, separated by ";";