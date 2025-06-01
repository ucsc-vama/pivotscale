// Copyright (c) 2025, The Regents of the University of California (Regents)
// See LICENSE for license details

#ifndef PIVOTSCALE_H_
#define PIVOTSCALE_H_

#include <cstdint>
#include <iostream>
#include <vector>

#include "benchmark.h"
#include "builder.h"
#include "comb_cache.h"
#include "command_line.h"
#include "graph.h"
#include "ordering.h"
#include "subgraph.h"


/*
PivotScale
File:   PivotScale Header
Author: Amogh Lonkar, Scott Beamer

Header to share imports and utils between pivotscale.cc and pivotscale-sweep.cc
*/


#ifdef USE_128
  using count_t = unsigned __int128;
#else
  using count_t = uint64_t;
#endif  // USE_128

CombCache<count_t> n_choose_k;

void Print_uint128(unsigned __int128 x) {
  char buffer[40];
  int i = sizeof(buffer) - 1;
  buffer[i] = '\0';
  do {
    buffer[--i] = '0' + (x % 10);
    x /= 10;
  } while (x > 0);
  printf("%39s", &buffer[i]);
}

void PrintCliqueCountRow(size_t k, count_t count) {
  #ifdef USE_128
    printf("%4zu ", k);
    Print_uint128(count);
    printf("\n");
  #else
    printf("%4zu %21" PRIu64 "\n", k, count);
  #endif  // USE_128
}

#endif  // PIVOTSCALE_H_
