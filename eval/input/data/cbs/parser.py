#! /usr/bin/env python
import re
from lab.parser import Parser


def solved(content, props):
    props["solved"] = int("sum-of-costs" in props)

def unsolvable(content, props):
    matches = re.findall(r"Planning NOT successful!", content)
    props["unsolvable"] = bool(matches)

if __name__ == "__main__":
    parser = Parser()
    parser.add_pattern("node", r"node: (.+)\n", type=str, file="driver.log", required=True)
    parser.add_pattern("solver_exit_code", r"solver exit code: (.+)\n", type=int, file="driver.log")

    parser.add_pattern("sum-of-costs", r"sum-of-costs: (\d+)\n", type=int, file="plan.yaml")
    parser.add_pattern("makespan", r"makespan: (\d+)\n", type=int, file="plan.yaml")
    parser.add_pattern("t_total", r"t_total: (.+)\n", type=float, file="plan.yaml")
    parser.add_pattern("t_solver", r"t_solver: (.+)\n", type=float, file="plan.yaml")
    parser.add_pattern("high_level_expansion", r"highLevelExpanded: (\d+)\n", type=int, file="plan.yaml")
    parser.add_pattern("low_level_expansion", r"lowLevelExpanded: (\d+)\n", type=int, file="plan.yaml")
    
    parser.add_function(solved)
    parser.add_function(unsolvable)
    parser.parse()
