#!/bin/bash
#SBATCH --time=24:0:00
#SBATCH --mem=1000
#SBATCH -N 1
#SBATCH -n 8
#SBATCH -p public
#SBATCH -J iovarplwsnet
#SBATCH --mail-type=ALL
#SBATCH --mail-user=otiova@unistra.fr

parallel -j 8 < batch_15.sh 
