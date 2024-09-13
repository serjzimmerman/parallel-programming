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

#include <iomanip>
#include <iostream>
#include <omp.h>

namespace po = boost::program_options;

auto main(int argc, const char **argv) -> int {
  auto desc = po::options_description{"allowed options"};
  auto n = uint64_t{0};

  desc.add_options()("help", "produce this help message")(
      "num,n", po::value(&n)->default_value(uint64_t{1} << 28),
      "number to sum");

  auto vm = po::variables_map{};
  po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return EXIT_FAILURE;
  }

  po::notify(vm);

  auto sum = double{};

#pragma omp parallel for reduction(+ : sum) schedule(static)
  for (auto i = uint64_t{1}; i <= n; ++i) {
    auto val = 1.0 / static_cast<double>(i);
    sum += val;
  }

  std::cout << std::setprecision(32) << sum << "\n";
}
