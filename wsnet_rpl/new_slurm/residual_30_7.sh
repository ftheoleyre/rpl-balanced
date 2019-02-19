#!/bin/bash
rm -rf out_residual_30_7 
mkdir out_residual_30_7 
wsnet-run-simulations -f ALL_residual_30.xml out_residual_30_7 1
