#!/bin/bash
rm -rf out_residual_30_4 
mkdir out_residual_30_4 
wsnet-run-simulations -f ALL_residual_30.xml out_residual_30_4 1
