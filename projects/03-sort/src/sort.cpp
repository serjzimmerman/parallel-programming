/*
 * MIT License
 *
 * Copyright (c) 2024 Sergei Zimmerman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <boost/format.hpp>
#include <boost/mpi.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/vector.hpp>
#include <range/v3/all.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <type_traits>

namespace mpi = boost::mpi;
namespace po = boost::program_options;

namespace {

constexpr auto root_rank = 0;

template <typename It,
          typename = std::enable_if_t<std::is_base_of_v<
              std::random_access_iterator_tag,
              typename std::iterator_traits<It>::iterator_category>>>
void merge_sort(It start, It finish) {
  if (start == finish || finish == std::next(start))
    return;
  auto middle = start + (finish - start) / 2;
  merge_sort(start, middle);
  merge_sort(middle, finish);
  std::inplace_merge(start, middle, finish);
}

template <typename T>
void remove_prefix(std::span<T> &view, std::size_t n = 1) {
  view = view.subspan(n);
}

template <typename T>
void parallel_merge_sort(const mpi::communicator &comm,
                         std::vector<T> &values) {
  comm.barrier();
  auto chunked = std::vector<std::vector<T>>{};

  if (comm.rank() == root_rank) {
    auto element_per_chunk = values.size() / comm.size();
    if (element_per_chunk == 0)
      ++element_per_chunk;

    chunked = ranges::views::chunk(values, element_per_chunk) |
              ranges::views::transform(
                  [](auto &&range) { return range | ranges::to_vector; }) |
              ranges::to_vector;

    while (chunked.size() > comm.size()) {
      auto last = std::move(chunked.back());
      chunked.pop_back();
      ranges::copy(last, std::back_inserter(chunked.back()));
    }

    chunked.resize(comm.size());
  }

  auto mine = std::vector<T>{};
  mpi::scatter(comm, chunked, mine, root_rank);
  merge_sort(mine.begin(), mine.end());
  mpi::gather(comm, mine, chunked, root_rank);

  chunked.erase(
      std::remove_if(chunked.begin(), chunked.end(),
                     [](auto &&container) { return empty(container); }),
      chunked.end());

  auto num_heap = std::vector<std::span<const T>>{};

  ranges::transform(chunked, std::back_inserter(num_heap),
                    [](const std::vector<T> &sorted_subrange) {
                      return std::span{sorted_subrange};
                    });

  auto front_proj = [](auto &&span) { return span.front(); };
  auto comp = std::greater<T>{};

  ranges::make_heap(num_heap, comp, front_proj);

  values.clear();
  while (!empty(num_heap)) {
    ranges::pop_heap(num_heap, comp, front_proj);

    auto smallest_it = rbegin(num_heap);
    auto smallest_value = smallest_it->front();

    values.push_back(smallest_value);
    remove_prefix(*smallest_it);

    if (smallest_it->empty()) {
      num_heap.pop_back();
      continue;
    }

    ranges::push_heap(num_heap, comp, front_proj);
  }
}

} // namespace

auto main(int argc, char **argv) -> int {
  auto env = mpi::environment{argc, argv};
  const auto world = mpi::communicator{};

  auto desc = po::options_description{"allowed options"};

  desc.add_options()("help", "produce this help message")(
      "num", po::value<uint32_t>()->default_value(32),
      "number of elements to sort")("min",
                                    po::value<int32_t>()->default_value(0),
                                    "minimum for uniform random distribution")(
      "max", po::value<int32_t>()->default_value(128),
      "maximum for uniform random distribution")(
      "seed", po::value<uint64_t>()->default_value(0),
      "seed for random number generator")("verbose", "print verbose output")(
      "samples", po::value<uint32_t>()->default_value(2048),
      "number of samples to average over")("parallel",
                                           "use mpi to sort in parallel");

  auto vm = po::variables_map{};
  po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
  po::notify(vm);

  auto generator = [&] {
    auto min = vm.at("min").as<int32_t>();
    auto max = vm.at("max").as<int32_t>();
    auto random_engine = std::mt19937{vm.at("seed").as<uint64_t>()};
    auto distribution = std::uniform_int_distribution<>{min, max};
    return [=]() mutable { return distribution(random_engine); };
  }();

  const auto values =
      ranges::views::generate_n(generator, vm.at("num").as<unsigned>()) |
      ranges::to_vector;

  auto measure_time =
      [&vm,
       &world](auto callable) -> std::chrono::duration<double, std::milli> {
    auto run_once = [&] {
      auto runnable = callable();
      auto begin_time = std::chrono::high_resolution_clock::now();
      runnable();
      auto end_time = std::chrono::high_resolution_clock::now();
      return std::chrono::duration<double>{end_time - begin_time}.count();
    };

    auto num_samples = vm.at("samples").as<uint32_t>();
    auto times = ranges::views::generate_n(run_once, num_samples);

    return std::chrono::duration<double>{ranges::accumulate(times, 0.0) /
                                         num_samples};
  };

  auto use_parallel = vm.count("parallel");

  auto serial = [&values]() {
    return [values = values]() mutable {
      merge_sort(values.begin(), values.end());
      if (!ranges::is_sorted(values))
        throw std::logic_error{"serial sort does not work properly"};
    };
  };

  auto parallel = [&world, &values]() mutable {
    return [&world, values = values]() mutable {
      parallel_merge_sort(world, values);
      if (!ranges::is_sorted(values))
        throw std::logic_error{"parallel sort does not work properly"};
    };
  };

  auto duration = use_parallel ? measure_time(std::move(parallel))
                               : measure_time(std::move(serial));

  if (world.rank() != root_rank)
    return 0;

  if (vm.count("verbose")) {
    const auto *type = use_parallel ? "parallel" : "serial";
    std::cout << "number of elements: " << vm.at("num").as<uint32_t>() << "\n"
              << type << " sort took: " << duration << "\n";
  } else {
    std::cout << duration.count() << "\n";
  }

  return 0;
}
