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

#include <boost/dynamic_bitset.hpp>
#include <boost/program_options.hpp>

#include <atomic>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <omp.h>

namespace po = boost::program_options;

auto easy_check_is_definitely_not_prime(uint64_t val) -> bool {
#define CHECK_CASE_(_divider)                                                  \
  if (val >= _divider * _divider && val % _divider == 0)                       \
    return true;

  CHECK_CASE_(2)
  CHECK_CASE_(3)
  CHECK_CASE_(5)
  CHECK_CASE_(7)
  CHECK_CASE_(11)
  CHECK_CASE_(13)

  return false;
}

// Don't use floating-point sqrt, since it's not accurate.
auto get_sqrt_upper_bound(uint64_t n) -> uint64_t {
  if (n < 2)
    throw std::invalid_argument{"number can't be smaller than 2"};
  auto msb_index = sizeof(uint64_t) * CHAR_BIT - std::countl_zero(n) - 1;
  auto upper_bound_pow_2 = (msb_index + 1);
  auto sqrt_pow_upper_bound = (upper_bound_pow_2 / 2) + (upper_bound_pow_2 % 2);
  return (uint64_t{1} << sqrt_pow_upper_bound);
}

// NOTE: Processing block-wise gives better cache locality.
void process_blockwise(auto &are_prime, uint64_t from, uint64_t to) {
  for (auto i = uint64_t{2}; i * i <= to; ++i) {
    if (easy_check_is_definitely_not_prime(i))
      continue;

    auto start_j = std::max(((from + i - 1) / i) * i, i * i);
    for (auto j = start_j; j <= to; j += i)
      are_prime[j] = false;
  }
}

auto find_primes_sequential(uint64_t n, uint64_t block_size) {
  auto are_prime = boost::dynamic_bitset(n);
  are_prime.set();

  for (auto i = uint64_t{2}; i < n; i += block_size)
    process_blockwise(are_prime, i, std::min(i + block_size, n - 1));

  return are_prime;
}

auto find_primes_naive(uint64_t n) {
  auto are_prime = boost::dynamic_bitset(n);
  are_prime.set();

  for (auto i = uint64_t{2}; i * i < n; i += 1) {
    if (easy_check_is_definitely_not_prime(i))
      continue;

    for (auto j = i * i; j < n; j += i)
      are_prime[j] = false;
  }

  return are_prime;
}

auto find_primes_parallel(uint64_t n, uint64_t block_size) {
  auto are_prime = std::vector<std::atomic<bool>>(n);

#pragma omp parallel for
  for (auto i = uint64_t{0}; i < are_prime.size(); ++i)
    are_prime[i].store(true);

#pragma omp parallel for schedule(dynamic)
  for (auto i = uint64_t{2}; i < n; i += block_size)
    process_blockwise(are_prime, i, std::min(i + block_size, n));

  return are_prime;
}

auto main(int argc, const char **argv) -> int {
  auto desc = po::options_description{"allowed options"};
  auto n = uint64_t{0};

  auto mode = std::string{};
  auto print = false;
  auto block_size = uint64_t{128 * 1024};

  desc.add_options()("help", "produce this help message")(
      "num,n", po::value(&n)->default_value(uint64_t{1} << 20),
      "max number to check")("mode",
                             po::value(&mode)->default_value("parallel"))(
      "print", po::bool_switch(&print)->default_value(false))(
      "block,s", po::value(&block_size)->default_value(128 * 1024),
      "size of the block to iterate over");

  auto vm = po::variables_map{};
  po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return EXIT_FAILURE;
  }

  po::notify(vm);

  auto do_print = [&](auto &&cont) {
    if (!print)
      return;
    for (auto i = uint64_t{2}; i < cont.size(); ++i) {
      if (cont[i])
        std::cout << i << "\n";
    }
  };

  if (mode == "parallel") {
    auto are_primes = find_primes_parallel(n, block_size);
    do_print(are_primes);
  } else if (mode == "seq") {
    auto are_primes = find_primes_sequential(n, block_size);
    do_print(are_primes);
  } else if (mode == "naive") {
    auto are_primes = find_primes_naive(n);
    do_print(are_primes);
  } else {
    auto ss = std::stringstream{};
    ss << "unknown mode " << std::quoted(mode);
    throw std::invalid_argument{std::move(ss).str()};
  }
}
