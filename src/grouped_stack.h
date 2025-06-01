// Copyright (c) 2025, The Regents of the University of California (Regents)
// See LICENSE for license details

#ifndef GROUPED_STACK_H_
#define GROUPED_STACK_H_

#include <span>
#include <vector>


/*
PivotScale
File:   CombCache
Author: Scott Beamer, Amogh Lonkar

Provides abstraction of a stack where objects are pushed into a frame, and
can provide an iterator (via std::span) to that frame ignoring the rest of
the contents. New frames can be created with create_new_frame() and popped
with pop_frame().

NOTE: For stability of the output of last_frame_iter, ensure no insertions
(via push_back) or you have used reserve(new_size) to prevent the need for
realloc.
*/


template <typename T_>
class GroupedStack {
  std::vector<T_> elems_;
  std::vector<size_t> starts_;

 public:
  GroupedStack() {}

  void reserve(int num_elems) {
    elems_.reserve(num_elems);
  }

  void create_new_frame() {
    starts_.push_back(elems_.size());
  }

  void push_back(T_ new_elem) {
    // assert(elems_.capacity() > elems_.size());  // to detect if resizing
    elems_.push_back(new_elem);
  }

  std::span<const T_> last_frame_iter() const {
    return std::span(elems_.begin() + starts_.back(), elems_.end());
  }

  void pop_frame() {
    size_t new_size = starts_.back();
    starts_.pop_back();
    elems_.resize(new_size);
  }

  void clear() {
    // assert(starts_.size() == 0);
    // assert(elems_.size() == 0);
    starts_.clear();
    elems_.clear();
  }
};

#endif  // GROUPED_STACK_H_
