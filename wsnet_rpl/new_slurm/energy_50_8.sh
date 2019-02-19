#!/bin/bash
rm -rf out_energy_50_8 
mkdir out_energy_50_8 
wsnet-run-simulations -f ALL_energy_50.xml out_energy_50_8 1
