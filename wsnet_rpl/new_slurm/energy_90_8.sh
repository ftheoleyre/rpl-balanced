#!/bin/bash
rm -rf out_energy_90_8 
mkdir out_energy_90_8 
wsnet-run-simulations -f ALL_energy_90.xml out_energy_90_8 1
