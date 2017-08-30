#!/bin/bash

sudo python ExperimentStringScaleTDF/vtmn_scalability.py
rm *.log

sudo python ExperimentStringGbLinkTDF/vtmn_fidelity.py
rm *.log

