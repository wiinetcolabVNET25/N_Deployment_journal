## FUNCTIONS TO EVALUATE INFRASTRUCTURE BASED ON METRICS (MAY INCLUDE COVERAGE);
## CHOOSE METRIC -> EVALUATE

import numpy as np
import pandas as pd

MAX_GRID_WIDTH = 100
MAX_GRID_HEIGHT = 100

def get_vehicles_covered_based_on_gamma_metric(
    rsus_pos: list | set, 
    vehicles_trace_df: pd.DataFrame, 
    inter_contact_time_threshold: int
) -> int:

    covered_vehicles = []

    for vehicle_id in vehicles_trace_df["vehicle_id"].unique():

        vehicle_trace = list(
            vehicles_trace_df[vehicles_trace_df["vehicle_id"] == vehicle_id].itertuples(index=False, name=None)
        )

        is_covered = check_if_vehicle_is_covered_based_on_gamma_metric(
            rsus_pos=rsus_pos,
            vehicle_trace=vehicle_trace,
            tau=inter_contact_time_threshold
        )

        if (is_covered):
            covered_vehicles.append(vehicle_id)

    return covered_vehicles

def check_if_vehicle_is_covered_based_on_gamma_metric(
    rsus_pos: list | set, vehicle_trace: list[tuple], tau: int
) -> bool:


    time_without_rsus = 0

    # Check if vehicle is covered looking at its entire trip;
    # If some subpath is not covered, return early;
    for t_line in vehicle_trace:

        grid_x_pos = t_line[2]
        grid_y_pos = t_line[3]
        v_time_in_cell = t_line[4]

        if (grid_x_pos, grid_y_pos) in rsus_pos:
            time_without_rsus = 0
        else:
            time_without_rsus += v_time_in_cell

        if (time_without_rsus * 10 > tau):
            return False
        
    return True

def get_vehicles_covered_based_on_n_deployment(
    rsus_pos: list | set, 
    vehicles_trace_df: pd.DataFrame, 
    n_contacts: int, contacts_time_threshold: int
) -> list:

    timer = 0
    current_vehicle = -1
    i = 0
    vehicles_num_contacts = {vehicle_id : 0 for vehicle_id in vehicles_trace_df["vehicle_id"].unique()}
    trace_as_list = list(vehicles_trace_df.itertuples(index=False, name=None))
    while i < len(trace_as_list):

        vehicle_id = trace_as_list[i][0]
        time_instant = trace_as_list[i][1]
        grid_x_pos = trace_as_list[i][2]
        grid_y_pos = trace_as_list[i][3]
        time_in_cell = trace_as_list[i][4]
        if current_vehicle == vehicle_id:

            if (((grid_x_pos, grid_y_pos) in rsus_pos)
                and (time_instant < (timer + contacts_time_threshold))):
                vehicles_num_contacts[vehicle_id] += 1

        else:

            current_vehicle = vehicle_id
            timer = time_instant
            timer = timer + time_in_cell * 10
            i -= 1

        i += 1

    covered_vehicles = []
    for vehicle_id, contacts in vehicles_num_contacts.items():
        if contacts >= n_contacts:
            covered_vehicles.append(vehicle_id)

    return covered_vehicles

from abc import ABC, abstractmethod
import pandas as pd

class metric(ABC):
    
    def __init__(self):
        self.last_eval = None

    @abstractmethod
    def name(self) -> str:
        ...

    @abstractmethod
    def get_last_eval(self) -> dict:
        ...

    @abstractmethod
    def get_last_eval_str(self) -> str:
        ...

    # Evaluates infra with metrics defined in subclass constructor
    # and saves result in variable last_eval;
    # The results can contain anything, but must contain a list of the covered vehicles in the key "covered_vehicles";
    @abstractmethod
    def evaluate_infra_and_save_result(
        self, trace_df: pd.DataFrame, infrastructure: list[tuple]):
        ...

