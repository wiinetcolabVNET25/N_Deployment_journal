from . import abs_deployment_param

from pathlib import Path, PurePath
import pandas as pd

class n_deployment_param(abs_deployment_param.deployment_param):

    def __init__(self, executable_path: PurePath,
                 num_rsus: int, tau: int, rcl_len: int, 
                 n_iter: int, num_cont: int, seed: int, 
                 trace_path: PurePath = None):
        super().__init__(executable_path=executable_path,
                         trace_path=trace_path)
        self.num_rsus = num_rsus
        self.tau = tau
        self.rcl_len = rcl_len
        self.n_iter = n_iter
        self.num_cont = num_cont
        self.seed = seed

    def to_cmdline_args(self) -> list[str]:
        return [str(self.num_rsus), str(self.tau), str(self.rcl_len), 
                str(self.n_iter), str(self.num_cont), str(self.seed), self.trace_path]

    def __str__(self):
        return (f"-num_rsus: {self.num_rsus} -tau: {self.tau} -rcl_len: {self.rcl_len} -n_iter: {self.n_iter} " 
                + f"-num_cont: {self.num_cont} -seed: {self.seed} -trace_path: {self.trace_path}")

def get_infrastructure_path(n_deployment_p: n_deployment_param):

    return PurePath(".") / (f"rsu={n_deployment_p.num_rsus}_tau={n_deployment_p.tau}" 
                            + f"_rcl={n_deployment_p.rcl_len}_iter={n_deployment_p.n_iter}" 
                            + f"_cont={n_deployment_p.num_cont}_rsus.csv")

def get_best_coverage_log_path(n_deployment_p: n_deployment_param):

    return PurePath(".") / (f"rsu={n_deployment_p.num_rsus}_tau={n_deployment_p.tau}" 
                            + f"_rcl={n_deployment_p.rcl_len}_iter={n_deployment_p.n_iter}" 
                            + f"_cont={n_deployment_p.num_cont}_best_coverage_log.csv")

def get_summary_path(n_deployment_p: n_deployment_param):

    return PurePath(".") / (f"rsu={n_deployment_p.num_rsus}_tau={n_deployment_p.tau}" 
                            + f"_rcl={n_deployment_p.rcl_len}_iter={n_deployment_p.n_iter}" 
                            + f"_cont={n_deployment_p.num_cont}_summary.txt")

# If files are not present, does not do anything;
def delete_n_deployment_output_files(n_deployment_p: n_deployment_param):

    pattern = (f"rsu={n_deployment_p.num_rsus}_tau={n_deployment_p.tau}" 
                 + f"_rcl={n_deployment_p.rcl_len}_iter={n_deployment_p.n_iter}_cont={n_deployment_p.num_cont}*")
    output_files = list(Path(".").glob(pattern=pattern))
    for file in output_files:
        file.unlink()