// Copyright (c) 2025, The Regents of the University of California (Regents)
// See LICENSE for license details

#ifndef SUBGRAPH_H_
#define SUBGRAPH_H_

#include <iostream>
#include <span>
#include <utility>
#include <vector>

#include "benchmark.h"
#include "graph.h"
#include "grouped_stack.h"
#include "hash_table8.hpp"


/*
PivotScale
File:   SubGraph
Author: Scott Beamer, Amogh Lonkar

Subgraph providing capabilities needed for Pivotscale
- Initially created by inducing from a vertex (InduceFromDAG)
- Further subgraph inductions mutate this data structure (InduceFromSelfMutate)
- Can undo a subgraph induction (UndoSelfMutate)
- Can induce (and undo) an arbitrary number of times (uses stack internally)
*/


class SubGraph {
  // active list (P set)
  std::vector<uint8_t> active_;
  std::vector<NodeID> active_list_;
  // adjacency list
  std::vector<std::vector<NodeID>> adj_list_;
  std::vector<NodeID> active_tails_;
  // stack-style frames to hold dropped vertices or non-neighbors of pivot
  GroupedStack<NodeID> dropped_verts_;
  GroupedStack<NodeID> pivot_non_neighs_;


 public:
  SubGraph() {}


  void InduceFromDAG(const Graph &dag, NodeID u) {
    // Initialize and reset data structures
    NodeID num_orig_nodes = dag.out_degree(u);
    emhash8::HashMap<NodeID, NodeID> remapper;
    remapper.reserve(num_orig_nodes);
    active_.assign(num_orig_nodes, false);
    active_list_.clear();
    adj_list_.resize(num_orig_nodes);
    active_tails_.resize(num_orig_nodes);
    dropped_verts_.clear();
    pivot_non_neighs_.clear();
    pivot_non_neighs_.reserve(num_orig_nodes);

    // Populate remappings for vertices included and mark active
    for (NodeID v : dag.out_neigh(u)) {
      NodeID v_r = remapper.size();
      remapper.emplace_unique(v, v_r);
      active_[v_r] = true;
      active_list_.push_back(v_r);
      adj_list_[v_r].clear();
    }

    // Build new subgraph of neighbors of u
    for (NodeID v : dag.out_neigh(u)) {
      NodeID v_r = remapper[v];
      for (NodeID w : dag.out_neigh(v)) {
        if (remapper.contains(w)) {
          NodeID w_r = remapper[w];
          adj_list_[v_r].push_back(w_r);
          adj_list_[w_r].push_back(v_r);
        }
      }
    }
    for (NodeID v_r : active_list_) {
      active_tails_[v_r] = adj_list_[v_r].size();
    }
  }


  NodeID NumActive() {
    return active_list_.size();
  }


  std::span<const NodeID> Neighs(NodeID u_r) const {
    return std::span(&adj_list_[u_r][0], &adj_list_[u_r][active_tails_[u_r]]);
  }


  // has highest active degree (or tied)
  NodeID FindPivot() {
    assert(NumActive() > 0);
    NodeID max_v_r = active_list_.front();
    for (NodeID n_r : active_list_) {
      if (active_tails_[n_r] > active_tails_[max_v_r])
        max_v_r = n_r;
    }
    return max_v_r;
  }


  // NOTE: includes self (usually pivot) since no self-loops
  std::span<const NodeID> ActiveUnreachableFromPivot(NodeID u_r) {
    pivot_non_neighs_.create_new_frame();
    // mark all neighbors as inactive
    for (NodeID v_r : Neighs(u_r)) {
      active_[v_r] = false;
    }
    // recognize difference between active and active_list to do set difference
    for (NodeID n_r : active_list_) {
      if (active_[n_r]) {
        pivot_non_neighs_.push_back(n_r);
      } else {
        active_[n_r] = true;
      }
    }
    return pivot_non_neighs_.last_frame_iter();
  }


  void InduceFromSelfMutate(NodeID u_r, const std::span<const NodeID> &excl) {
    // unset all bitmap entries (temporary)
    for (NodeID n_r : active_list_) {
      active_[n_r] = false;
    }
    // set bitmap for next active
    for (NodeID v_r : Neighs(u_r)) {
      active_[v_r] = true;
    }
    // subtract excl(usion) list from active, considering vertex ID ordering
    for (NodeID n_r : excl) {
      if (n_r < u_r)
        active_[n_r] = false;
    }
    dropped_verts_.create_new_frame();
    // count on active_list_ to hold old active_ to recognize future inactive
    for (NodeID i=0; i < static_cast<NodeID>(active_list_.size()); i++) {
      NodeID n_r = active_list_[i];
      if (active_[n_r]) {
        for (NodeID j=0; j < active_tails_[n_r]; j++) {
          NodeID v_r = adj_list_[n_r][j];
          if (!active_[v_r]) {
            // v_r is now inactive, so need to swap to back of neighbor list
            NodeID new_tail = active_tails_[n_r] - 1;
            NodeID tail_v_r = adj_list_[n_r][new_tail];
            while ((new_tail > j) && (!active_[tail_v_r])) {
              new_tail--;
              tail_v_r = adj_list_[n_r][new_tail];
            }
            if (new_tail > j) {
              std::swap(adj_list_[n_r][j], adj_list_[n_r][new_tail]);
            }
            active_tails_[n_r] = new_tail;
          }
        }
      } else {
        // n_r is now inactive, so remove from active and add to dropped
        std::swap(active_list_[i], active_list_.back());
        active_list_.pop_back();
        dropped_verts_.push_back(n_r);
        i--;
      }
    }
  }


  void UndoSelfMutate() {
    // mark last dropped vertices as active
    for (NodeID n_r : dropped_verts_.last_frame_iter()) {
      active_[n_r] = true;
      active_list_.push_back(n_r);
    }
    dropped_verts_.pop_frame();
    // for all active vertices, extend neighbor lists to include newly active
    for (NodeID u_r : active_list_) {
      NodeID new_tail = active_tails_[u_r];
      while (new_tail < static_cast<NodeID>(adj_list_[u_r].size())) {
        NodeID tail_v_r = adj_list_[u_r][new_tail];
        if (active_[tail_v_r])
          new_tail++;
        else
          break;
      }
      active_tails_[u_r] = new_tail;
    }
  }


  void PopNonNeighbors() {
    pivot_non_neighs_.pop_frame();
  }


  void PrintTopoStats() {
    NodeID num_nodes_active = 0;
    int64_t num_edges_active = 0;
    for (NodeID n_r : active_list_) {
      num_nodes_active++;
      num_edges_active += active_tails_[n_r];
    }
    std::cout << "nodes: " << num_nodes_active;
    std::cout << " edges: " << num_edges_active << std::endl;
  }


  void PrintTopology() const {
    for (NodeID u_r : active_list_) {
      std::cout << u_r << ": ";
      for (NodeID v_r : Neighs(u_r)) {
        std::cout << v_r << " ";
      }
      std::cout <<std::endl;
    }
  }


  template <typename T_>
  void PrintVector(const std::vector<T_> &vec) {
    std::cout << "{";
    for (T_ n : vec) {
      std::cout << n << " ";
    }
    std::cout << "}" << std::endl;
  }
};

#endif  // SUBGRAPH_H_
