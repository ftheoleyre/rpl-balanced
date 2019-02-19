#!/bin/bash
rm -rf out_energy_30_4 
mkdir out_energy_30_4 
wsnet-run-simulations -f ALL_energy_30.xml out_energy_30_4 1
