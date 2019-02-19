#!/bin/bash
rm -rf out_residual_50_8 
mkdir out_residual_50_8 
wsnet-run-simulations -f ALL_residual_50.xml out_residual_50_8 1
