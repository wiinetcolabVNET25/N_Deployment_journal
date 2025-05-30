###NSGA-2

####Compile

`$ g++ nsga2.cpp -o nsga2`


####Execution

`$ ./nsga2 exp τ1 ... τn`

where exp is the experiment number and the n (number of clusters) other arguments are the model parameters τ. At the end of the execution, a folder called experiment$exp will be created, with the algorithm solutions and the baselines for comparison.
