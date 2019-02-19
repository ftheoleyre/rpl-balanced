#!/bin/bash
rm -rf out_residual_90_3 
mkdir out_residual_90_3 
wsnet-run-simulations -f ALL_residual_90.xml out_residual_90_3 1
