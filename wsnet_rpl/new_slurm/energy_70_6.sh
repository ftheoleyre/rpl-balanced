#!/bin/bash
rm -rf out_energy_70_6 
mkdir out_energy_70_6 
wsnet-run-simulations -f ALL_energy_70.xml out_energy_70_6 1
