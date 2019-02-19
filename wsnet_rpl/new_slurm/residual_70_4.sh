#!/bin/bash
rm -rf out_residual_70_4 
mkdir out_residual_70_4 
wsnet-run-simulations -f ALL_residual_70.xml out_residual_70_4 1
