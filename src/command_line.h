// Copyright (c) 2025, The Regents of the University of California (Regents)
// See LICENSE for license details

#ifndef COMMAND_LINE_H_
#define COMMAND_LINE_H_

#include <getopt.h>

#include <algorithm>
#include <cinttypes>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>


/*
GAP Benchmark Suite
Class:  CLBase
Author: Scott Beamer, Amogh Lonkar

Handles command line argument parsing
 - Through inheritance, can add more options to object
 - For example, most kernels will use CLApp
*/


class CLBase {
 protected:
  int argc_;
  char** argv_;
  std::string name_;
  std::string get_args_ = "f:g:hk:su:";
  std::vector<std::string> help_strings_;

  int scale_ = -1;
  int degree_ = 16;
  std::string filename_ = "";
  bool symmetrize_ = false;
  bool uniform_ = false;

  void AddHelpLine(char opt, std::string opt_arg, std::string text,
                   std::string def = "") {
    const int kBufLen = 100;
    char buf[kBufLen];
    if (opt_arg != "")
      opt_arg = "<" + opt_arg + ">";
    if (def != "")
      def = "[" + def + "]";
    snprintf(buf, kBufLen, " -%c %-9s: %-54s%10s", opt, opt_arg.c_str(),
            text.c_str(), def.c_str());
    help_strings_.push_back(buf);
  }

 public:
  CLBase(int argc, char** argv, std::string name = "") :
         argc_(argc), argv_(argv), name_(name) {
    AddHelpLine('h', "", "print this help message");
    AddHelpLine('f', "file", "load graph from file");
    AddHelpLine('s', "", "symmetrize input edge list", "false");
    AddHelpLine('g', "scale", "generate 2^scale kronecker graph");
    AddHelpLine('u', "scale", "generate 2^scale uniform-random graph");
    AddHelpLine('k', "degree", "average degree for synthetic graph",
                std::to_string(degree_));
  }

  bool ParseArgs() {
    signed char c_opt;
    extern char *optarg;          // from and for getopt
    while ((c_opt = getopt(argc_, argv_, get_args_.c_str())) != -1) {
      HandleArg(c_opt, optarg);
    }
    if ((filename_ == "") && (scale_ == -1)) {
      std::cout << "No graph input specified. (Use -h for help)" << std::endl;
      return false;
    }
    if (scale_ != -1)
      symmetrize_ = true;
    return true;
  }

  void virtual HandleArg(signed char opt, char* opt_arg) {
    switch (opt) {
      case 'f': filename_ = std::string(opt_arg);           break;
      case 'g': scale_ = atoi(opt_arg);                     break;
      case 'h': PrintUsage();                               break;
      case 'k': degree_ = atoi(opt_arg);                    break;
      case 's': symmetrize_ = true;                         break;
      case 'u': uniform_ = true; scale_ = atoi(opt_arg);    break;
    }
  }

  void PrintUsage() {
    std::cout << name_ << std::endl;
    // std::sort(help_strings_.begin(), help_strings_.end());
    for (std::string h : help_strings_)
      std::cout << h << std::endl;
    std::exit(0);
  }

  int scale() const { return scale_; }
  int degree() const { return degree_; }
  std::string filename() const { return filename_; }
  bool symmetrize() const { return symmetrize_; }
  bool uniform() const { return uniform_; }
};



class CLApp : public CLBase {
  bool do_analysis_ = false;
  int num_trials_ = 16;
  int64_t start_vertex_ = -1;
  bool do_verify_ = false;

 public:
  CLApp(int argc, char** argv, std::string name) : CLBase(argc, argv, name) {
    get_args_ += "";
    AddHelpLine('a', "", "output analysis of last run", "false");
    AddHelpLine('n', "n", "perform n trials", std::to_string(num_trials_));
    AddHelpLine('r', "node", "start from node r", "rand");
    AddHelpLine('v', "", "verify the output of each run", "false");
  }

  void HandleArg(signed char opt, char* opt_arg) override {
    switch (opt) {
      case 'a': do_analysis_ = true;                    break;
      case 'n': num_trials_ = atoi(opt_arg);            break;
      case 'r': start_vertex_ = atol(opt_arg);          break;
      case 'v': do_verify_ = true;                      break;
      default: CLBase::HandleArg(opt, opt_arg);
    }
  }

  bool do_analysis() const { return do_analysis_; }
  int num_trials() const { return num_trials_; }
  int64_t start_vertex() const { return start_vertex_; }
  bool do_verify() const { return do_verify_; }
};

class CLConvert : public CLBase {
  std::string out_filename_ = "";
  bool out_weighted_ = false;
  bool out_el_ = false;
  bool out_sg_ = false;

 public:
  CLConvert(int argc, char** argv, std::string name)
      : CLBase(argc, argv, name) {
    get_args_ += "e:b:w";
    AddHelpLine('b', "file", "output serialized graph to file");
    AddHelpLine('e', "file", "output edge list to file");
    AddHelpLine('w', "file", "make output weighted");
  }

  void HandleArg(signed char opt, char* opt_arg) override {
    switch (opt) {
      case 'b': out_sg_ = true; out_filename_ = std::string(opt_arg);   break;
      case 'e': out_el_ = true; out_filename_ = std::string(opt_arg);   break;
      case 'w': out_weighted_ = true;                                   break;
      default: CLBase::HandleArg(opt, opt_arg);
    }
  }

  std::string out_filename() const { return out_filename_; }
  bool out_weighted() const { return out_weighted_; }
  bool out_el() const { return out_el_; }
  bool out_sg() const { return out_sg_; }
};



class CLKClique : public CLBase {
  int clique_size_;
  std::string ordering_type_;
  int num_threads_;
  bool max_k_;
  double epsilon_;

 public:
  CLKClique(int argc, char** argv, std::string name, int clique_size, bool max_k) :
    CLBase(argc, argv, name), clique_size_(clique_size), max_k_(max_k)  {
    get_args_ += "c:m";
    AddHelpLine('c', "k", "clique size", std::to_string(clique_size_));
    AddHelpLine('m', "", "count all possible sizes of cliques", "false");
  }

  void HandleArg(signed char opt, char* opt_arg) override {
    switch (opt) {
      case 'c': clique_size_ = atoi(opt_arg);            break;
      case 'm': max_k_ = true;                           break;
      default: CLBase::HandleArg(opt, opt_arg);
    }
  }

  int clique_size() const { return clique_size_; }
  bool max_k() const { return max_k_; }
};

#endif  // COMMAND_LINE_H_
