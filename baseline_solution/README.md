Deployment method of putting RSUs in the most frequented cells;

# Compilation:

    make build

# Execution:

    ./baseline n trace

- n: number of cells to be chosen for infrastructure positioning, from the first ones with the most vehicle flow; the maximum is set as 1024; if the maximum is exceeded, an error message is returned. Otherwise, if there are fewer cells frequented by vehicles than n, all the frequented ones are filled and no error occurs;

- trace: vehicle trace file, in the datasets format;

# Output:

- execution summary file "n_summary.txt" contains execution time, input number of rsus (n) and actual number selected;

- infrastructure file: "n_rsus.txt" contains pairs "x,y" separated by ";";