#!/bin/bash
rm -rf out_residual_50_4 
mkdir out_residual_50_4
wsnet-run-simulations -f ALL_residual_50.xml out_residual_50_4 1
