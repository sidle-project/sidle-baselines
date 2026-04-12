

#include <getopt.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <unordered_set>

#include "helper.h"

double mean_v = 0;
double stddev = 2;
size_t datasize = 1000;
double scale = 100000000000000;
double tolerance = 2;
std::string outfile = "datasize_output.txt";
std::string dist = "lognormal";

void write_to_file(std::string &outfname, std::unordered_set<int64_t> &data) {
  std::vector<int64_t> dataset;
  dataset.reserve(data.size());
  for (auto &n : data) {
    dataset.push_back(n);
  }
  data.clear();

  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::shuffle(dataset.begin(), dataset.end(),
               std::default_random_engine(seed));
  COUT_THIS("done shuffle generated keys. keys.size()=" << dataset.size());

  std::fstream fs;
  fs.open(outfname.c_str(), std::fstream::out);
  INVARIANT(fs.is_open());
  COUT_THIS("Start Writing to file " << outfname << "...");

  uint64_t cnt = 0;
  for (auto &n : dataset) {
    if (cnt == dataset.size() - 1) {
      fs << n;
    } else {
      fs << n << "\n";
    }
    cnt++;
  }

  fs.close();
  COUT_THIS("Finish writing to file " << outfname);
}

void uniform_data_gen(size_t table_size, uint64_t maximum, std::string &fname) {
  INVARIANT(maximum < std::numeric_limits<int64_t>::max());
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> dis(0, maximum);
  std::unordered_set<int64_t> dataset;
  dataset.reserve(table_size);

  COUT_THIS("Start generate uniform dataset. datasize=" << datasize << "...");
  size_t cnt = 0;
  while (dataset.size() < table_size) {
    int64_t n = dis(gen);
    if (dataset.count(n) == 0) {
      INVARIANT(n >= 0);
      dataset.insert(n);
    }
    cnt++;
    if (cnt % (table_size / 10) == 0) {
      COUT_VAR(dataset.size());
    }
  }
  COUT_THIS("Finish generate dataset. datasize=" << datasize
                                                 << ", try_cnt=" << cnt);

  write_to_file(fname, dataset);
}

template <class distribution>
void normal_data_gen(distribution &dis, size_t table_size, double maximum,
                     double tole, std::string &fname) {
  std::random_device rd;
  std::mt19937 gen(rd());

  COUT_THIS("Start generate dataset. datasize=" << datasize << "...");
  // std::unordered_set<int64_t> raw_dataset;
  std::vector<double> raw_dataset;
  raw_dataset.reserve(tole * table_size);
  size_t cnt = 0;

  double min_raw_k = std::numeric_limits<int64_t>::max(),
         max_raw_k = std::numeric_limits<int64_t>::min();
  while (raw_dataset.size() < table_size * tole) {
    // int64_t d = std::round(dis(gen));
    double d = dis(gen);
    if (d > max_raw_k) max_raw_k = d;
    if (d < min_raw_k) min_raw_k = d;
    raw_dataset.push_back(d);
    // if (raw_dataset.count(d) == 0) {
    //   raw_dataset.insert(d);
    // }
    cnt++;
  }
  COUT_THIS("Finish scan raw data. size=" << raw_dataset.size()
                                          << " try_cnt=" << cnt);

  double scale_factor = maximum * 1.0 / (max_raw_k - min_raw_k);
  double bias = -1 * min_raw_k * scale_factor;
  bool out_max_range = (std::round(max_raw_k * scale_factor + bias) > maximum);
  bool out_min_range = (std::round(min_raw_k * scale_factor + bias) < 0);
  COUT_VAR(maximum);
  COUT_VAR(scale_factor);
  COUT_VAR(bias);
  COUT_VAR(out_max_range);
  COUT_VAR(out_min_range);

  std::unordered_set<int64_t> dataset;
  dataset.reserve(table_size);
  int64_t min_k = std::numeric_limits<int64_t>::max(),
          max_k = std::numeric_limits<int64_t>::min();
  size_t out_range_cnt = 0;
  cnt = 0;
  for (auto &i : raw_dataset) {
    double trans = i * scale_factor + bias;
    int64_t n = std::round(trans);
    if (n < maximum && n >= 0) {
      if (dataset.count(n) == 0) {
        dataset.insert(n);
        if (n < min_k) min_k = n;
        if (n > max_k) max_k = n;
        if (dataset.size() == datasize) break;
      }
    } else {
      out_range_cnt++;
    }
    cnt++;
    if (cnt % (table_size / 10) == 0) {
      COUT_VAR(dataset.size());
    }
  }
  COUT_VAR(dataset.size());

  COUT_THIS("Finish generate dataset. datasize="
            << datasize << ", try_cnt=" << cnt << ", out_range_cnt="
            << out_range_cnt << ", min_k=" << min_k << ", max_k=" << max_k);

  write_to_file(fname, dataset);
}

