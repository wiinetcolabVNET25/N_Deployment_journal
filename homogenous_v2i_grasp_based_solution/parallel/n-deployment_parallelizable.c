#include "n-deployment_parallelizable.h"
#include <stdlib.h>
#include <unistd.h>

void fill_scores_in_cells(
    trace_line *trace,
    int tam, int **cells,
    int time_interval, int number_of_contacts);

void reset_cells(int **cells);

void reset_score_of_cells_having_rsu(
    int **cells,
    pos_2d *solution, int solution_size);

void get_cell_with_highest_score(
    int **cells, int *_x, int *_y);

int check_coverage(
    trace_line *trace, int trace_size,
    pos_2d *solution, int solution_size,
    int *num_of_contacts, int **cells,
    int time_interval, int number_of_contacts);

void reset_vehicles(int *vehicles, int tam);

// IMPORTANT:
// Assumes, for each vehicle, its trace is composed by adjacent lines,
// int is big enough for every value, no line is bigger than 200 bytes,
// and fields are integers separated only by ';' and line ends with ';';
int read_trace(const char *trace_file_name, trace_line *output_trace, int *output_trace_size,
               char *output_error_msg)
{
    int num_seen_vehicles = 0;
    int last_seen_vehicle_id = -1;

    FILE *f_stream = fopen(trace_file_name, "r");

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
                ((0 <= aux.grid_y_pos) && (aux.grid_y_pos < MAX_CELL_GRID_HEIGHT))))
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

n_deployment_output_t n_deployment(n_deployment_input_t n_deployment_input)
{
    trace_line *trace = n_deployment_input.trace;

    int trace_size = n_deployment_input.trace_size;

    int coverage_best_solution = 0;
    int coverage_worst_solution = MAX_NUMBER_OF_VEHICLES + 1;

    // Used to check itermediate solutions coverage (check_coverage());
    int num_of_contacts[MAX_NUMBER_OF_VEHICLES];

    // Used to build the intermediate solutions and then check their objective function values;
    int **cells;
    cells = malloc(MAX_CELL_GRID_WIDTH * sizeof(int *));
    for (int i = 0; i < MAX_CELL_GRID_WIDTH; i++) {
        cells[i] = malloc(MAX_CELL_GRID_HEIGHT * sizeof(int));
    }

    int iteration_index = 0;
    struct drand48_data rand_buffer;
    srand48_r(n_deployment_input.grasp_rng_seed, &rand_buffer);
    for (iteration_index = 0; iteration_index < n_deployment_input.n_deploy_num_ite; iteration_index++)
    {

        pos_2d solution[MAX_NUMBER_OF_RSUS];
        int solution_size = 0;

        while (solution_size < n_deployment_input.number_of_rsus)
        {
            // fill score for each urban cell
            for (int i = 0; i < MAX_CELL_GRID_WIDTH; i++)
            {
                memcpy(cells[i], n_deployment_input.cells_scores[i], MAX_CELL_GRID_HEIGHT * sizeof(int));
            }

            // if cell has rsu, score goes to 0
            reset_score_of_cells_having_rsu(cells, solution, solution_size);

            // lets get the "rcl-len" best cells
            pos_2d *rcl = (pos_2d *)malloc(n_deployment_input.grasp_rcl_len * sizeof(pos_2d));
            int i = 0;
            for (; i < n_deployment_input.grasp_rcl_len; i++)
            {
                get_cell_with_highest_score(cells, &rcl[i].x, &rcl[i].y);
            }

            // pick random number between 0 and rcl_len
            double random_num;
            drand48_r(&rand_buffer, &random_num);
            int selected_index = (int) (random_num * (n_deployment_input.grasp_rcl_len));

            // add selected rsu to the solution
            solution[solution_size] = rcl[selected_index];
            solution_size++;

            free(rcl);
        }

        int coverage = check_coverage(trace, trace_size, solution, solution_size,
                                      num_of_contacts, cells,
                                      n_deployment_input.contacts_time_threshold, n_deployment_input.number_of_contacts);

        // If current coverage is better, update best solution;
        if (coverage > coverage_best_solution)
        {
            coverage_best_solution = coverage;
        }

        if (coverage < coverage_worst_solution)
        {
            coverage_worst_solution = coverage;
        }
    }

    for (int i = 0; i < MAX_CELL_GRID_WIDTH; i++)
    {
        free(cells[i]);
    }
    free(cells);

    n_deployment_output_t output;
    output.best_coverage = coverage_best_solution;
    output.worst_coverage = coverage_worst_solution;
    return output;
}

void fill_scores_in_cells(trace_line *trace, int tam,
                          int **cells, int time_interval, int number_of_contacts)
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
                    cells[trace[i].grid_x_pos][trace[i].grid_y_pos] += 1;
                }
            }
        }
        else
        {
            current_time = 0;
            current_vehicle = trace[i].vehicle_id;
            cells[trace[i].grid_x_pos][trace[i].grid_y_pos] += 1;
        }
        current_time = current_time + 10 * trace[i].r;
    }
}

void reset_cells(int **cells)
{
    int i, j;
    for (i = 0; i < MAX_CELL_GRID_WIDTH; i++)
        for (j = 0; j < MAX_CELL_GRID_HEIGHT; j++)
            cells[i][j] = 0;
}

void reset_score_of_cells_having_rsu(int **cells,
                                     pos_2d *solution, int solution_size)
{
    int i;
    for (i = 0; i < solution_size; i++)
    {
        cells[solution[i].x][solution[i].y] = 0;
    }
}

void get_cell_with_highest_score(int **cells, int *_x, int *_y)
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
                   int *num_of_contacts, int **cells,
                   int time_interval, int number_of_contacts)
{
    reset_vehicles(num_of_contacts, MAX_NUMBER_OF_VEHICLES);
    reset_cells(cells);

    int i;
    for (i = 0; i < solution_size; i++)
        cells[solution[i].x][solution[i].y] = 1;

    int timer = 0;
    int current_vehicle = -1;
    for (i = 0; i < trace_size; i++)
    {
        if (current_vehicle == trace[i].vehicle_id)
        {
            // check if it is inside rsu
            if (cells[trace[i].grid_x_pos][trace[i].grid_y_pos] == 1 && (trace[i].time < (timer + time_interval)))
            {
                num_of_contacts[trace[i].vehicle_id] += 1;
            }
        }
        else
        {
            current_vehicle = trace[i].vehicle_id;
            timer = trace[i].time;
            timer = timer + trace[i].r * 10;
            i--;
        }
    }

    int covered = 0;
    for (i = 0; i < MAX_NUMBER_OF_VEHICLES; i++)
        if (num_of_contacts[i] >= number_of_contacts)
            covered++;

    return covered;
}

void reset_vehicles(int *vehicles, int tam)
{
    int i;
    for (i = 0; i < tam; i++)
        vehicles[i] = 0;
}