// Copyright (c) 2025, The Regents of the University of California (Regents)
// See LICENSE for license details

#ifndef COMB_CACHE_H_
#define COMB_CACHE_H_

#include <algorithm>
#include <array>
#include <cstdint>


/*
PivotScale
File:   CombCache
Author: Scott Beamer, Amogh Lonkar

Computes (n choose k), and if inputs are small, uses precomputed values
*/


template <typename T_>
class CombCache {
  static const int kNumPrecompute = 100;
  std::array<std::array<T_, kNumPrecompute>, kNumPrecompute> memo;

  T_ compute(T_ n, T_ k) const {
    if (k > n)
      return 0;
    if (k == 0 || k == n)
      return 1;
    k = std::min(k, n-k);
    T_ result = 1;
    for (T_ i=1; i <= k; i++) {
      result = result * (n - (k - i)) / i;
    }
    return result;
  }

 public:
  CombCache() {
    for (int n=0; n < kNumPrecompute; n++) {
      for (int k=0; k <= n; k++) {
        if (k == 0 || k == n)
          memo[n][k] = 1;
        else
          memo[n][k] = memo[n-1][k-1] + memo[n-1][k];
      }
    }
  }

  T_ operator()(int n, int k) const {
    // ASSUMES: n>0, k>0, k<n
    if (n < kNumPrecompute && k < kNumPrecompute)
      return memo[n][k];
    return compute(n, k);
  }
};

#endif  // COMB_CACHE_H_
