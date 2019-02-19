#!/bin/bash
rm -rf out_residual_30_2 
mkdir out_residual_30_2 
wsnet-run-simulations -f ALL_residual_30.xml out_residual_30_2 1
