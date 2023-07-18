# OpenSHMEM Collective Routines Library

Welcome to the OpenSHMEM Collective Routines Library! This library provides a set of efficient collective communication routines for High-Performance Computing (HPC) applications using the OpenSHMEM programming model. The library contains multiple implementations for collective routines from OpenSHMEM 1.4 such as All-to-All, All-to-All Strided, Barrier, Broadcast, Collect, Fcollect, and Reduction to simplify communication and coordination among parallel processes. All collective routines are implemented using OpenSHMEM API and can be used with any OpenSHMEM library.

# Publication

The collective routines implemented in this repository, along with a comprehensive performance analysis, have been detailed in the following publication:

Milaković, Srđan, Oguz Selvitopi, Israt Nisa, Zoran Budimlić, and Aydin Buluç. "Parallel algorithms for masked sparse matrix-matrix products." In Proceedings of the 51st International Conference on Parallel Processing, pp. 1-11. 2022.

[Publication link](https://link.springer.com/chapter/10.1007/978-3-030-04918-8_6)

# Supported algorithms

* **alltoall**, **alltoalls** - shift exchange, XOR pair-wise exchange, and generalized pairwise exchange
* **barrier**, **sync** - linear, tree, binomial tree, and dissemination barrier
* **boradcast** - linear, tree, binomial tree, and Van de Geijn’s algorithm
* **collect***, **fcollect** - linear, all-linear, recursive doubling, ring, neighbor-exchange, and Bruck's algorithm
* **reduce** - linear, binomial, recursive doubling, and Rabenseifner's algorithm
