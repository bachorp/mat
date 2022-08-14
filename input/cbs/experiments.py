#! /usr/bin/env python

"""
Example experiment using a simple vertex cover solver.
"""

import glob
import os
import platform

from downward.reports.absolute import AbsoluteReport
from lab.environments import SlurmEnvironment, LocalEnvironment
from lab.experiment import Experiment
from lab.reports import Attribute

REMOTE = True


class FreiburgSlurmEnvironment(SlurmEnvironment):
    """Environment for Freiburg's AI group."""

    def __init__(self, **kwargs):
        SlurmEnvironment.__init__(self, **kwargs)
        # SlurmEnvironment.__init__(self, extra_options='#SBATCH --exclude=kisexe[06]', **kwargs)

    DEFAULT_PARTITION = "gki_cpu-cascadelake"
    #DEFAULT_PARTITION =  "gki_gpu-ti"
    DEFAULT_QOS = "normal"
    DEFAULT_MEMORY_PER_CPU = "6000M"

# Create custom report class with suitable info and error attributes.


class BaseReport(AbsoluteReport):
    INFO_ATTRIBUTES = ["time_limit", "memory_limit"]
    ERROR_ATTRIBUTES = [
        "domain",
        "problem",
        "algorithm",
        "unexplained_errors",
        "node",
    ]


REPO = "/home/bergdolr/mat"
SEEDS = range(0, 20)
GRIDS = range(4, 13)
AGENTS = range(1, 11)
CONTAINERS = range(1, 11)

TIME_LIMIT = 600  # sec
MEMORY_LIMIT = 4096  # MB

if REMOTE:
    ENV = FreiburgSlurmEnvironment()
    SEEDS = list(range(0, 20))
    GRIDS = list(range(4, 13))
    BLOCKS = [10, 20]
    AGENTS = list(range(1, 11))
    CONTAINERS = list(range(1, 11))
else:
    ENV = LocalEnvironment(processes=2)
    # Use smaller suite for local tests.
    SEEDS = [13]
    GRIDS = [4, 7]
    BLOCKS = [10]
    AGENTS = [4, 8]
    CONTAINERS = [2, 4]

ATTRIBUTES = [
    "cost",
    "converter_exit_code",
    "converter_time",
    "high_level_expansion",
    "low_level_expansion",
    "overall_time",
    Attribute("solved", absolute=True, min_wins=False),
    "solver_exit_code",
    "solver_time",
    "makespan",
]

# Create a new experiment.
exp = Experiment(environment=ENV)

exp.add_resource("cbs_ta", os.path.join(REPO, "build", "cbs_ta"))

# Add custom parser.
exp.add_parser("parser.py")

for s in SEEDS:
    for g in GRIDS:
        for b in BLOCKS:
            for a in AGENTS:
                for c in CONTAINERS:
                    if c > a:
                        continue
                    run = exp.add_run()
                    run.add_command(
                        "solver",
                        ["{cbs_ta}", "s", s, "g", g, "b", b, "a", a, "c", c, "o", "plan.yaml"],
                        time_limit=TIME_LIMIT,
                        memory_limit=MEMORY_LIMIT,
                    )

                    # AbsoluteReport needs the following attributes:
                    # 'domain', 'problem' and 'algorithm'.
                    domain = "g{}-b{}".format(g, b)
                    task_name = "a{}-c{}-s{}".format(a, c, s)
                    run.set_property("domain", domain)
                    run.set_property("problem", task_name)
                    run.set_property("algorithm", "cbs_ta")
                    # BaseReport needs the following properties:
                    # 'time_limit', 'memory_limit', 'seed'.
                    run.set_property("time_limit", TIME_LIMIT)
                    run.set_property("memory_limit", MEMORY_LIMIT)
                    # Every run has to have a unique id in the form of a list.
                    run.set_property("id", [domain, task_name])

# Add step that writes experiment files to disk.
exp.add_step("build", exp.build)

# Add step that executes all runs.
exp.add_step("start", exp.start_runs)

exp.add_parse_again_step()

# Add step that collects properties from run directories and
# writes them to *-eval/properties.
exp.add_fetcher(name="fetch")

# Make a report.
exp.add_report(BaseReport(attributes=ATTRIBUTES), outfile="report.html")

# Parse the commandline and run the given steps.
exp.run_steps()
