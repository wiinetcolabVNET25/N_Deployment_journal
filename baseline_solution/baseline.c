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
#define MAX_NUMBER_OF_RSUS          1500

#define MAX_INPUT_FILE_PATH_SIZE    100

typedef struct trace_ 
{
    int vehicle_id;
    int time;
    int grid_x_pos;
    int grid_y_pos;
    int r;

} struct_trace;

typedef struct solution_
{
    int x;
    int y;

} struct_solution;

typedef struct baseline_input_ 
{
    int number_of_most_frequent_cells_to_pick;

} struct_baseline_input;

typedef struct baseline_output_
{
    double total_execution_time;
    struct_solution solution[MAX_NUMBER_OF_RSUS];
    int solution_size;

} struct_baseline_output;

// ==================== INPUT FUNCTIONS ==================== //
// If succeedes, returns 0 and output_baseline_input can be used;
// Otherwise, returns 1 and error_msg can be used;
int read_commandline_args(int argc, char **argv, struct_baseline_input *output_baseline_input, 
char* output_input_file_path, char* output_error_msg);
int try_to_get_positive_int_from_arg(char* arg, char* var_name, int* output, 
char* output_error_msg);

// If succeedes, returns 0 and output_trace and output_trace_size can be used;
// Otherwise, returns 1 and error_msg can be used;
int read_trace(const char* trace_file_name, struct_trace *output_trace, int *output_trace_size, 
char* output_error_msg);
// ==================== INPUT FUNCTIONS ==================== //


// ==================== BASELINE FUNTCIONS ================= //
void baseline(struct_trace* trace, int trace_size, struct_baseline_input baseline_input,
struct_baseline_output* output_baseline_output);
int get_num_frequented_cells(int matrix[MAX_CELL_GRID_WIDTH][MAX_CELL_GRID_HEIGHT]);
void get_max_cell_coords_and_zero_it(int matrix[MAX_CELL_GRID_WIDTH][MAX_CELL_GRID_HEIGHT], 
int *output_x_coord, int* output_y_coord);
// ==================== BASELINE FUNCTIONS ================= //


// ==================== OUTPUT FUNCTIONS ==================== //
int write_summary_to_file(struct_baseline_input baseline_input, 
struct_baseline_output baseline_output, char* output_error_msg);
int write_rsus_to_file(struct_baseline_input baseline_input, 
struct_baseline_output baseline_output, char* output_error_msg);
// ==================== OUTPUT FUNCTIONS ==================== //


int main(int argc, char** argv)
{
    // In case any specified error below occurs;
    int status;
    char error_msg[300];
    error_msg[0] = '\0';

    // ==================== 1 - READ INPUT ==================== //

    const char* correct_input_format = "USAGE: baseline <num of most frequent cells> <trace file path>";

    // -------------------- 1.1 COMMAND LINE ------------------ //

    char input_file_path[MAX_INPUT_FILE_PATH_SIZE + 1];
    input_file_path[0] = '\0';
    struct_baseline_input baseline_input;
    status = read_commandline_args(argc, argv, &baseline_input, input_file_path, error_msg);
    if (status != 0)
    {
        printf("BASELINE: COMMAND LINE ERROR: %s\n\n%s\n", 
        error_msg, correct_input_format);
        return 1;
    }

    // -------------------- 1.2 TRACE FILE -------------------- //

    struct_trace *trace = (struct_trace *) malloc(sizeof(struct_trace) * MAX_TRACE_SIZE);
    if (!trace)
    {
        printf("BASELINE: MEMORY ERROR: can't allocate memory"
        " for %d trace lines (MAX_TRACE_SIZE)\n", MAX_TRACE_SIZE);
        return 1;
    }

    int trace_size;
    status = read_trace(input_file_path, trace, &trace_size, error_msg);
    if (status != 0)
    {
        printf("BASELINE: INPUT TRACE FILE ERROR: %s\n", error_msg);
        free(trace);
        return 1;
    }

    // ==================== 2 - RUN =========================== //

    struct_baseline_output baseline_output;
    baseline(trace, trace_size, baseline_input, &baseline_output);

    // ==================== 3 - WRITE RESULTS ================= //

    status = write_summary_to_file(baseline_input, baseline_output, error_msg);
    if (status != 0)
    {
        printf("BASELINE: OUTPUT SUMMARY FILE ERROR: %s\n", error_msg);
        free(trace);
        return 1;
    }

    status = write_rsus_to_file(baseline_input, baseline_output, error_msg);
    if (status != 0)
    {
        printf("BASELINE: OUTPUT RSUS FILE ERROR: %s\n", error_msg);
        free(trace);
        return 1;
    }

    // ==================== 4 - FREE REMAINING RESOURCES ====== //
    free(trace);
    
    return 0;
}

