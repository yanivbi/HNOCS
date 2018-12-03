# HNoCS: Modular Open-Source Simulator for Heterogeneous NoCs.

HNOCS is an open-source implementation of a NoC simulation framework using OMNeT++. As an event driven simulation engine OMNeT++ provides C++ APIs to a reach set of services that can be used to model, configure, describe the topology, collect simulation data and perform analysis. OMNeT++ provides extensive user documentation and training material enabling fast ramp-up time for new researches. The HNoCS framework utilizes the OMNeT++ module interface feature to support runtime selection of simulation modules from a library of parametrized components. The models provided support heterogeneous NoC configuration in terms of link capacity and number of VCs. HNOCS modules available today implement wormhole switching, with round-robin or winner-takes-all arbitration. 


Please cite our summary paper when you publish results that you have obtained with HNOCS:
"HNOCS: Modular Open-Source Simulator for Heterogeneous NoCs"
Y. Ben-Itzhak, E. Zahavi, I. Cidon, and A. Kolodny. , SAMOS XII: International Conference on Embedded Computer Systems: Architectures, Modeling and Simulation
[PDF]          [bibtex]
