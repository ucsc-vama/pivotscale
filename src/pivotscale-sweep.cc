// Copyright (c) 2025, The Regents of the University of California (Regents)
// See LICENSE for license details

#include "pivotscale.h"


/*
PivotScale
File:   PivotScale-Sweep
Author: Amogh Lonkar, Scott Beamer

Counts occurrences of cliques for all sizes up to and including k
*/


void PivotRecurse(SubGraph &sg, NodeID max_k, std::vector<count_t> &counts,
                  NodeID clique_size, NodeID pivots) {
  NodeID holds = clique_size - pivots;
  if (sg.NumActive() == 0 || (holds == max_k)) {
    for (NodeID p=0; p <= std::min(pivots, max_k - holds); p++) {
      counts[holds + p] += n_choose_k(pivots, p);
    }
    return;
  }
  NodeID pivot_id_r = sg.FindPivot();
  auto verts_to_induce = sg.ActiveUnreachableFromPivot(pivot_id_r);
  for (NodeID v_r : verts_to_induce) {
    if (v_r == pivot_id_r) {
      std::vector<NodeID> empty_vec;
      sg.InduceFromSelfMutate(v_r, empty_vec);
      PivotRecurse(sg, max_k, counts, clique_size+1, pivots+1);
    } else {
      sg.InduceFromSelfMutate(v_r, verts_to_induce);
      PivotRecurse(sg, max_k, counts, clique_size+1, pivots);
    }
    sg.UndoSelfMutate();
  }
  sg.PopNonNeighbors();
}


std::vector<count_t> PivotCount(const Graph &dag, NodeID max_k) {
  std::vector<count_t> counts(max_k+1, 0);
  #pragma omp parallel
  {
    SubGraph sg;
    std::vector<count_t> local_counts(max_k+1, 0);
    #pragma omp for schedule(dynamic, 1) nowait
    for (NodeID v=0; v < dag.num_nodes(); v++) {
      sg.InduceFromDAG(dag, v);
      PivotRecurse(sg, max_k, local_counts, 1, 0);
    }
    for (size_t k=0; k < local_counts.size(); k++) {
      #pragma omp atomic
      counts[k] += local_counts[k];
    }
  }
  return counts;
}


void PrintCliqueCounts(const std::vector<count_t> &counts) {
  #ifdef USE_128
    printf("   k |                          clique count\n");
    printf("--------------------------------------------\n");
  #else
    printf("   k |        clique count\n");
    printf("--------------------------\n");
  #endif  // USE_128
  for (size_t k=0; k < counts.size(); k++) {
    if (counts[k] != 0) {
      PrintCliqueCountRow(k, counts[k]);
    }
  }
}


int main(int argc, char* argv[]) {
  CLKClique cli(argc, argv, "PivotScale clique count k-sweep", 3, false);
  if (!cli.ParseArgs()) {
    return -1;
  }
  Builder b(cli);
  Timer t;
  Graph dag;
  {  // reduced scope to trigger deletion of g for memory savings
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
  PrintTime("Directing Time", direct_time);

  NodeID max_k = cli.max_k() ? Ordering::FindMaxDegree(dag)+1 : cli.clique_size();
  t.Start();
  std::vector<count_t> counts = PivotCount(dag, max_k);
  t.Stop();
  double count_time = t.Seconds();

  PrintTime("Counting Time", count_time);
  PrintTime("Total Time", direct_time + count_time);
  PrintCliqueCounts(counts);
  return 0;
}
