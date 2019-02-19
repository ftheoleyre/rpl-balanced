#!/bin/bash
rm -rf out_residual_90_1 
mkdir out_residual_90_1 
wsnet-run-simulations -f ALL_residual_90.xml out_residual_90_1 1
