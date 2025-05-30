Deployment method of putting RSUs mainly on entrance points, so that vehicles reach N contacts within a tau time threshold. This parallel version does not output solutions, only the objective value (vehicle coverage);

- N = 1 -> Primary Deployment Strategy;
- N = 2 -> 2CON Deployment Strategy;

# Compilation:

    make build

# Execution:

    ./n-deployment_parallel -r <N_RSUS> -t <TAU> -l <RCL_LEN> -i <N_ITER> -c <N_CONT> -s <SEED> -n <N_THREADS> <TRACE_PATH>

- &lt;N_RSUS>: number of cells to be chosen for infrastructure positioning;

- &lt;TAU>: tau time threshold;

- &lt;RCL_LEN>: restricted candidates list length for building solutions incrementally;

- &lt;N_ITER>: number of solutions to build in the search;

- &lt;N_CONT>: number of required contacts;

- &lt;SEED>: random number generator seed for building the solutions;

- &lt;N_THREADS>: number of threads to execute jobs. The number of jobs is equal to one of the inputs interval size. The interval (&lt;left>:&lt;right>:&lt;step>) can be set for one of the inputs: {&lt;N_RSUS>, &lt;TAU>, &lt;RCL_LEN>, &lt;N_ITER>, &lt;N_CONT>}

- &lt;TRACE_PATH>: vehicle trace file, in the datasets format;

# Output:

- Input values and results are printed in the standard output;

- For each input combination, output contains lines of "best_coverage:worst_coverage,-r,-t,-l,-i,-c,-s", that is, the best and worst solutions coverage and the input parameters;