int read_commandline_args(int argc, char **argv, struct_baseline_input *output_n_deployment_input, 
char* output_input_file_path, char* output_error_msg)
{
    if (argc != 3)
    {
        strcpy(output_error_msg, "wrong number of arguments");
        return 1;
    }

    // ========== Try to convert all arguments to correct numeric values; ========== //
    int status;

    status = try_to_get_positive_int_from_arg(argv[1], "<num of most frequent cells>", 
    &(output_n_deployment_input->number_of_most_frequent_cells_to_pick), output_error_msg);
    if (status != 0)
    {
        return 1;
    }
    if (output_n_deployment_input->number_of_most_frequent_cells_to_pick > MAX_NUMBER_OF_RSUS)
    {        
        sprintf(output_error_msg, "number of most frequent cells must be at most %d", MAX_NUMBER_OF_RSUS);
        return 1;
    }
    
    // ========== try to get input file path ========== //
    if (strlen(argv[2]) > MAX_INPUT_FILE_PATH_SIZE)
    {
        sprintf(output_error_msg, "file path is bigger than %d bytes", MAX_INPUT_FILE_PATH_SIZE);
        return 1;
    }
    else {

        strcpy(output_input_file_path, argv[2]);
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
int read_trace(const char* trace_file_name, struct_trace* output_trace, int *output_trace_size, 
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
    struct_trace aux;
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

void baseline(struct_trace* trace, int trace_size, struct_baseline_input baseline_input,
struct_baseline_output* output_baseline_output)
{
    int number_of_most_frequent_cells_to_pick = baseline_input.number_of_most_frequent_cells_to_pick;

    int cell_frequency[MAX_CELL_GRID_WIDTH][MAX_CELL_GRID_HEIGHT] = {0};

    clock_t begin_timer = clock();

    int i;
    for (i = 0; i < trace_size; i++)
    {
        cell_frequency[trace[i].grid_x_pos][trace[i].grid_y_pos] += 1;
    }

    int solution_size = 0;
    int number_of_frequented_cells = get_num_frequented_cells(cell_frequency);
    int solution_size_to_achieve = 
    (number_of_most_frequent_cells_to_pick > number_of_frequented_cells) ? 
    number_of_frequented_cells : number_of_most_frequent_cells_to_pick;

    while (solution_size < solution_size_to_achieve)
    {
        struct_solution chosen_position;
        get_max_cell_coords_and_zero_it(cell_frequency, &(chosen_position.x), &(chosen_position.y));

        output_baseline_output->solution[solution_size] = chosen_position;
        solution_size += 1;
    }

    clock_t end_timer = clock();
    double execution_time_in_secs = (double) (end_timer - begin_timer) / CLOCKS_PER_SEC;

    output_baseline_output->total_execution_time = execution_time_in_secs;
    output_baseline_output->solution_size = solution_size;
}

int get_num_frequented_cells(int matrix[MAX_CELL_GRID_WIDTH][MAX_CELL_GRID_HEIGHT])
{
    int num_frequented_cells = 0;
    int i, j;
    for (i = 0; i < MAX_CELL_GRID_WIDTH; i++)
    {
        for (j = 0; j < MAX_CELL_GRID_HEIGHT; j++)
        {
            if (matrix[i][j] != 0)
            {
                num_frequented_cells += 1;
            }
        }
    }

    return num_frequented_cells;
}

void get_max_cell_coords_and_zero_it(int matrix[MAX_CELL_GRID_WIDTH][MAX_CELL_GRID_HEIGHT], 
int *output_x_coord, int* output_y_coord)
{
    int max_x = 0;
	int max_y = 0;
	int i, j;
	for (i = 0; i < MAX_CELL_GRID_WIDTH; i++)
	{
		for (j = 0; j < MAX_CELL_GRID_HEIGHT; j++)
		{
			if (matrix[i][j] > matrix[max_x][max_y])
			{
				max_x = i;
				max_y = j;
			}
		}
	}
	
	matrix[max_x][max_y] = 0;
	*output_x_coord = max_x;
	*output_y_coord = max_y;
}

int write_summary_to_file(struct_baseline_input baseline_input, 
struct_baseline_output baseline_output, char* output_error_msg)
{
    char output_file_name[MAX_INPUT_FILE_PATH_SIZE + 100];
    sprintf(output_file_name, "n=%d_summary.txt", 
    baseline_input.number_of_most_frequent_cells_to_pick);

    FILE *output_file = fopen(output_file_name, "w");
    if (output_file == NULL)
    {
        sprintf(output_error_msg, "can't open file \"%s\"", output_file_name);
        return 1;
    }

    fprintf(output_file, "==================== BASELINE RESULTS ====================\n\n");
    
    fprintf(output_file, "N. RSUS TO PICK (INPUT): %d\n", baseline_input.number_of_most_frequent_cells_to_pick);
    fprintf(output_file, "N. RSUS (ACTUAL): %d\n", baseline_output.solution_size);
    fprintf(output_file, "N-DEPLOY TOTAL EXECUTION TIME: %.06f\n\n", baseline_output.total_execution_time);

    fclose(output_file);

    return 0;
}

int write_rsus_to_file(struct_baseline_input baseline_input, 
struct_baseline_output baseline_output, char* output_error_msg)
{
    char output_file_name[MAX_INPUT_FILE_PATH_SIZE + 100];
    sprintf(output_file_name, "n=%d_rsus.txt", 
    baseline_input.number_of_most_frequent_cells_to_pick);

    FILE *output_file = fopen(output_file_name, "w");
    if (output_file == NULL)
    {
        sprintf(output_error_msg, "can't open file \"%s\"", output_file_name);
        return 1;
    }

    int i;
    for (i = 0; i < baseline_output.solution_size; i++)
    {
        struct_solution pos = baseline_output.solution[i];
        fprintf(output_file, "%d,%d", pos.x, pos.y);
        if (i != baseline_output.solution_size - 1)
            fprintf(output_file, ";");
    }

    fclose(output_file);

    return 0;
}
