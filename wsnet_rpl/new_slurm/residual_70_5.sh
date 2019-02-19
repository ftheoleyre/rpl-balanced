#!/bin/bash
rm -rf out_residual_70_5 
mkdir out_residual_70_5 
wsnet-run-simulations -f ALL_residual_70.xml out_residual_70_5 1
