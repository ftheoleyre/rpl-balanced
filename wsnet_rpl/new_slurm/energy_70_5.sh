#!/bin/bash
rm -rf out_energy_70_5 
mkdir out_energy_70_5 
wsnet-run-simulations -f ALL_energy_70.xml out_energy_70_5 1
