#!/bin/bash
rm -rf out_residual_90_5 
mkdir out_residual_90_5 
wsnet-run-simulations -f ALL_residual_90.xml out_residual_90_5 1
