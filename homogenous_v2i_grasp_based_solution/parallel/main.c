#include "n-deployment_parallelizable.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <pthread.h>
#include "thpool.h"

#define MAX_INTERVAL_SIZE 1500
#define MAX_NUMBER_OF_THREADS 16

typedef struct
{
    int left;
    int right;
    int step;

} interval_t;

enum variable_quantity
{
    NONE,
    N_RSUS,
    TAU,
    RCL_LEN,
    N_ITER,
    N_CONT,
};

// At most one interval will have size greater than 1, which is identified by the enum;
typedef struct
{
    enum variable_quantity variable;

    interval_t n_rsus_interval;
    interval_t tau_interval;
    interval_t rcl_len_interval;
    interval_t n_iter_interval;
    interval_t n_cont_interval;

    int starting_seed;
    int num_threads;
    int num_works;

} experiments_parameters_t;

int parse_commandline_args(
    int argc, char **argv,
    experiments_parameters_t *experiments_parameters,
    char *output_input_file_path, char *output_error_msg);

typedef struct {

    n_deployment_input_t *n_deployment_input;
    enum variable_quantity variable;

} work_input_t;

void initialize_work_input_array(
    work_input_t *work_input_array, 
    trace_line *trace, int trace_size, experiments_parameters_t experiments_parameters);

void free_work_input(
    work_input_t *n_deployment_input);

void thread_task(void *arg)
{
    work_input_t *work_input = (work_input_t*) arg;
    n_deployment_input_t *n_deployment_input = work_input->n_deployment_input;

    n_deployment_output_t n_deployment_output;
    n_deployment_output = n_deployment(*n_deployment_input);

    printf("%d:%d,%d,%d,%d,%d,%d,%d\n",
        n_deployment_output.best_coverage,
        n_deployment_output.worst_coverage,
        n_deployment_input->number_of_rsus,
        n_deployment_input->contacts_time_threshold,
        n_deployment_input->grasp_rcl_len,
        n_deployment_input->n_deploy_num_ite,
        n_deployment_input->number_of_contacts,
        n_deployment_input->grasp_rng_seed
    );      

    free_work_input(work_input);
}

