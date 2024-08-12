
# μMon Reference Implementation

The reference implementation of SIGCOMM'24 paper 'Empowering Microsecond-level Network Monitoring with Wavelets'.

This repository contains cpp/python versions of the WaveSketch and part of the data trace (the whole trace is too large) used in the paper. 

The Python version gives straightforward logic about how WaveSketch (basic version) is implemented and works. Please see waveSketchTest.ipynb, where we give an example. Note that the python version's accuracy is not optimized. It's just for understanding.

The CPP version can reproduce the evaluation results in the paper.