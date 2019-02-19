#!/bin/bash
rm -rf out_residual_50_2 
mkdir out_residual_50_2 
wsnet-run-simulations -f ALL_residual_50.xml out_residual_50_2 1