int main(int argc, char **argv)
{
    // In case any specified error below occurs;
    int status;
    char error_msg[300];
    error_msg[0] = '\0';

    // ==================== 1 - READ INPUT ==================== //

    char *correct_input_format = malloc(sizeof(argv[0]) + 500);
    sprintf(correct_input_format, "USAGE: %s -r <N_RSUS> -t <TAU> "
                                  "-l <RCL_LEN> -i <N_ITER> -c <N_CONT> -s <SEED> -n <N_THREADS> <TRACE_PATH>\n\n"
                                  "For setting an INTERVAL \"<left>:<right>:<step>\" (if not present, step's default is 1), choose at most one of:\n{<N_RSUS>, <TAU>, "
                                  "<RCL_LEN>, <N_ITER>, <N_CONT>}\n",
            argv[0]);

    // -------------------- 1.1 COMMAND LINE ------------------ //

    char input_file_path[MAX_INPUT_FILE_PATH_SIZE + 1];
    input_file_path[0] = '\0';
    experiments_parameters_t experiments_parameters;
    status = parse_commandline_args(argc, argv, &experiments_parameters, input_file_path, error_msg);
    if (status != 0)
    {
        printf("N-DEPLOYMENT: COMMAND LINE ERROR: %s\n\n%s\n",
               error_msg, correct_input_format);
        return 1;
    }

    // -------------------- 1.2 TRACE FILE -------------------- //

    trace_line *trace = (trace_line *)malloc(sizeof(trace_line) * MAX_TRACE_SIZE);
    if (!trace)
    {
        printf("N-DEPLOYMENT: MEMORY ERROR: can't allocate memory"
               " for %d trace lines (MAX_TRACE_SIZE)\n",
               MAX_TRACE_SIZE);
        return 1;
    }

    int trace_size;
    status = read_trace(input_file_path, trace, &trace_size, error_msg);
    if (status != 0)
    {
        printf("N-DEPLOYMENT: INPUT TRACE FILE ERROR: %s\n", error_msg);
        free(trace);
        return 1;
    }

    free(correct_input_format);

    // ==================== 2 - RUN EXPERIMENTS =========================== //

    // -------------------- Initialize thread pool -------------------- //
    threadpool t_pool = thpool_init(experiments_parameters.num_threads);

    // -------------------- Add work -------------------- //
    work_input_t* work_input_array =
        (work_input_t *)malloc(sizeof(work_input_t) * experiments_parameters.num_works);

    if (!work_input_array)
    {
        printf("N-DEPLOYMENT: MEMORY ERROR: can't allocate memory"
               " for %d work arguments\n",
               experiments_parameters.num_works);
        return 1;
    }
    for (int i = 0; i < experiments_parameters.num_works; i++)
    {
        work_input_array[i].n_deployment_input = 
        (n_deployment_input_t *) malloc(sizeof(n_deployment_input_t));

        memset(work_input_array[i].n_deployment_input, 0, sizeof(n_deployment_input_t));
    }

    // printf("allocated works\n");

    initialize_work_input_array(work_input_array, 
        trace, trace_size, experiments_parameters);

    // printf("initialized works\n");

    // USAGE: %s -r <N_RSUS> -t <TAU> "
    // "-l <RCL_LEN> -i <N_ITER> -c <N_CONT> -s <SEED> -n <N_THREADS> <TRACE_PATH>\n\n"
    // "For setting an INTERVAL \"<left>:<right>\", choose at most one of:\n{<N_RSUS>, <TAU>, "
    // "<RCL_LEN>, <N_ITER>, <N_CONT>}

    printf("INPUT:\n-r=%d:%d,-t=%d:%d,-l=%d:%d,-i=%d:%d,-c=%d:%d,-s=%d,-n=%d,trace=%s\n",
    experiments_parameters.n_rsus_interval.left, experiments_parameters.n_rsus_interval.right,
    experiments_parameters.tau_interval.left, experiments_parameters.tau_interval.right,
    experiments_parameters.rcl_len_interval.left, experiments_parameters.rcl_len_interval.right,
    experiments_parameters.n_iter_interval.left, experiments_parameters.n_iter_interval.right,
    experiments_parameters.n_cont_interval.left, experiments_parameters.n_cont_interval.right,
    experiments_parameters.starting_seed,
    experiments_parameters.num_threads,
    input_file_path);

    printf("RESULTS:\nbest_coverage:worst_coverage,-r,-t,-l,-i,-c,-s\n");

    for (int i = 0; i < experiments_parameters.num_works; i++)
    {
        int result = thpool_add_work(t_pool, thread_task, &work_input_array[i]);
        if (result)
        {
            printf("cannot add work\n");
            return 1;
        }
    }

    // printf("added %d works\n", experiments_parameters.num_works);

    // -------------------- Wait for threads -------------------- //
    thpool_wait(t_pool);
    thpool_destroy(t_pool);

    // ==================== 3 - FREE REMAINING RESOURCES ====== //
    free(trace);
    free(work_input_array);

    return 0;
}

int parse_positive_number(const char *arg, int *output)
{

    char *endptr;
    errno = 0;
    long val = strtol(arg, &endptr, 10);

    // Check for errors: non-numeric input, negative value, or zero
    if (errno != 0 || *endptr != '\0' || val <= 0)
    {
        return 1;
    }

    *output = (int)val;
    return 0;
}

