#!/bin/bash
rm -rf out_residual_50_5 
mkdir out_residual_50_5 
wsnet-run-simulations -f ALL_residual_50.xml out_residual_50_5 1
