#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#define MAX_CELL_GRID_WIDTH         100
#define MAX_CELL_GRID_HEIGHT        100

#define MAX_TRACE_SIZE              2200000
#define MAX_NUMBER_OF_VEHICLES      80000
// max solution size;
#define MAX_NUMBER_OF_RSUS          1024

#define MAX_INPUT_FILE_PATH_SIZE    100

typedef struct trace_ 
{
    int vehicle_id;
    int time;
    int grid_x_pos;
    int grid_y_pos;
    int r;

} trace_line;

typedef struct solution_
{
    int x;
    int y;

} pos_2d;

typedef struct n_deployment_input_ 
{
    // Problem instance;
    int number_of_rsus;
    int number_of_contacts;
    int contacts_time_threshold;
    
    // Algorithm parameters;
    int grasp_rng_seed;
    int n_deploy_num_ite;
    int grasp_rcl_len;

    // To avoid recomputing cells scores when building a solution;
    int cells[MAX_CELL_GRID_WIDTH][MAX_CELL_GRID_HEIGHT];

} struct_n_deployment_input;

typedef struct n_deployment_output_
{
    double mean_ite_execution_time;
    double ite_execution_time;
    pos_2d best_solution[MAX_NUMBER_OF_RSUS];
    int best_solution_obj_f_value;

} struct_n_deployment_output;

// ==================== INPUT FUNCTIONS ==================== //
// If succeedes, returns 0 and n_deployment_input can be used;
// Otherwise, returns 1 and error_msg can be used;
int read_commandline_args(
    int argc, char **argv, 
    struct_n_deployment_input *output_n_deployment_input, 
    char* output_input_file_path, char* output_error_msg
);

int try_to_get_positive_int_from_arg(
    char* arg, char* var_name, 
    int* output, 
    char* output_error_msg
);

// If succeedes, returns 0 and output_trace and output_trace_size can be used;
// Otherwise, returns 1 and error_msg can be used;
int read_trace(
    const char* trace_file_name, 
    trace_line *output_trace, int *output_trace_size, 
    char* output_error_msg
);
// ==================== INPUT FUNCTIONS ==================== //


// ==================== N-DEPLOYMENT FUNCTIONS ==================== //
void n_deployment(
    trace_line *trace, int trace_size, 
    struct_n_deployment_input n_deployment_input, 
    struct_n_deployment_output* output_n_deployment_output,
    FILE* coverage_log_file
);

void fill_scores_in_cells(
    trace_line *trace, 
    int tam, int cells[ MAX_CELL_GRID_WIDTH ][ MAX_CELL_GRID_HEIGHT ], 
    int time_interval, int number_of_contacts
);

void reset_cells(int cells[ MAX_CELL_GRID_WIDTH ][ MAX_CELL_GRID_HEIGHT ]);

void reset_score_of_cells_having_rsu(
    int cells[ MAX_CELL_GRID_WIDTH ][ MAX_CELL_GRID_HEIGHT ], 
    pos_2d *solution, int solution_size
);

void get_cell_with_highest_score(
    int cells[ MAX_CELL_GRID_WIDTH ][ MAX_CELL_GRID_HEIGHT ], int *_x, int *_y
);

int check_coverage(
    trace_line *trace, int trace_size, 
    pos_2d *solution, int solution_size, 
    int *num_of_contacts, int cells[MAX_CELL_GRID_WIDTH][MAX_CELL_GRID_HEIGHT],
    int time_interval, int number_of_contacts
);

void reset_vehicles(int *vehicles, int tam);
// ==================== N-DEPLOYMENT FUNCTIONS ==================== //

// ==================== OUTPUT FUNCTIONS ==================== //
int write_summary_to_file(
    struct_n_deployment_input n_deployment_input, 
    struct_n_deployment_output n_deployment_output, 
    char *output_error_msg
);

int write_rsus_to_file(
    struct_n_deployment_input n_deployment_input, 
    pos_2d* rsus_pos, int num_rsus,
    char *output_error_msg
);
// ==================== OUTPUT FUNCTIONS ==================== //

