#!/bin/bash
rm -rf out_energy_50_6 
mkdir out_energy_50_6 
wsnet-run-simulations -f ALL_energy_50.xml out_energy_50_6 1
