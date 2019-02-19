#!/bin/bash
rm -rf out_energy_30_0 
mkdir out_energy_30_0 
wsnet-run-simulations -f ALL_energy_30.xml out_energy_30_0 1