int main(int argc, char **argv)
{
    // In case any specified error below occurs;
    int status;
    char error_msg[300];
    error_msg[0] = '\0';

    // ==================== 1 - READ INPUT ==================== //

    const char* correct_input_format = "USAGE: n-deployment <num of rsus> <contacts time threshold> "
    "<GRASP's rcl length> <n-deploy num iterations> <num of contacts> <GRASP's seed> <trace file path>";

    // -------------------- 1.1 COMMAND LINE ------------------ //

    char input_file_path[MAX_INPUT_FILE_PATH_SIZE + 1];
    input_file_path[0] = '\0';
    struct_n_deployment_input n_deployment_input;
    status = read_commandline_args(argc, argv, &n_deployment_input, input_file_path, error_msg);
    if (status != 0)
    {
        fprintf(stderr, "N-DEPLOYMENT: COMMAND LINE ERROR: %s\n\n%s\n", 
        error_msg, correct_input_format);
        return 1;
    }

    // -------------------- 1.2 TRACE FILE -------------------- //

    trace_line *trace = (trace_line *) malloc(sizeof(trace_line) * MAX_TRACE_SIZE);
    if (!trace)
    {
        fprintf(stderr, "N-DEPLOYMENT: MEMORY ERROR: can't allocate memory"
        " for %d trace lines (MAX_TRACE_SIZE)\n", MAX_TRACE_SIZE);
        return 1;
    }

    int trace_size;
    status = read_trace(input_file_path, trace, &trace_size, error_msg);
    if (status != 0)
    {
        fprintf(stderr, "N-DEPLOYMENT: INPUT TRACE FILE ERROR: %s\n", error_msg);
        free(trace);
        return 1;
    }

    fill_scores_in_cells(trace, trace_size, 
    n_deployment_input.cells, n_deployment_input.contacts_time_threshold, n_deployment_input.number_of_contacts);

    // ==================== 2 - RUN AND WRITE COVERAGE LOG =========================== //
    char coverage_log_file_name[MAX_INPUT_FILE_PATH_SIZE + 250];
    sprintf(coverage_log_file_name, "rsu=%d_tau=%d_rcl=%d_iter=%d_cont=%d_coverage_log.txt", 
    n_deployment_input.number_of_rsus, n_deployment_input.contacts_time_threshold, 
    n_deployment_input.grasp_rcl_len, n_deployment_input.n_deploy_num_ite, n_deployment_input.number_of_contacts);

    FILE* coverage_log_file = fopen(coverage_log_file_name, "w");
    if (coverage_log_file == NULL)
    {
        fprintf(stderr, "N-DEPLOYMENT: OUTPUT FILE ERROR: can't write coverage log file\n");
        free(trace);
        return 1;
    }

    struct_n_deployment_output n_deployment_output;
    n_deployment(trace, trace_size, n_deployment_input, &n_deployment_output, coverage_log_file);

    fclose(coverage_log_file);
    // ==================== 3 - WRITE OTHER RESULTS: SUMMARY AND RSUS ================= //
    status = write_summary_to_file(n_deployment_input, n_deployment_output, error_msg);
    if (status != 0)
    {
        fprintf(stderr, "N-DEPLOYMENT: OUTPUT FILE ERROR: %s\n", error_msg);
        free(trace);
        return 1;
    }

    status = write_rsus_to_file(n_deployment_input, n_deployment_output.best_solution,  n_deployment_input.number_of_rsus, error_msg);
    if (status != 0)
    {
        fprintf(stderr, "N-DEPLOYMENT: OUTPUT FILE ERROR: %s\n", error_msg);
        free(trace);
        return 1;
    }

    // ==================== 4 - FREE REMAINING RESOURCES ====== //
    free(trace);
    
    return 0;
}

