from abc import ABC, abstractmethod
from pathlib import PurePath

class deployment_param(ABC):

    def __init__(self, executable_path: PurePath, trace_path: PurePath):
        super().__init__()
        self.trace_path = trace_path
        self.executable_path = executable_path
    
    @abstractmethod
    def to_cmdline_args(self):
        ...