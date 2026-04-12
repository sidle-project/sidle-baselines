#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include <utility>
#include <unordered_map>
#include <unordered_set>

constexpr int ENTRIES_COUNT = 16;

class sidle_histogram {
public:
  enum class type { hot = 0, warm, cold };

  sidle_histogram(const size_t size = 16): histogram_(size, 0), 
                                          next_histogram_(size, 0) {} 

  ~sidle_histogram() = default;

  /// @brief update node access statistic info and decide its current hotness
  type update(uint16_t access_count) {
    if (access_count <= 1) {
      ++next_histogram_[0];
    } else {
      size_t cur_index = get_idx(access_count);
      ++next_histogram_[cur_index];
#ifdef WORKER_DEBUG
      // if (access_count < 512) {
      //   SIDLE_RECORD(WORKER_DEBUG, "[update] the access count is %d, index: %d\n", 
      //     access_count, cur_index);
      // }
#endif
    }
    if (access_count > hot_threshold_) {
      return type::hot;
    }
    if (access_count < cold_threshold_) {
      return type::cold;
    }
    return type::warm;
  }

  /// @param total_number is the total number of leaf nodes in the new traversal
  void refresh(uint64_t total_number, uint64_t total_access_count) {
    total_number_ = total_number;
    total_access_count_ = total_access_count;
    // replace the histogram
    histogram_ = std::move(next_histogram_);
    next_histogram_.assign(histogram_.size(), 0);
  }

  // refer https://github.com/cosmoss-jigu/memtis
  void adjust_threshold() {
    uint64_t active_count = 0;
    uint64_t active_access = 0;
    if (total_number_ == 0) {
      return;
    }
    /// @note use static threshold temporarily
    // uint64_t active_lower_bound = 3 * total_number_ / 100;
    // uint64_t inactive_upper_bound = 10 * total_number_ / 100;
    uint64_t active_lower_bound = 5 * total_number_ / 100;
    uint64_t inactive_upper_bound = 80 * total_number_ / 100;
    int hot_index = -1, cold_index = -1;
    int weight = 1 << 15;
    for (int index = 15; index >= 0; --index) {
      // SIDLE_RECORD(WORKER_DEBUG, "[adjust_threshold] the histogram[%d] is %ld, active lower bound: %ld, inactive upper bound %ld\n", 
        // index, histogram_[index], active_lower_bound, inactive_upper_bound);
      if (active_count + histogram_[index] > active_lower_bound &&
          hot_index == -1) {
        hot_index = index;
      }
      if (active_count + histogram_[index] > inactive_upper_bound &&
          cold_index == -1) {
        cold_index = index;
        break;
      }
      active_count += histogram_[index];
      active_access += histogram_[index];
    }

    if (hot_index != 15) {
      ++hot_index;
    }
    if (cold_index == -1) {
      cold_index = 0;
    }
    hot_threshold_ = 1 << hot_index;
    cold_threshold_ = 1 << cold_index;
  }

  void adjust_for_cooling() {
    for (int i = 0; i < 15; ++i) {
      histogram_[i] = histogram_[i + 1];
    }
    histogram_[15] = 0;
    hot_threshold_ = hot_threshold_ >> 1;
    cold_threshold_ = cold_threshold_ >> 1;
  }

  inline void decrease_tolerance_for_cold(int value_for_decrease = 1) {
    cold_threshold_ = cold_threshold_ + value_for_decrease;
  }

private:  
  // refer https://github.com/cosmoss-jigu/memtis
  std::size_t get_idx(uint16_t num) {
    unsigned int cnt = 0;
    num++;
    while (1) {
      num = num >> 1;
      if (num)
        cnt++;
      else {
        return cnt;
      }

      if (cnt == 15)
        break;
    }
    return cnt;
  }

  // The x-axis is in exponential form to adapt to the zip distribution
  std::vector<uint64_t> histogram_;
  /// @brief the next histogram for the next round
  std::vector<uint64_t> next_histogram_;
  uint16_t hot_threshold_{64};
  uint16_t cold_threshold_{80};
  // The total number of the leaf node
  uint64_t total_number_{0};
  uint64_t total_access_count_{0};
};

int main() {
  std::ifstream in("zipf_dist.txt");
  if (!in.is_open()) {
    std::cerr << "Error: cannot open the file" << std::endl;
    exit(1);
  }
  std::vector<std::pair<uint64_t, uint64_t>> zipf_dist;
  uint64_t key, value;
  while (in >> key >> value) {
    zipf_dist.emplace_back(key, value);
  }
  int level = log(zipf_dist.size()) / log(ENTRIES_COUNT) + 1;
  sidle_histogram histogram;
  std::vector<std::unordered_set<uint64_t>> hot_path_nodes(level + 1);
  std::vector<std::unordered_set<uint64_t>> untracked_hot_path_nodes(level + 1);
  uint64_t total_number = 0;
  uint64_t total_access_count = 0;
  for (const auto &pair : zipf_dist) {
    histogram.update(pair.second);
    ++total_number;
    total_access_count += pair.second;
  }
  histogram.refresh(total_number, total_access_count);
  histogram.adjust_threshold();
  for (const auto &pair : zipf_dist) {
    if (histogram.update(pair.second) == sidle_histogram::type::hot) {
      hot_path_nodes[0].insert(pair.first);
      uint64_t key = pair.first;
      for (int i = 1; i <= level; ++i) {
        key = key / ENTRIES_COUNT;
        hot_path_nodes[i].insert(key);
      }
    }
  }

  std::vector<std::pair<uint64_t, uint64_t>> new_zipf_dist;
  auto refresh_zipf_dist = [&]() {
    new_zipf_dist.clear();
    size_t zipf_dist_size = zipf_dist.size();
    size_t j = 0;
    total_number = 0;
    total_access_count = 0;
    for (j = 0; j < zipf_dist_size; j += ENTRIES_COUNT) {
      uint64_t key = zipf_dist[j].first / ENTRIES_COUNT;
      uint64_t value = 0;
      for (size_t k = j; k < j + ENTRIES_COUNT; ++k) {
        value += zipf_dist[k].second;
      }
      new_zipf_dist.emplace_back(key, value);
      histogram.update(value);
      ++total_number;
      total_access_count += value;
    }
    if (j < zipf_dist_size) {
      uint64_t key = zipf_dist[j].first / ENTRIES_COUNT;
      uint64_t value = 0;
      for (size_t k = j; k < zipf_dist_size; ++k) {
        value += zipf_dist[k].second;
      }
      new_zipf_dist.emplace_back(key, value);
      histogram.update(value);
    }
    histogram.refresh(total_number, total_access_count);
    histogram.adjust_threshold();
  };
  
  refresh_zipf_dist();
  // get all hot path nodes
  for (int i = 1; i <= level; ++i) {
    zipf_dist = new_zipf_dist;
    std::cout << "level " << i << " total nodes: " << zipf_dist.size() << std::endl;
    for (const auto &pair : zipf_dist) {
      if (histogram.update(pair.second) == sidle_histogram::type::hot) {
        if (!hot_path_nodes[i].count(pair.first)) {
          // if (i == 1) {
          //   printf("untracked hot path node: %ld, access count: %ld\n", pair.first, pair.second);
          // }
          untracked_hot_path_nodes[i].insert(pair.first);
        } 
      }
    }
    refresh_zipf_dist();
  }
  for (int i = 1; i <= level; ++i) {
    std::cout << "level " << i << " hot path nodes: " << hot_path_nodes[i].size() <<  ", untracked hot path nodes: " << untracked_hot_path_nodes[i].size() << std::endl;
  }
  return 0;
}