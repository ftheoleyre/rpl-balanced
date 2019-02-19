#!/bin/bash
rm -rf out_energy_50_3 
mkdir out_energy_50_3 
wsnet-run-simulations -f ALL_energy_50.xml out_energy_50_3 1
