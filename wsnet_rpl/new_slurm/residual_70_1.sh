#!/bin/bash
rm -rf out_residual_70_1 
mkdir out_residual_70_1 
wsnet-run-simulations -f ALL_residual_70.xml out_residual_70_1 1
