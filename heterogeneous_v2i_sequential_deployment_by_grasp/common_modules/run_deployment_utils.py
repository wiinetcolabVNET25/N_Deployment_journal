## FUNCTIONS TO HELP RUN DEPLOYMENT METHODS AND GET INFRASTRUCTURE;
import subprocess
from pathlib import PurePath
import pandas as pd

# trace must have column "vehicle_id" (integer);
def get_trace_with_no_vehicles_in_list_indexes(trace_df: pd.DataFrame, vehicles: list[int]):

    rows_with_vehicles_indexes = trace_df[~trace_df["vehicle_id"].isin(vehicles)].index
    return rows_with_vehicles_indexes

def get_trace_with_vehicles_in_list_indexes(trace_df: pd.DataFrame, vehicles: list[int]):

    rows_with_vehicles_indexes = trace_df[trace_df["vehicle_id"].isin(vehicles)].index
    return rows_with_vehicles_indexes

# ATTENTION: assuming cell_time was not scaled;
def write_trace_file_with_ids_mapped_to_first_naturals(trace_df: pd.DataFrame, file_path: PurePath):
    
    trace_as_list = list(trace_df.itertuples(index=False, name=None))

    new_vehicle_id = 0
    ids_mapping_to_0_n_minus_1 = {}

    with open(file_path, mode="w") as f:
        for vehicle_id, time_instant, x, y, cell_time in trace_as_list:

            if not (vehicle_id in ids_mapping_to_0_n_minus_1):
                ids_mapping_to_0_n_minus_1[vehicle_id] = new_vehicle_id
                new_vehicle_id += 1

            f.write(f"{ids_mapping_to_0_n_minus_1[vehicle_id]};{time_instant};{x};{y};{cell_time};\n")

# If return code != 0 raises ValueError;
def run_program_and_wait(file_path: PurePath, cmdline_args: list[str]):
    result = subprocess.run(args=[file_path] + cmdline_args, capture_output=True)
    if result.returncode != 0:
        raise ValueError(result.stderr)

def read_infrastructure_csv(file_path: PurePath) -> list:
    
    with open(file_path, "r") as infra_f:

        rsus = set()
        for line in infra_f.readlines():

            split = line.split(",")
            rsu_coord = (int(split[0]), int(split[1]))

            rsus.add(rsu_coord)

        return list(rsus)