#!/bin/bash
rm -rf out_energy_90_6 
mkdir out_energy_90_6 
wsnet-run-simulations -f ALL_energy_90.xml out_energy_90_6 1