class n_contacts_metric(metric):

    def __init__(self, N: int, tau: int):
        super().__init__()
        self.N = N
        self.tau = tau

    def name(self) -> str:
        return f"N. CONTACTS METRIC - N: {self.N} | TAU: {self.tau}"

    def get_last_eval(self) -> dict:
        return self.last_eval

    def get_last_eval_str(self) -> str:
        str_repr = ""
        if not (self.last_eval is None):
            num_cov_v = self.last_eval["num_covered_vehicles"]
            efficiency = self.last_eval["efficiency"]
            str_repr += f"[+] num. covered vehicles: {num_cov_v}\n"
            str_repr += f"[+] efficiency: {efficiency}"

        return str_repr

    def evaluate_infra_and_save_result(self, trace_df: pd.DataFrame, infrastructure: list[tuple]) -> None:

        evaluation = {}

        if len(infrastructure) == 0:
            
            evaluation["covered_vehicles"] = []
            evaluation["num_covered_vehicles"] = 0
            evaluation["efficiency"] = 0

            self.last_eval = evaluation

            return

        evaluation["covered_vehicles"] = get_vehicles_covered_based_on_n_deployment(
            rsus_pos=set(infrastructure),
            vehicles_trace_df=trace_df,
            n_contacts=self.N,
            contacts_time_threshold=self.tau,
        )
        evaluation["num_covered_vehicles"] = len(evaluation["covered_vehicles"])
        evaluation["efficiency"] = len(evaluation["covered_vehicles"]) / len(infrastructure)

        self.last_eval = evaluation

class gamma_metric(metric):

    def __init__(self, tau: int):
        super().__init__()
        self.tau = tau
        self.last_eval = None

    def name(self) -> str:
        return f"GAMMA METRIC - TAU: {self.tau}"

    def get_last_eval(self) -> dict:
        return self.last_eval

    def get_last_eval_str(self) -> str:
        str_repr = ""
        if not (self.last_eval is None):
            num_cov_v = self.last_eval["num_covered_vehicles"]
            efficiency = self.last_eval["efficiency"]
            str_repr += f"[+] num. covered vehicles: {num_cov_v}\n"
            str_repr += f"[+] efficiency: {efficiency}"

        return str_repr

    def evaluate_infra_and_save_result(self, trace_df: pd.DataFrame, infrastructure: list[tuple]) -> None:

        evaluation = {}

        if len(infrastructure) == 0:

            evaluation["covered_vehicles"] = []
            evaluation["num_covered_vehicles"] = 0
            evaluation["efficiency"] = 0

            self.last_eval = evaluation

            return

        evaluation["covered_vehicles"] = get_vehicles_covered_based_on_gamma_metric(
            rsus_pos=set(infrastructure),
            vehicles_trace_df=trace_df,
            inter_contact_time_threshold=self.tau,
        )
        evaluation["num_covered_vehicles"] = len(evaluation["covered_vehicles"])
        evaluation["efficiency"] = len(evaluation["covered_vehicles"]) / len(infrastructure)

        self.last_eval = evaluation

# Changes the metric object, updating the last evaluation;
def evaluate_infrastructure_with_metrics(
    metrics: list[metric], trace_df: pd.DataFrame, infrastructure: list[tuple]) -> dict[str, dict]:

    for metric in metrics:
        metric.evaluate_infra_and_save_result(trace_df=trace_df, infrastructure=infrastructure)

class overall_infra_evaluation_param:
    
    def __init__(self, metrics: list[metric]):
        self.metrics = metrics

class overall_infra_evaluation_results:

    def __init__(self, metrics: list[metric]):
        self.metrics = metrics

    def print_results(self) -> None:

        for metric in self.metrics:
            print(metric.name())
            print(metric.get_last_eval_str())

    def __str__(self):

        str_repr = ""
        for metric in self.metrics:
            str_repr += (metric.name() + "\n")
            str_repr += (metric.get_last_eval_str() + "\n")
        return str_repr