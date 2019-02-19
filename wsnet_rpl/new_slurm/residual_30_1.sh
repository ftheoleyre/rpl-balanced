#!/bin/bash
rm -rf out_residual_30_1 
mkdir out_residual_30_1 
wsnet-run-simulations -f ALL_residual_30.xml out_residual_30_1 1
