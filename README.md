# HNoCS: Modular Open-Source Simulator for Heterogeneous NoCs.

HNOCS is an open-source implementation of a NoC simulation framework using OMNeT++. As an event driven simulation engine OMNeT++ provides C++ APIs to a reach set of services that can be used to model, configure, describe the topology, collect simulation data and perform analysis. OMNeT++ provides extensive user documentation and training material enabling fast ramp-up time for new researches. The HNoCS framework utilizes the OMNeT++ module interface feature to support runtime selection of simulation modules from a library of parametrized components. The models provided support heterogeneous NoC configuration in terms of link capacity and number of VCs. HNOCS modules available today implement wormhole switching, with round-robin or winner-takes-all arbitration. 

For getting started and FAQ, see:  
[GETTING STARTED](GETTING%20STARTED)  
[How to Add HNOCS to OMNeT Workspace](How%20to%20Add%20HNOCS%20to%20OMNeT%20Workspace.pdf)  
[FAQ](FAQ.pdf)  
Also, see the [HNOCS Google discussion group](https://groups.google.com/forum/#!forum/hnocs-simulator) 




A summary paper of HNOCS:  
Y. Ben-Itzhak, E. Zahavi, I. Cidon, and A. Kolodny, "[HNOCS: Modular Open-Source Simulator for Heterogeneous NoCs](https://webee.technion.ac.il/Sites/People/kolodny/ftp/HNOCS%20Modular%20Open-Source%20Simulator%20for%20Heterogeneous%20NoCs.pdf)"
 , SAMOS XII: International Conference on Embedded Computer Systems: Architectures, Modeling and Simulation

bibtex:
<pre>
@inproceedings{ben2012hnocs,
  title={HNOCS: modular open-source simulator for heterogeneous NoCs},
  author={Ben-Itzhak, Yaniv and Zahavi, Eitan and Cidon, Israel and Kolodny, Avinoam},
  booktitle={2012 international conference on embedded computer systems (SAMOS)},
  pages={51--57},
  year={2012},
  organization={IEEE}
}
</pre>
