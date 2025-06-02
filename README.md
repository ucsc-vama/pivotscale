PivotScale 
===================

This is the optimized parallel implementation for PivotScale, a high-performance pivoting-based clique counting algorithm. 


Quick Start
-----------
Building PivotScale requires a compiler that supports C++20 and OpenMP.

Compile PivotScale:

    $ make pivotscale

Execute PivotScale on the DBLP graph to count 8-cliques:

    $ ./pivotscale -f dblp.sg -c 8

PivotScale is based on the [gapbs codebase](https://github.com/sbeamer/gapbs) and includes its graph generation and loading support. Consult the [gapbs README](https://github.com/sbeamer/gapbs/blob/master/README.md) for more information on additional flags for graph loading. PivotScale requires input graphs to be unweighted and undirected, and input graphs can be made undirected while loading with the `-s` flag. Additional command line flags can be found with `-h`.


Additional Tips
---------------

PivotScale uses the number of threads provided by the OpenMP runtime. The best way to set the number of threads is to set the `OMP_NUM_THREADS` environment variable, e.g. to use 16 threads:

    $ OMP_NUM_THREADS=16 ./pivotscale -f dblp.sg -c 8

On most platforms, the default number of threads for OpenMP is the number of hardware thread contexts (including hyperthreading if present). Hyperthreading (SMT) can provide benefit for this workload, but we recommend experimenting with the number of threads to find the best performance.

To minimize graph loading time, we recommend using gapbs's serialized graph format (`.sg`) which can be made using the included `converter` tool. We also provide a script to convert a graph from [SNAP](https://snap.stanford.edu/data/index.html) into that `.sg` format:

    $ bash ConvertSNAP.sh path_to_graph_from_snap.txt

Pivoting-based _k_-clique counting approaches can also efficiently count the number of occurrences of every clique size up through _k_. We include an implementation variant capable of that, and it can be built with:

    $ make pivotscale-sweep

Very large clique counts can overflow the 64-bit integers used to hold the counts (default), so PivotScale can be compiled to use 128-bit integers for counting:

    $ make pivotscale USE_128=1


How to Cite
-----------

If you use this code, please cite our paper:

> Amogh Lonkar, Scott Beamer. *PivotScale: A Holistic Approach for Scalable Clique Counting*. International Parallel & Distributed Processing Symposium (IPDPS), June 2025.