int parse_interval_or_positive_number(const char *arg, interval_t *output) {

    char *temp = strdup(arg);  // Duplicate input to modify safely
    if (!temp) return 1;       // Handle strdup failure

    char *parts[3] = {NULL, NULL, NULL};
    char *token = strtok(temp, ":");
    int i = 0;

    while (token && i < 3) {
        parts[i++] = token;
        token = strtok(NULL, ":");
    }

    int result = 1;  // Assume failure initially

    if (i == 1) {

        // Single number case: left only
        if (parse_positive_number(parts[0], &(output->left)) == 0) {
            output->right = -1;
            output->step = -1;
            result = 0;
        }

    } else if (i == 2) {

        // Interval case: left:right
        if (parse_positive_number(parts[0], &(output->left)) == 0 &&
            parse_positive_number(parts[1], &(output->right)) == 0) {
            output->step = 1;  // Default step value
            result = 0;
        }

    } else if (i == 3) {

        // Interval with step: left:right:step
        if (parse_positive_number(parts[0], &(output->left)) == 0 &&
            parse_positive_number(parts[1], &(output->right)) == 0 &&
            parse_positive_number(parts[2], &(output->step)) == 0) {
            result = 0;
        }
    }

    free(temp);
    return result;
}

int parse_commandline_args(
    int argc, char **argv,
    experiments_parameters_t *experiments_parameters,
    char *output_input_file_path, char *output_error_msg)
{
    int opt;
    int num_intervals = 0;

    experiments_parameters->variable = NONE;
    experiments_parameters->num_works = 1;

    while ((opt = getopt(argc, argv, "r:t:l:i:c:s:n:")) != -1)
    {

        switch (opt)
        {

        // Arguments that can be intervals;
        case 'r':
        case 't':
        case 'l':
        case 'i':
        case 'c':
        {

            interval_t *target;
            char arg_name[20];
            if (opt == 'r')
            {
                target = &(experiments_parameters->n_rsus_interval);
                strcpy(arg_name, "<N_RSUS>");
            }
            if (opt == 't')
            {
                target = &(experiments_parameters->tau_interval);
                strcpy(arg_name, "<TAU>");
            }
            if (opt == 'l')
            {
                target = &(experiments_parameters->rcl_len_interval);
                strcpy(arg_name, "<RCL_LEN>");
            }
            if (opt == 'i')
            {
                target = &(experiments_parameters->n_iter_interval);
                strcpy(arg_name, "<N_ITER>");
            }
            if (opt == 'c')
            {
                target = &(experiments_parameters->n_cont_interval);
                strcpy(arg_name, "<N_CONT>");
            }

            if (parse_interval_or_positive_number(optarg, target))
            {
                sprintf(output_error_msg, "%s must be an interval of positive integers \"<left>:<right>:<step>\" or a positive integer", arg_name);
                return 1;
            }

            // It is an interval;
            if (target->right != -1)
            {

                num_intervals++;
                if (num_intervals > 1)
                {

                    sprintf(output_error_msg, "Multiple intervals are not allowed");
                    return 1;
                }

                if (target->left >= target->right)
                {
                    sprintf(output_error_msg, "An interval should be of positive integers \"<left>:<right>:<step>\" with left < right");
                    return 1;
                }

                if (target->right - target->left + 1 > MAX_INTERVAL_SIZE)
                {
                    sprintf(output_error_msg, "An interval size must not exceed %d", MAX_INTERVAL_SIZE);
                    return 1;
                }

                if (target->left + target->step > target->right)
                {
                    sprintf(output_error_msg, "%s's interval step is too big", arg_name);
                    return 1;
                }

                if (opt == 'r')
                    experiments_parameters->variable = N_RSUS;
                if (opt == 't')
                    experiments_parameters->variable = TAU;
                if (opt == 'l')
                    experiments_parameters->variable = RCL_LEN;
                if (opt == 'i')
                    experiments_parameters->variable = N_ITER;
                if (opt == 'c')
                    experiments_parameters->variable = N_CONT;

                experiments_parameters->num_works = ((target->right - target->left) / target->step) + 1;
            }
            else
            {

                if (opt == 'r' && target->left > MAX_NUMBER_OF_RSUS)
                {
                    sprintf(output_error_msg, "%s must not exceed %d", arg_name, MAX_NUMBER_OF_RSUS);
                    return 1;
                }
            }

            break;
        }

        // Arguments that can only be positive integers;
        case 's':
        case 'n':
        {

            int *target;
            char arg_name[20];
            if (opt == 's')
            {
                target = &(experiments_parameters->starting_seed);
                strcpy(arg_name, "<SEED>");
            }
            if (opt == 'n')
            {
                target = &(experiments_parameters->num_threads);
                strcpy(arg_name, "<N_THREADS>");
            }

            if (parse_positive_number(optarg, target))
            {
                sprintf(output_error_msg, "%s must be a positive integer", arg_name);
                return 1;
            }

            if (opt == 'n' && *target > MAX_NUMBER_OF_THREADS)
            {
                sprintf(output_error_msg, "%s must not exceed %d", arg_name, MAX_NUMBER_OF_THREADS);
                return 1;
            }

            break;
        }

        default:

            sprintf(output_error_msg, "%c is not a valid option", opt);
            return 1;
        }
    }

    // The remaining argument is the file path;
    if (optind >= argc)
    {
        sprintf(output_error_msg, "Missing one argument");
        return 1;
    }

    if (strlen(argv[optind]) > MAX_INPUT_FILE_PATH_SIZE)
    {
        sprintf(output_error_msg, "<TRACE_PATH> must not exceed %d bytes", MAX_INPUT_FILE_PATH_SIZE);
        return 1;
    }

    strcpy(output_input_file_path, argv[optind]);

    return 0;
}

