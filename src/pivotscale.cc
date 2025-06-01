// Copyright (c) 2025, The Regents of the University of California (Regents)
// See LICENSE for license details

#include "pivotscale.h"


/*
PivotScale
File:   PivotScale
Author: Amogh Lonkar, Scott Beamer

Counts occurrences of cliques of size k
*/


count_t PivotRecurse(SubGraph *sg, NodeID max_k, NodeID clique_size,
                     NodeID num_pivots) {
  if ((sg->NumActive() + clique_size) < max_k)
    return 0;
  NodeID num_holds = clique_size - num_pivots;
  if (sg->NumActive() == 0 || (num_holds == max_k)) {
    return n_choose_k(num_pivots, max_k - num_holds);
  }
  NodeID pivot_id_r = sg->FindPivot();
  count_t count = 0;
  auto verts_to_induce = sg->ActiveUnreachableFromPivot(pivot_id_r);
  for (NodeID v_r : verts_to_induce) {
    if (v_r == pivot_id_r) {
      std::vector<NodeID> empty_vec;
      sg->InduceFromSelfMutate(v_r, empty_vec);
      count += PivotRecurse(sg, max_k, clique_size+1, num_pivots+1);
    } else {
      sg->InduceFromSelfMutate(v_r, verts_to_induce);
      count += PivotRecurse(sg, max_k, clique_size+1, num_pivots);
    }
    sg->UndoSelfMutate();
  }
  sg->PopNonNeighbors();
  return count;
}


count_t PivotCount(const Graph &dag, NodeID k) {
  count_t count = 0;
  #pragma omp parallel
  {
    SubGraph sg;
    // SubGraph sg(dag.num_nodes()); // use only for dense
    #pragma omp for reduction(+ : count) schedule(dynamic, 1)
    for (NodeID v=0; v < dag.num_nodes(); v++) {
      sg.InduceFromDAG(dag, v);
      count += PivotRecurse(&sg, k, 1, 0);
    }
  }
  return count;
}


int main(int argc, char* argv[]) {
  CLKClique cli(argc, argv, "PivotScale clique counting", 3, false);
  if (!cli.ParseArgs()) {
    return -1;
  }
  Builder b(cli);
  Timer t;
  Graph dag;
  {  // restricted scope to trigger deletion of g for memory savings
    Graph g = b.MakeGraph();
    if (g.directed()) {
      std::cout << "Input graph is directed but clique counting requires";
      std::cout << " undirected" << std::endl;
      std::exit(-2);
    }
    t.Start();
    dag = Ordering::Directionalize(g, b);
    t.Stop();
  }

  double direct_time = t.Seconds();
  dag.PrintStats();
  PrintStep("Max Degree", static_cast<int64_t>(Ordering::FindMaxDegree(dag)));
  PrintTime("Directing Time", direct_time);

  t.Start();
  count_t k_count = PivotCount(dag, cli.clique_size());
  t.Stop();
  double count_time = t.Seconds();

  PrintTime("Counting Time", count_time);
  PrintTime("Total Time", direct_time + count_time);
  std::cout << "k: ";
  PrintCliqueCountRow(cli.clique_size(), k_count);
  return 0;
}