int read_commandline_args(int argc, char **argv, struct_n_deployment_input *output_n_deployment_input, 
char* output_input_file_path, char* output_error_msg)
{
    if (argc != 8)
    {
        strcpy(output_error_msg, "wrong number of arguments");
        return 1;
    }

    // ========== Try to convert all arguments to correct numeric values; ========== //
    int status;

    status = try_to_get_positive_int_from_arg(argv[1], "<num of rsus>", 
    &(output_n_deployment_input->number_of_rsus), output_error_msg);
    if (status != 0)
    {
        return 1;
    }
    if (output_n_deployment_input->number_of_rsus > MAX_NUMBER_OF_RSUS)
    {        
        sprintf(output_error_msg, "number of RSUs must be at most %d", MAX_NUMBER_OF_RSUS);
        return 1;
    }
    
    status = try_to_get_positive_int_from_arg(argv[2], "<contacts time threshold>", 
    &(output_n_deployment_input->contacts_time_threshold), output_error_msg);
    if (status != 0)
    {
        return 1;
    }
    status = try_to_get_positive_int_from_arg(argv[3], "<GRASP's rcl length>", 
    &(output_n_deployment_input->grasp_rcl_len), output_error_msg);
    if (status != 0)
    {
        return 1;
    }
    status = try_to_get_positive_int_from_arg(argv[4], "<n-deploy num iterations>", 
    &(output_n_deployment_input->n_deploy_num_ite), output_error_msg);
    if (status != 0)
    {
        return 1;
    }
    status = try_to_get_positive_int_from_arg(argv[5], "<num of contacts>", 
    &(output_n_deployment_input->number_of_contacts), output_error_msg);
    if (status != 0)
    {
        return 1;
    }
    status = try_to_get_positive_int_from_arg(argv[6], "<GRASP's seed>", 
    &(output_n_deployment_input->grasp_rng_seed), output_error_msg);
    if (status != 0)
    {
        return 1;
    }

    // ========== try to get input file path ========== //
    if (strlen(argv[7]) > MAX_INPUT_FILE_PATH_SIZE)
    {
        sprintf(output_error_msg, "file path is bigger than %d bytes", MAX_INPUT_FILE_PATH_SIZE);
        return 1;
    }
    else {

        strcpy(output_input_file_path, argv[7]);
    }

    return 0;
}

// Get int from command line argument;
// IMPORTANT: Assumes int is big enough for converted value;
int try_to_get_positive_int_from_arg(char* arg, char* var_name, int* output, 
char* output_error_msg)
{
    int value = atoi(arg);
    if (value <= 0)
    {
        sprintf(output_error_msg, "%s must be a positive integer", var_name);
        return 1;
    }

    *output = value;

    return 0;
}

