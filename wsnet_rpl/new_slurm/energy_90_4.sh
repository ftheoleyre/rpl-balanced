#!/bin/bash
rm -rf out_energy_90_4 
mkdir out_energy_90_4 
wsnet-run-simulations -f ALL_energy_90.xml out_energy_90_4 1
