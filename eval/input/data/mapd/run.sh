#!/usr/bin/env bash
#SBATCH -a 0-199
#SBATCH -p gki_cpu-cascadelake
#SBATCH --cpus-per-task=4
#SBATCH --mem=16G
#SBATCH --time=24:00:00

let "SEED = SLURM_ARRAY_TASK_ID / 10"
let "GRID = SLURM_ARRAY_TASK_ID % 10 + 3"

mkdir -p $GRID
/usr/bin/time --verbose ../build/mat d grid $GRID seed $SEED $GRID/$SEED.csv
exit $?