// IMPORTANT: 
// Assumes, for each vehicle, its trace is composed by adjacent lines,
// int is big enough for every value, no line is bigger than 200 bytes,
// and fields are integers separated only by ';' and line ends with ';';
int read_trace(const char* trace_file_name, trace_line* output_trace, int *output_trace_size, 
char* output_error_msg)
{
    int num_seen_vehicles = 0;
    int last_seen_vehicle_id = -1;

    FILE* f_stream = fopen(trace_file_name, "r");

    if (f_stream == NULL)
    {
        sprintf(output_error_msg, "can't open file \"%s\"", trace_file_name);
        return 1;
    }

    char line_buffer[201];
    trace_line aux;
    int num_read_lines = 0;
    while (fgets(line_buffer, sizeof(line_buffer), f_stream) != NULL)
    {
        if (num_read_lines == MAX_TRACE_SIZE)
        {   
            sprintf(output_error_msg, "file \"%s\" is too big (max is %d lines)", 
            trace_file_name, MAX_TRACE_SIZE);
            fclose(f_stream);
            return 1;
        }

        sscanf(line_buffer, "%d;%d;%d;%d;%d;",
        &(aux.vehicle_id), &(aux.time), &(aux.grid_x_pos), &(aux.grid_y_pos), &(aux.r));
     
        if (aux.vehicle_id != last_seen_vehicle_id)
        {
            num_seen_vehicles++;
            last_seen_vehicle_id = aux.vehicle_id;
        }

        if (num_seen_vehicles > MAX_NUMBER_OF_VEHICLES)
        {   
            sprintf(output_error_msg, 
            "file \"%s\" contains a vehicle with id equal or bigger than %d in line %d (limit is %d vehicles)",
            trace_file_name, MAX_NUMBER_OF_VEHICLES - 1, num_read_lines + 1, MAX_NUMBER_OF_VEHICLES);
            fclose(f_stream);
            return 1;
        }

        if (!(
            ((0 <= aux.grid_x_pos) && (aux.grid_x_pos < MAX_CELL_GRID_WIDTH)) && 
            ((0 <= aux.grid_y_pos) && (aux.grid_y_pos < MAX_CELL_GRID_HEIGHT))
            ))
        {
            sprintf(output_error_msg, 
            "file \"%s\" contains a cell with coordinates out of bounds in line %d." 
            " Allowed: (0 <= x < %d) (0 <= y < %d)", 
            trace_file_name, num_read_lines + 1, MAX_CELL_GRID_WIDTH, MAX_CELL_GRID_HEIGHT);
            fclose(f_stream);
            return 1;
        }

        output_trace[num_read_lines] = aux;
        num_read_lines++;
    }

    if (num_read_lines == 0)
    {
        sprintf(output_error_msg, "file \"%s\" is empty", trace_file_name);
        fclose(f_stream);
        return 1;
    }

    *output_trace_size = num_read_lines;

    fclose(f_stream);
    return 0;
}

void n_deployment(trace_line *trace, int trace_size, struct_n_deployment_input n_deployment_input, struct_n_deployment_output* output_n_deployment_output, FILE* coverage_log_file)
{
    int coverage_best_solution = 0;

    // Used to check itermediate solutions coverage (check_coverage());
    int num_of_contacts[MAX_NUMBER_OF_VEHICLES];

    // Used to build the intermediate solutions and then check their objective function values;
	int cells[ MAX_CELL_GRID_WIDTH ][ MAX_CELL_GRID_HEIGHT ];

    clock_t begin_timer = clock();
    int iteration_index = 0;
    srand(n_deployment_input.grasp_rng_seed);
    for (iteration_index = 0; iteration_index < n_deployment_input.n_deploy_num_ite; iteration_index++)
    {

        pos_2d solution[MAX_NUMBER_OF_RSUS];
		int solution_size = 0;

		while (solution_size < n_deployment_input.number_of_rsus)
		{
		    // fill score for each urban cell
            memcpy(&cells, &(n_deployment_input.cells), sizeof cells);
	    
		    // if cell has rsu, score goes to 0
		    reset_score_of_cells_having_rsu(cells, solution, solution_size);

		    // lets get the "rcl-len" best cells
            pos_2d* rcl = (pos_2d*) malloc(n_deployment_input.grasp_rcl_len * sizeof(pos_2d));
            int i = 0;
		    for (; i < n_deployment_input.grasp_rcl_len; i++)
			{
				get_cell_with_highest_score(cells, &rcl[i].x, &rcl[i].y);
			} 

		    // pick random number between 0..rcl_len
		    int selected = rand() % n_deployment_input.grasp_rcl_len;
		    
		    // add selected rsu to the solution
		    solution[solution_size] = rcl[selected];
		    solution_size++;

            free(rcl);
		}

        int coverage = check_coverage(trace, trace_size, solution, solution_size, 
        num_of_contacts, cells, 
        n_deployment_input.contacts_time_threshold, n_deployment_input.number_of_contacts);
		
        fprintf(coverage_log_file, "%d", coverage);
        if (iteration_index != n_deployment_input.n_deploy_num_ite - 1)
            fprintf(coverage_log_file, ";");

        // If current coverage is better, update best solution;
		if (coverage > coverage_best_solution)
		{
			coverage_best_solution = coverage;
            int i = 0;
            for (; i < solution_size; i++)
            {
                output_n_deployment_output->best_solution[i] = solution[i];
            }
		}
	}

    clock_t end_timer = clock();
    double execution_time_in_secs = (double) (end_timer - begin_timer) / CLOCKS_PER_SEC;

    output_n_deployment_output->mean_ite_execution_time = 
    execution_time_in_secs / n_deployment_input.n_deploy_num_ite;
    
    output_n_deployment_output->ite_execution_time = execution_time_in_secs;

    output_n_deployment_output->best_solution_obj_f_value = coverage_best_solution;
}

