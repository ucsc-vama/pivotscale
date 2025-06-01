// Copyright (c) 2025, The Regents of the University of California (Regents)
// See LICENSE for license details

#ifndef ORDERING_H_
#define ORDERING_H_

#include <algorithm>
#include <iostream>
#include <ranges>
#include <utility>
#include <vector>

#include "benchmark.h"
#include "graph.h"

/*
PivotScale
File:   Ordering
Author: Amogh Lonkar, Scott Beamer

Various ordering methods used to directionalize the graph
*/

namespace Ordering {

NodeID FindMaxDegree(const Graph &g) {
  int64_t max_seen = 0;
  #pragma omp parallel for reduction(max : max_seen)
  for (NodeID n=0; n < g.num_nodes(); n++) {
    max_seen = std::max(max_seen, g.out_degree(n));
  }
  return static_cast<NodeID>(max_seen);
}


bool CoreIsAdvantageous(const Graph &g, const double param_a = 0.0015,
                                        const double param_b = 0.1) {
  // Find ID of highest degree vertex and its highest degree neighbor
  auto compare_by_degree = [&g](NodeID a, NodeID b) {
    return g.out_degree(a) < g.out_degree(b); };
  auto vertex_id_range = std::views::iota(0, g.num_nodes());  
  auto biggest_id = std::ranges::max(vertex_id_range, compare_by_degree);
  auto biggest_neigh = std::ranges::max(g.out_neigh(biggest_id), compare_by_degree);

  // Compute intersection size
  NodeID intersection_size = 0;
  auto it = g.out_neigh(biggest_id).begin();
  for (NodeID w : g.out_neigh(biggest_neigh)) {
    while (*it < w) {
      it++;
    }
    if (w == *it) {
      intersection_size++;
    }
  }

  double largest_neigh_frac = static_cast<double>(g.out_degree(biggest_neigh)) / g.num_nodes();
  double intersection_frac = static_cast<double>(intersection_size) / g.out_degree(biggest_neigh);

  return (g.num_nodes() > 1000000) &&
         ((largest_neigh_frac > param_a) || (intersection_frac > param_b));
}


std::vector<NodeID> CoreSequential(const Graph &g) {
  std::vector<NodeID> ranking(g.num_nodes());
  std::vector<NodeID> index_in_level(g.num_nodes());
  std::vector<NodeID> curr_degree(g.num_nodes());

  std::vector<std::vector<NodeID>> nodes_at_degree;
  for (NodeID n = 0; n < g.num_nodes(); n++) {
    NodeID degree = g.out_degree(n);
    curr_degree[n] = degree;
    if (static_cast<size_t>(degree) >= nodes_at_degree.size()) {
      nodes_at_degree.resize(degree + 1);
    }
    index_in_level[n] = nodes_at_degree[degree].size();
    nodes_at_degree[degree].push_back(n);
  }

  NodeID num_removed = 0;
  NodeID min_degree = 0;
  while (num_removed < g.num_nodes()) {
    if (!nodes_at_degree[min_degree].empty()) {
      // Pop min degree vertex and assign ranking
      NodeID u = nodes_at_degree[min_degree].back();
      nodes_at_degree[min_degree].pop_back();
      curr_degree[u] = -1;
      index_in_level[u] = -1;
      ranking[u] = num_removed++;
      // Update degrees of neighbors
      for (NodeID v : g.out_neigh(u)) {
        NodeID v_deg = curr_degree[v];
        if (v_deg != -1) {      // neighbor is still unranked
          // Swap decremented neighbor to end and pop
          NodeID swapped_id = nodes_at_degree[v_deg].back();
          std::swap(nodes_at_degree[v_deg][index_in_level[v]], nodes_at_degree[v_deg].back());
          index_in_level[swapped_id] = index_in_level[v];
          nodes_at_degree[v_deg].pop_back();
          // Insert into correct level
          index_in_level[v] = nodes_at_degree[v_deg-1].size();
          nodes_at_degree[v_deg-1].push_back(v);
          curr_degree[v] = v_deg - 1;
          min_degree = std::min(min_degree, v_deg - 1);
        }
      }
    } else {
      min_degree++;
    }
  }
  return ranking;
}


std::vector<NodeID> CoreApprox(const Graph &g, double epsilon) {
  Timer t;
  std::vector<NodeID> rankings(g.num_nodes(), -1);
  pvector<NodeID> curr_degree(g.num_nodes());
  #pragma omp parallel for
  for (NodeID n = 0; n < g.num_nodes(); n++) {
    curr_degree[n] = g.out_degree(n);
  }
  int64_t active_degree_total = g.num_edges_directed();
  NodeID min_deg_active = g.num_nodes();
  NodeID num_remaining = g.num_nodes();

  NodeID level = 0;
  #pragma omp parallel
  {
    std::vector<NodeID> remaining, next_remaining, removed;

    while (num_remaining > 0) {
      double avg_deg = static_cast<double>(active_degree_total) / num_remaining;
      NodeID deg_thresh = static_cast<NodeID>((1 + epsilon) * avg_deg);
      #pragma omp barrier
      #pragma omp single
      {
        t.Start();
        num_remaining = 0;
        min_deg_active = g.num_nodes();
      }    

      int64_t local_edges_removed = 0;
      if (level == 0) {
        #pragma omp for schedule(static, 1024) nowait
        for (NodeID u=0; u<g.num_nodes(); u++) {
          if (g.out_degree(u) <= deg_thresh) {
            rankings[u] = level;
            for (NodeID v : g.out_neigh(u)) {
              if (g.out_degree(v) > deg_thresh) {
                #pragma omp atomic
                curr_degree[v]--;
                local_edges_removed++;
              }
            }
            local_edges_removed += curr_degree[u];
          } else {
            remaining.push_back(u);
          }
        }
      } else {
        // scan for min degree active vertex
        if (!remaining.empty()) {
          NodeID local_min_deg = g.num_nodes();
          for (NodeID u : remaining) {
            local_min_deg = std::min(local_min_deg, curr_degree[u]);
          }
          NodeID prior_min = min_deg_active;
          while (local_min_deg < prior_min &&
                 !compare_and_swap(min_deg_active, prior_min, local_min_deg)) {
            prior_min = min_deg_active;
          }
        }
        #pragma omp barrier
        if (deg_thresh < min_deg_active) {
          deg_thresh = min_deg_active;
        }
        // select vertices to remove
        for (NodeID u : remaining) {
          if (curr_degree[u] <= deg_thresh) {
            rankings[u] = level;
            removed.push_back(u);
          } else {
            next_remaining.push_back(u);
          }
        }
        #pragma omp barrier
        // remove vertices
        local_edges_removed = 0;
        for (NodeID u : removed) {
          for (NodeID v : g.out_neigh(u)) {
            if (rankings[v] == -1) {
              #pragma omp atomic
              curr_degree[v]--;
              local_edges_removed++;
            }
          }
          local_edges_removed += curr_degree[u];
        }
        remaining.swap(next_remaining);
        next_remaining.clear();
        removed.clear();
      }
      #pragma omp atomic
      active_degree_total -= local_edges_removed;
      #pragma omp atomic
      num_remaining += remaining.size();
      #pragma omp barrier

      #pragma omp single nowait
      {
        t.Stop();
        // PrintStep(level, t.Seconds(), num_remaining);
        level++;
      }
    }
  }
  return rankings;
}


Graph Directionalize(const Graph &g, const Builder &b) {
  if (CoreIsAdvantageous(g)) {
    std::cout << "Using core approximation ordering..." << std::endl;
    Timer t;
    t.Start();
    // vector<NodeID> ranking = CoreSequential(g);
    double epsilon = -0.5;
    std::vector<NodeID> ranking = CoreApprox(g, epsilon);
    t.Stop();
    PrintTime("Ranking", t.Seconds());
    return b.DirectGraphCore(g, ranking);
  } else {
    std::cout << "Using degree ordering..." << std::endl;
    return b.DirectGraphDegree(g);
  }
}

}  // namespace Ordering

#endif  // ORDERING_H_
