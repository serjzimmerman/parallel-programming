// Copyright (c) 2024 Sergei Zimmerman
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <boost/container/static_vector.hpp>
#include <boost/program_options.hpp>
#include <fmt/core.h>
#include <range/v3/all.hpp>

#include <atomic>
#include <concepts>
#include <iostream>
#include <numeric>

#include "concurrentqueue.h"

#if 0
#define DEBUG_PRINTS
#endif

namespace po = boost::program_options;

namespace {

namespace lockfree {

template <std::floating_point T> struct task {
  T a;
  T b;
  T func_val_a;
  T func_val_b;
  T integral;
};

template <std::floating_point T>
auto parallel_integrate(T a, T b, auto function, uint32_t num_threads, T eps)
    -> T {
  auto active_count = std::atomic<uint32_t>{0};
  constexpr auto queue_size_multiplier = 4;
  auto global_queue = moodycamel::ConcurrentQueue<lockfree::task<T>>{
      queue_size_multiplier * num_threads};
  auto accumulated_integral = std::atomic<T>{};
  static_assert(std::atomic<T>::is_always_lock_free);

  auto global_handler = [&](task<T> global_task) {
    constexpr auto local_queue_max_size = 32;
    constexpr auto capacity_extra = 4;

    auto local_accumulator = T{0};
    auto local_queue =
        boost::container::static_vector<lockfree::task<T>,
                                        capacity_extra + local_queue_max_size>{
            std::move(global_task)};

    auto local_handler = [&](lockfree::task<T> task) {
      auto mid = std::midpoint(task.a, task.b);
      auto func_val_mid = function(mid);
      auto integral_left =
          std::midpoint(task.func_val_a, func_val_mid) * (mid - task.a);
      auto integral_right =
          std::midpoint(func_val_mid, task.func_val_b) * (task.b - mid);
      auto new_integral = integral_left + integral_right;

      if (std::abs(new_integral - task.integral) >=
          eps * std::abs(new_integral)) {
        auto to_enqueue = std::array<lockfree::task<T>, 2>{
            lockfree::task<T>{.a = task.a,
                              .b = mid,
                              .func_val_a = task.func_val_a,
                              .func_val_b = func_val_mid,
                              .integral = integral_left},
            lockfree::task<T>{.a = mid,
                              .b = task.b,
                              .func_val_a = func_val_mid,
                              .func_val_b = task.func_val_b,
                              .integral = integral_right}};
        ranges::copy(to_enqueue, std::back_inserter(local_queue));
        return;
      }

      local_accumulator += new_integral;
    };

    while (local_queue.size() <= local_queue_max_size) {
      if (local_queue.empty()) {
        accumulated_integral += local_accumulator;
        return;
      }

      auto item = std::move(local_queue.back());
      local_queue.pop_back();
      local_handler(item);
    }

    global_queue.enqueue_bulk(local_queue.begin(), local_queue.size());
    accumulated_integral += local_accumulator;
  };

  {
    auto func_val_a = function(a);
    auto func_val_b = function(b);
    global_queue.enqueue(lockfree::task<T>{
        .a = a,
        .b = b,
        .func_val_a = func_val_a,
        .func_val_b = func_val_b,
        .integral = std::midpoint(func_val_a, func_val_b) * (b - a)});
  }

  auto threads = std::vector<std::jthread>{};

  auto loop_function = [&] {
    auto current_item = task<T>{};
    auto currently_active = false;

    while (true) {
      auto found = global_queue.try_dequeue(current_item);

      if (!found) {
        if (currently_active) {
          currently_active = false;
          --active_count;
          continue;
        }

        if (active_count == 0)
          break;

        continue;
      }

      assert(found);

      if (!currently_active) {
        currently_active = true;
        ++active_count;
      }

      global_handler(current_item);
    }
  };

  for ([[maybe_unused]] auto _ :
       ranges::views::iota(uint32_t{1}, num_threads)) {
    threads.push_back(std::jthread{[&]() { loop_function(); }});
  }

  loop_function();
  return accumulated_integral.load();
}

} // namespace lockfree

} // namespace

auto main(int argc, char **argv) -> int {
  auto desc = po::options_description{"allowed options"};

  using floating_type = double;
  desc.add_options()("help,h", "produce this help message")(
      "num-threads,t", po::value<uint32_t>()->default_value(2),
      "number of threads to use")(
      "a,a", po::value<floating_type>()->default_value(0.5e-3),
      "start of the integration interval")(
      "b,b", po::value<floating_type>()->default_value(1.0),
      "end of the integration interval")(
      "eps,e", po::value<floating_type>()->default_value(1e-7),
      "precision to signal the end of integration");

  auto vm = po::variables_map{};
  po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return EXIT_FAILURE;
  }

  auto num_threads = vm.at("num-threads").as<uint32_t>();
  auto a = vm.at("a").as<floating_type>();
  auto b = vm.at("b").as<floating_type>();
  auto eps = vm.at("eps").as<floating_type>();

  auto function = [](auto x) { return std::cos(decltype(x){1} / x); };

  fmt::println("{}",
               lockfree::parallel_integrate(a, b, function, num_threads, eps));
}