void fill_scores_in_cells(trace_line *trace, int tam, 
int cells[ MAX_CELL_GRID_WIDTH ][ MAX_CELL_GRID_HEIGHT ], int time_interval, int number_of_contacts)
{
    reset_cells(cells);
    int current_time = 0;
    int current_vehicle = -1;

	int i;
    for (i = 0; i < tam; i++)
    {
        if (current_vehicle == trace[i].vehicle_id)
        {
            // if the vehicle reaches the rsu within the given time threshold
            if (current_time <= time_interval)
            {
                // viable location
                {
	                cells[ trace[i].grid_x_pos ][ trace[i].grid_y_pos ]+=1;
                    // if (cells[ trace[i].x ][ trace[i].y ] >= number_of_contacts)
	                //    current_vehicle_is_covered = 1;
				}
            }
        }
        else
        {
            current_time = 0;
            current_vehicle = trace[i].vehicle_id;
            cells[ trace[i].grid_x_pos ][ trace[i].grid_y_pos ]+=1;
        }
        current_time = current_time + 10 * trace[i].r;
    }
}

void reset_cells(int cells[ MAX_CELL_GRID_WIDTH ][ MAX_CELL_GRID_HEIGHT ])
{
	int i, j;
    for (i = 0; i< MAX_CELL_GRID_WIDTH; i++)
        for (j = 0; j < MAX_CELL_GRID_HEIGHT; j++)
            cells[i][j]=0;
}

void reset_score_of_cells_having_rsu(int cells[ MAX_CELL_GRID_WIDTH ][ MAX_CELL_GRID_HEIGHT ], 
pos_2d *solution, int solution_size)
{
	int i;
	for (i = 0; i < solution_size; i++)
	{
		cells[ solution[i].x ][ solution[i].y ] = 0;
	}
}

void get_cell_with_highest_score(int cells[ MAX_CELL_GRID_WIDTH ][ MAX_CELL_GRID_HEIGHT ], int *_x, int *_y)
{
	int max_x = 0;
	int max_y = 0;
	int i, j;
	for (i = 0; i < MAX_CELL_GRID_WIDTH; i++)
	{
		for (j = 0; j < MAX_CELL_GRID_HEIGHT; j++)
		{
			if (cells[i][j] > cells[max_x][max_y])
			{
				max_x = i;
				max_y = j;
			}
		}
	}
	
	cells[max_x][max_y] = 0;
	*_x = max_x;
	*_y = max_y;
}

int check_coverage(trace_line *trace, int trace_size, pos_2d *solution, int solution_size, 
int *num_of_contacts, int cells[MAX_CELL_GRID_WIDTH][MAX_CELL_GRID_HEIGHT],
int time_interval, int number_of_contacts)
{
    reset_vehicles (num_of_contacts, MAX_NUMBER_OF_VEHICLES);
    reset_cells(cells);

    int i;
    for (i = 0; i < solution_size; i++) cells[ solution[i].x ][ solution[i].y ] = 1;
    
	int timer = 0;
	int current_vehicle = -1;
	for (i = 0; i < trace_size; i++)
	{
		if (current_vehicle == trace[i].vehicle_id)
		{
			// check if it is inside rsu
			if ( cells[ trace[i].grid_x_pos ][ trace[i].grid_y_pos ] == 1 
            && (trace[i].time < (timer + time_interval)) ) 
			{
				num_of_contacts[ trace[i].vehicle_id ] += 1;
			}
		}
		else
		{
			current_vehicle = trace[i].vehicle_id;
			timer = trace[i].time;
			timer = timer + trace[i].r*10;
			i--;
		}
	}
	
	int covered = 0;
	for (i = 0; i < MAX_NUMBER_OF_VEHICLES; i++)
		if (num_of_contacts[i] >= number_of_contacts) covered++;
	
	return covered;
}

