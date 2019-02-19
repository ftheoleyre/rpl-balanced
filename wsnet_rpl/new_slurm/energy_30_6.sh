#!/bin/bash
rm -rf out_energy_30_6 
mkdir out_energy_30_6 
wsnet-run-simulations -f ALL_energy_30.xml out_energy_30_6 1