void allocate_matrix(int ***matrix, const int w, const int h)
{
    *matrix = (int **) malloc(sizeof(int *) * w);
    for (int i = 0; i < w; i++)
    {
        (*matrix)[i] = (int *) malloc(sizeof(int) * h);
    }
}

void deallocate_matrix(int **matrix, const int w)
{
    for (int i = 0; i < w; i++)
    {
        free(matrix[i]);
    }

    free(matrix);
}

void initialize_work_input_array(
    work_input_t *work_input_array, 
    trace_line *trace, int trace_size, experiments_parameters_t experiments_parameters)
{
    n_deployment_input_t aux;
    aux.number_of_rsus = experiments_parameters.n_rsus_interval.left;
    aux.number_of_contacts = experiments_parameters.n_cont_interval.left;
    aux.contacts_time_threshold = experiments_parameters.tau_interval.left;

    aux.trace = trace;
    aux.trace_size = trace_size;

    aux.grasp_rng_seed = experiments_parameters.starting_seed;
    aux.n_deploy_num_ite = experiments_parameters.n_iter_interval.left;
    aux.grasp_rcl_len = experiments_parameters.rcl_len_interval.left;

    aux.cells_scores = NULL;

    // Scores don't change along experiments;
    if (experiments_parameters.variable != TAU && experiments_parameters.variable != N_CONT)
    {
        allocate_matrix(&(aux.cells_scores), 
            MAX_CELL_GRID_WIDTH, MAX_CELL_GRID_HEIGHT);

        // printf("allocated matrix\n");

        fill_scores_in_cells(trace, trace_size, 
            aux.cells_scores, 
            experiments_parameters.tau_interval.left, 
            experiments_parameters.n_cont_interval.left);

        // printf("filled scores matrix\n");

        // All arguments are constant, just copy the auxiliary struct;
        if (experiments_parameters.variable == NONE)
        {
            // copy_n_deployment_input_t(work_input_array[0].n_deployment_input, &aux);
            *(work_input_array[0].n_deployment_input) = aux;

            // printf("copy n-deployment input\n");

            work_input_array[0].variable = NONE;
        }
        // Number of RSUs changes;
        else if (experiments_parameters.variable == N_RSUS)
        {
            for (int i = 0; i < experiments_parameters.num_works; i++)
            {
                // copy_n_deployment_input_t(work_input_array[i].n_deployment_input, &aux);
                *(work_input_array[i].n_deployment_input) = aux;

                work_input_array[i].n_deployment_input->number_of_rsus = 
                experiments_parameters.n_rsus_interval.left + experiments_parameters.n_rsus_interval.step * i;

                work_input_array[i].n_deployment_input->grasp_rng_seed =
                experiments_parameters.starting_seed + i;

                work_input_array[i].variable = N_RSUS;
            }
        }
        // Length of restricted candidate list changes;
        else if (experiments_parameters.variable == RCL_LEN)
        {
            for (int i = 0; i < experiments_parameters.num_works; i++)
            {
                // copy_n_deployment_input_t(work_input_array[i].n_deployment_input, &aux);
                *(work_input_array[i].n_deployment_input) = aux;

                work_input_array[i].n_deployment_input->grasp_rcl_len = 
                experiments_parameters.rcl_len_interval.left + experiments_parameters.rcl_len_interval.step * i;

                work_input_array[i].n_deployment_input->grasp_rng_seed =
                experiments_parameters.starting_seed + i;

                work_input_array[i].variable = RCL_LEN;
            }
        }
        // Number of iterations changes;
        else if (experiments_parameters.variable == N_ITER)
        {
            for (int i = 0; i < experiments_parameters.num_works; i++)
            {
                // copy_n_deployment_input_t(work_input_array[i].n_deployment_input, &aux);
                *(work_input_array[i].n_deployment_input) = aux;

                work_input_array[i].n_deployment_input->n_deploy_num_ite = 
                experiments_parameters.n_iter_interval.left + experiments_parameters.n_iter_interval.step * i;

                work_input_array[i].n_deployment_input->grasp_rng_seed =
                experiments_parameters.starting_seed + i;

                work_input_array[i].variable = N_ITER;
            }
        }
    }
    // Scores change over the experiments
    else
    {
        if (experiments_parameters.variable == TAU)
        {
            for (int i = 0; i < experiments_parameters.num_works; i++)
            {
                allocate_matrix(&(aux.cells_scores), 
                    MAX_CELL_GRID_WIDTH, MAX_CELL_GRID_HEIGHT);
    
                fill_scores_in_cells(trace, trace_size, 
                    aux.cells_scores, 
                    experiments_parameters.tau_interval.left + experiments_parameters.tau_interval.step * i, 
                    experiments_parameters.n_cont_interval.left);
    
                // copy_n_deployment_input_t(work_input_array[i].n_deployment_input, &aux);
                *(work_input_array[i].n_deployment_input) = aux;

                work_input_array[i].n_deployment_input->contacts_time_threshold = 
                experiments_parameters.tau_interval.left + experiments_parameters.tau_interval.step * i;

                work_input_array[i].variable = TAU;
            }

        }
        else if (experiments_parameters.variable == N_CONT)
        {
            for (int i = 0; i < experiments_parameters.num_works; i ++)
            {
                allocate_matrix(&(aux.cells_scores), 
                    MAX_CELL_GRID_WIDTH, MAX_CELL_GRID_HEIGHT);
    
                fill_scores_in_cells(trace, trace_size, 
                    aux.cells_scores, 
                    experiments_parameters.tau_interval.left, 
                    experiments_parameters.n_cont_interval.left + experiments_parameters.n_cont_interval.step * i);
    
                // copy_n_deployment_input_t(work_input_array[i].n_deployment_input, &aux);
                *(work_input_array[i].n_deployment_input) = aux;

                work_input_array[i].n_deployment_input->number_of_contacts = 
                experiments_parameters.n_cont_interval.left + experiments_parameters.n_cont_interval.step * i;

                work_input_array[i].variable = N_CONT;
            }
        }
    }
}

void free_work_input(
    work_input_t *work_input)
{
    if (!work_input || !work_input->n_deployment_input) return;

    // Free scores early only when tau or number of contacts are variables, as threads do not share the scores;
    if (work_input->variable == TAU || work_input->variable == N_CONT)
    {
        deallocate_matrix(work_input->n_deployment_input->cells_scores, MAX_CELL_GRID_WIDTH);
        // printf("freed scores\n");
    }

    free(work_input->n_deployment_input);
}