void reset_vehicles(int *vehicles, int tam)
{
	int i;
	for (i=0; i<tam; i++) vehicles[i]=0;
}

int write_summary_to_file(
    struct_n_deployment_input n_deployment_input, 
    struct_n_deployment_output n_deployment_output, 
    char* output_error_msg)
{
    char output_file_name[MAX_INPUT_FILE_PATH_SIZE + 250];
    sprintf(output_file_name, "rsu=%d_tau=%d_rcl=%d_iter=%d_cont=%d_summary.txt", 
    n_deployment_input.number_of_rsus, n_deployment_input.contacts_time_threshold, 
    n_deployment_input.grasp_rcl_len, n_deployment_input.n_deploy_num_ite, n_deployment_input.number_of_contacts);

    FILE *output_file = fopen(output_file_name, "w");
    if (output_file == NULL)
    {
        sprintf(output_error_msg, "can't write summary file");
        return 1;
    }

    fprintf(output_file, "==================== N-DEPLOYMENT RESULTS ====================\n\n");
    
    fprintf(output_file, "-------------------- INPUT -----------------------------------\n\n");
    fprintf(output_file, "N. RSUS: %d\n", n_deployment_input.number_of_rsus);
    fprintf(output_file, "N. CONTACTS: %d\n", n_deployment_input.number_of_contacts);
    fprintf(output_file, "CONTACTS TIME INTERVAL: %d\n", n_deployment_input.contacts_time_threshold);

    fprintf(output_file, "GRASP RNG SEED: %d\n", n_deployment_input.grasp_rng_seed);
    fprintf(output_file, "N-DEPLOY N. ITERATIONS: %d\n", n_deployment_input.n_deploy_num_ite);
    fprintf(output_file, "GRASP RCL LENGTH: %d\n", n_deployment_input.grasp_rcl_len);

    fprintf(output_file, "N-DEPLOY AVERAGE ITERATION EXECUTION TIME: %.06f\n", 
    n_deployment_output.mean_ite_execution_time);
    fprintf(output_file, "N-DEPLOY TOTAL EXECUTION TIME (ITERATIONS): %.06f\n\n", 
    n_deployment_output.ite_execution_time);

    fprintf(output_file, "-------------------- BEST SOLUTION ---------------------------\n\n");

    fprintf(output_file, "OBJECTIVE FUNCTION VALUE (N. COVERED VEHICLES): %d\n\n", 
    n_deployment_output.best_solution_obj_f_value);

    fclose(output_file);

    return 0;
}

int write_rsus_to_file(
    struct_n_deployment_input n_deployment_input, 
    pos_2d* rsus_pos, int num_rsus,
    char *output_error_msg
)
{
    char output_file_name[MAX_INPUT_FILE_PATH_SIZE + 250];
    sprintf(output_file_name, "rsu=%d_tau=%d_rcl=%d_iter=%d_cont=%d_rsus.txt", 
    n_deployment_input.number_of_rsus, n_deployment_input.contacts_time_threshold, 
    n_deployment_input.grasp_rcl_len, n_deployment_input.n_deploy_num_ite, n_deployment_input.number_of_contacts);

    FILE* rsus_file = fopen(output_file_name, "w");
    if (rsus_file == NULL)
    {
        sprintf(output_error_msg, "can't write rsus file");
        return 1;
    }

    int i;
    for (i = 0; i < num_rsus; i++)
    {
        pos_2d pos = rsus_pos[i];
        fprintf(rsus_file, "%d,%d", pos.x, pos.y);
        if (i != num_rsus - 1)
            fprintf(rsus_file, ";");
    }

    fclose(rsus_file);

    return 0;
}
