#!/bin/bash
rm -rf out_energy_90_9 
mkdir out_energy_90_9 
wsnet-run-simulations -f ALL_energy_90.xml out_energy_90_9 1
