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

#include <barrier>
#include <iostream>
#include <thread>

namespace po = boost::program_options;

auto main(int argc, char **argv) -> int {
  auto desc = po::options_description{"allowed options"};

  desc.add_options()("help,h", "produce this help message")(
      "num-threads,n", po::value<uint32_t>()->default_value(8),
      "number of threads to use");

  auto vm = po::variables_map{};
  po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return EXIT_FAILURE;
  }

  auto thread_id = uint32_t{0};
  auto count = vm.at("num-threads").as<uint32_t>();
  auto barrier = std::barrier{count};

  auto create_handler = [&](auto i) {
    return [i, &barrier] {
      barrier.arrive_and_wait();
      fmt::println("Hello, threads. id = {}", i);
    };
  };

  auto threads = std::vector<std::jthread>{};
  for (auto i : ranges::views::iota(thread_id + 1, count)) {
    threads.push_back(std::jthread{create_handler(i)});
  }
  create_handler(thread_id)();
}
