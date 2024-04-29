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

#include <boost/program_options.hpp>
#include <fmt/core.h>
#include <range/v3/all.hpp>

#include <future>
#include <iostream>

namespace po = boost::program_options;

auto main(int argc, char **argv) -> int {
  auto desc = po::options_description{"allowed options"};

  auto default_num = uint64_t{1} << 10;
  desc.add_options()("help,h", "produce this help message")(
      "num-threads,t", po::value<uint32_t>()->default_value(2),
      "number of threads to use")(
      "num,n", po::value<uint64_t>()->default_value(default_num),
      "number of elements from harmonic series to sum");

  auto vm = po::variables_map{};
  po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return EXIT_FAILURE;
  }

  auto num_threads = vm.at("num-threads").as<uint32_t>();
  auto count = vm.at("num").as<uint64_t>();
  auto results = std::vector<std::future<double>>{};
  auto per_thread = count / num_threads;

  auto add_task = [&results](auto begin, auto end) {
    results.push_back(std::async(std::launch::async, [begin, end] {
      auto inverted = ranges::views::iota(begin, end) |
                      ranges::views::transform([](auto value) {
                        return 1.0 / static_cast<double>(value);
                      });
      return ranges::accumulate(inverted, 0.0);
    }));
  };

  auto one_past_all = count + 1;

  for (auto i : ranges::views::iota(uint32_t{0}, num_threads)) {
    auto start = std::max<uint64_t>(i * per_thread, 1);

    if (i == num_threads - 1) {
      add_task(start, one_past_all);
      continue;
    }

    auto end = std::clamp<uint64_t>((i + 1) * per_thread, 1, one_past_all);
    add_task(start, end);
  }

  auto accumulated = ranges::accumulate(
      ranges::views::transform(results,
                               [](auto &&future) { return future.get(); }),
      0.0);

  fmt::println("{}", accumulated);
}
