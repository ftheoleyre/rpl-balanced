#!/bin/bash
rm -rf out_energy_30_8 
mkdir out_energy_30_8 
wsnet-run-simulations -f ALL_energy_30.xml out_energy_30_8 1