void linear_data_gen(size_t table_size, double maximum, std::string &fname) {
  int64_t gap = maximum / table_size;
  // TODO: check this bias term
  int64_t bias = gap / 2;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int64_t> dis(-bias, bias);

  COUT_THIS("Start generate dataset. datasize=" << datasize << "...");
  std::unordered_set<int64_t> dataset;
  dataset.reserve(table_size);

  size_t cnt = 0;
  int64_t prev_n = 0;
  int64_t min_k = std::numeric_limits<int64_t>::max(),
          max_k = std::numeric_limits<int64_t>::min();
  for (size_t i = 0; i < table_size; ++i) {
    int64_t n = (i + 1) * gap;
    while (true) {
      cnt++;
      int64_t num = n + dis(gen);
      if (num > prev_n && dataset.count(num) == 0) {
        prev_n = num;
        if (num < min_k) min_k = num;
        if (num > max_k) max_k = num;
        dataset.insert(num);
        break;
      }
    }
    if (cnt % (table_size / 10) == 0) {
      COUT_VAR(dataset.size());
    }
  }
  COUT_THIS("Finish generate dataset. datasize=" << datasize << ", try_cnt="
                                                 << cnt << ", min_k=" << min_k
                                                 << ", max_k=" << max_k);

  write_to_file(fname, dataset);
}

inline void parse_args(int argc, char **argv) {
  struct option long_options[] = {{"mean_v", required_argument, 0, 'a'},
                                  {"stddev", required_argument, 0, 'b'},
                                  {"size", required_argument, 0, 'c'},
                                  {"scale", required_argument, 0, 'd'},
                                  {"tolerance", required_argument, 0, 'e'},
                                  {"file", required_argument, 0, 'f'},
                                  {"dist", required_argument, 0, 'g'},
                                  {0, 0, 0, 0}};
  std::string ops = "a:b:c:d:e:f:g:";
  int option_index = 0;

  while (1) {
    int c = getopt_long(argc, argv, ops.c_str(), long_options, &option_index);
    if (c == -1) break;

    switch (c) {
      case 0:
        if (long_options[option_index].flag != 0) break;
        abort();
        break;
      case 'a':
        mean_v = strtod(optarg, NULL);
        break;
      case 'b':
        stddev = strtod(optarg, NULL);
        break;
      case 'c':
        datasize = strtoul(optarg, NULL, 10);
        INVARIANT(datasize > 0);
        break;
      case 'd':
        scale = strtod(optarg, NULL);
        INVARIANT(scale > 0);
        break;
      case 'e':
        tolerance = strtod(optarg, NULL);
        INVARIANT(tolerance > 0);
        break;
      case 'f':
        outfile = std::string(optarg);
        INVARIANT(!outfile.empty());
        break;
      case 'g':
        dist = std::string(optarg);
        INVARIANT(!dist.empty());
        break;
      default:
        abort();
    }
  }

  COUT_VAR(dist);
  COUT_VAR(mean_v);
  COUT_VAR(stddev);
  COUT_VAR(datasize);
  COUT_VAR(scale);
  COUT_VAR(tolerance);
}

int main(int argc, char **argv) {
  COUT_THIS("[Overall] dataset generator");
  parse_args(argc, argv);

  if (dist == "lognormal") {
    std::lognormal_distribution<> dis(mean_v, stddev);
    normal_data_gen(dis, datasize, scale, tolerance, outfile);
  } else if (dist == "normal") {
    std::normal_distribution<> dis(mean_v, stddev);
    normal_data_gen(dis, datasize, scale, tolerance, outfile);
  } else if (dist == "uniform") {
    uniform_data_gen(datasize, scale, outfile);
  } else if (dist == "linearnoise") {
    linear_data_gen(datasize, scale, outfile);
  } else {
    COUT_N_EXIT("the fuck?")
  }
}
