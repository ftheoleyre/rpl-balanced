#!/bin/bash
rm -rf out_residual_90_4 
mkdir out_residual_90_4 
wsnet-run-simulations -f ALL_residual_90.xml out_residual_90_4 1
