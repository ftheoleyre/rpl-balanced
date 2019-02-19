#!/bin/bash
rm -rf out_energy_50_4 
mkdir out_energy_50_4 
wsnet-run-simulations -f ALL_energy_50.xml out_energy_50_4 1
