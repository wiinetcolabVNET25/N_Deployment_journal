// Differences in API from v2:
// - No read from command line (experiments should do it)
// - No output functions (only returns coverage (best and worst))
// - Simpler n-deployment input (one argument)
// - Input does not contain scores matrix, only a pointer to it to save space (experiments should initialize the scores with the appropriate dimensions). Apparently does not increase execution time much even with more memcpy calls;

// Difference in implementation:
// Using drand48_r instead of rand, for thread safety;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_CELL_GRID_WIDTH 100
#define MAX_CELL_GRID_HEIGHT 100

#define MAX_TRACE_SIZE 2200000
#define MAX_NUMBER_OF_VEHICLES 80000
// max solution size;
#define MAX_NUMBER_OF_RSUS 1500

#define MAX_INPUT_FILE_PATH_SIZE 100

typedef struct
{
    int vehicle_id;
    int time;
    int grid_x_pos;
    int grid_y_pos;
    int r;

} trace_line;

typedef struct
{
    int x;
    int y;

} pos_2d;

typedef struct
{
    // Problem instance;
    int number_of_rsus;
    int number_of_contacts;
    int contacts_time_threshold;

    trace_line *trace;
    int trace_size;

    // Algorithm parameters;
    int grasp_rng_seed;
    int n_deploy_num_ite;
    int grasp_rcl_len;

    // To avoid recomputing cells scores when building a solution;
    int** cells_scores;

} n_deployment_input_t;

typedef struct
{
    int best_coverage;
    int worst_coverage;

} n_deployment_output_t;

// ==================== INPUT FUNCTIONS ==================== //
// If succeedes, returns 0 and output_trace and output_trace_size can be used;
// Otherwise, returns 1 and error_msg can be used;
int read_trace(
    const char *trace_file_name,
    trace_line *output_trace, int *output_trace_size,
    char *output_error_msg);

// ==================== N-DEPLOYMENT ==================== //
// Necessary to avoid score recomputation;
void fill_scores_in_cells(
    trace_line *trace,
    int tam, int **cells,
    int time_interval, int number_of_contacts);

// Returns number of covered vehicles;
n_deployment_output_t n_deployment(
    n_deployment_input_t n_deployment_input);