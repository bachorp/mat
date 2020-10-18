#!/bin/bash
#SBATCH -a 0-69
#SBATCH -p gki_cpu-cascadelake
#SBATCH --cpus-per-task=4
#SBATCH --mem=16G
#SBATCH --time=24:00:00

let "GRID = SLURM_ARRAY_TASK_ID / 4 + 3"
let "CONFIG = SLURM_ARRAY_TASK_ID % 4"

mkdir -p $CONFIG
/usr/bin/time --verbose mat 0 grid $GRID config $CONFIG $CONFIG/$GRID.csv
exit $?
