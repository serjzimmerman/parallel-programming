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

#include <boost/format.hpp>
#include <boost/mpi.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/vector.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <range/v3/all.hpp>

#include <chrono>
#include <cmath>
#include <concepts>
#include <iostream>
#include <mdspan>
#include <numbers>

#if 0
#define DEBUG_PRINTS
#endif

namespace mpi = boost::mpi;
namespace po = boost::program_options;

namespace {

constexpr auto root_rank = 0;

auto get_num_time_points = [](auto mdspan) { return mdspan.extent(0); };

auto get_num_x_points = [](auto mdspan) { return mdspan.extent(1); };

template <typename T> auto linspace(T a, T b, std::size_t n) -> std::vector<T> {
  if (n < 2)
    return std::vector<T>(1, b);
  auto x = std::vector<T>(n);
  --n;

  auto c = (b - a) * (n - 1);
  if (std::isinf(c) && std::isinf(b - a)) {
    auto an = a / n;
    auto bn = b / n;
    for (auto i = std::size_t{0}; i <= n; ++i)
      x[i] = a + bn * i - an * i;
  } else {
    auto step = (b - a) / n;
    for (auto i = std::size_t{0}; i <= n; ++i)
      x[i] = a + step * i;
  }

  x[0] = a;
  x[n] = b;

  return x;
}

template <std::floating_point T>
auto solve_transfer_equation_impl(const mpi::communicator &world, auto data,
                                  std::span<T> boundary_value, T t_step,
                                  T x_step) {
  auto x_dim = get_num_x_points(data);

  constexpr auto tag = 0;
  for (auto i :
       ranges::views::iota(std::size_t{0}, get_num_time_points(data) - 1)) {
    auto send_data_to_next = !(world.rank() == world.size() - 1);

    if (send_data_to_next) {
      assert(world.rank() + 1 < world.size());
      world.send(world.rank() + 1, tag, data[i, x_dim - 1]);
    }

    for (auto j : ranges::views::iota(std::size_t{0}, x_dim)) {
      auto receive = [&]() {
        assert(world.rank() >= 1);
        auto result = T{};
        world.recv(world.rank() - 1, tag, result);
#ifdef DEBUG_PRINTS
        fmt::println("rank: {}, received from: {}, value: {}", world.rank(),
                     world.rank() - 1, result);
#endif
        return result;
      };

      auto neg = [&]() {
        if (j != 0)
          return data[i, j - 1];
        if (world.rank() != 0)
          return receive();
        return boundary_value[i];
      }();

      auto pos = data[i, j];
      data[i + 1, j] = pos + 0.0 * t_step - (pos - neg) * t_step / x_step;
    }
  }
}

template <typename T>
using column_major_mdspan =
    std::mdspan<T, std::dextents<std::size_t, 2>, std::layout_left>;

template <typename T> struct solve_result {
  column_major_mdspan<T> mdspan;
  std::vector<T> data;
};

template <std::floating_point T>
auto solve_transfer_equation(const mpi::communicator &world,
                             auto initial_condition, auto boundary_value, T a,
                             T b, T time, T t_step, T x_step, bool dont_collect)
    -> solve_result<T> {
  auto x_dim = static_cast<std::size_t>((b - a) / x_step) + 1;
  auto t_dim = static_cast<std::size_t>(time / t_step) + 1;

  auto xs = linspace(a, b, x_dim);
  auto ts = linspace(T{0}, time, t_dim);

  auto min_per_process = 1;
  auto per_process =
      std::max<std::size_t>(x_dim / world.size(), min_per_process);

  auto num_for_this_process = [&]() {
    auto rank = world.rank();
    if (rank == world.size() - 1)
      return x_dim - per_process * rank;
    if (per_process * rank > x_dim)
      return std::size_t{0};
    if (per_process * (rank + 1) > x_dim)
      return x_dim - per_process * rank;
    return per_process;
  }();

#ifdef DEBUG_PRINTS
  fmt::println("rank: {}, num_for_this_process: {}", world.rank(),
               num_for_this_process);
#endif

  auto data_for_process = std::vector<T>(num_for_this_process * t_dim);

  auto mdspan = column_major_mdspan<T>(data_for_process.data(), t_dim,
                                       num_for_this_process);
  {
    auto initial_values =
        ranges::views::transform(xs, initial_condition) | ranges::to_vector;
    auto starting_index = world.rank() * per_process;
    for (auto x_index :
         ranges::views::iota(std::size_t{0}, num_for_this_process)) {
      mdspan[0, x_index] = initial_values[starting_index + x_index];
    }
  }

  {
    auto boundary_values =
        ranges::views::transform(ts, boundary_value) | ranges::to_vector;
    solve_transfer_equation_impl(world, mdspan, std::span{boundary_values},
                                 t_step, x_step);
  }

  if (dont_collect)
    return {};

  auto gathered = std::vector<std::vector<T>>{};
  mpi::gather(world, data_for_process, gathered, root_rank);

  if (world.rank() != root_rank)
    return {};

#ifdef DEBUG_PRINTS
  fmt::println("rank: {}, gathered from number of processes: {}", world.rank(),
               gathered.size());
  for (auto &&[rank, received] : ranges::views::enumerate(gathered)) {
    fmt::println("from rank: {}, data: {}", rank, received);
  }
#endif
  auto final = std::vector<T>();

  for (auto &&vals : gathered)
    ranges::copy(vals, std::back_inserter(final));

  assert(final.size() == t_dim * x_dim);

  return solve_result<T>{
      .mdspan = column_major_mdspan<T>(final.data(), t_dim, x_dim),
      .data = std::move(final),
  };
}

} // namespace

auto main(int argc, char **argv) -> int {
  auto env = mpi::environment{argc, argv};
  const auto world = mpi::communicator{};

  auto desc = po::options_description{"allowed options"};

  auto default_num_points = 16;
  desc.add_options()("help", "produce this help message")(
      "a", po::value<double>()->default_value(0.0), "lower bound for x")(
      "b", po::value<double>()->default_value(std::numbers::pi),
      "upper bound for x")("h", po::value<double>()->default_value(
                                    std::numbers::pi / default_num_points))(
      "t", po::value<double>()->default_value(1.0), "upper bound for time")(
      "tau", po::value<double>()->default_value(0.25),
      "time value step")("samples", po::value<uint32_t>()->default_value(16))(
      "measure", "measure performance")("verbose", "enable verbose output");

  auto vm = po::variables_map{};
  po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return EXIT_FAILURE;
  }

  auto b = vm.at("b").as<double>();
  auto a = vm.at("a").as<double>();
  auto h = vm.at("h").as<double>();
  auto tau = vm.at("tau").as<double>();
  auto t = vm.at("t").as<double>();

  auto solve_function = [&](bool dont_collect) {
    return solve_transfer_equation(
        world, [](auto x) { return std::sin(x); },
        [](auto t) { return std::sin(t); }, a, b, t, tau, h, dont_collect);
  };

  auto measure_time =
      [&](auto callable) -> std::chrono::duration<double, std::milli> {
    auto run_once = [&] {
      auto begin_time = std::chrono::high_resolution_clock::now();
      callable(true);
      auto end_time = std::chrono::high_resolution_clock::now();
      return std::chrono::duration<double>{end_time - begin_time}.count();
    };

    auto num_samples = vm.at("samples").as<uint32_t>();
    auto times = ranges::views::generate_n(run_once, num_samples);

    return std::chrono::duration<double>{ranges::accumulate(times, 0.0) /
                                         num_samples};
  };

#ifdef DEBUG_PRINTS
  fmt::println("data.size(): {}, mdspan.extent(0): {}, mdspan.extent(1): {}",
               data.size(), mdspan.extent(0), mdspan.extent(1));
  fmt::println("raw data: {}", data);
#endif

  if (vm.count("measure")) {
    auto duration = measure_time(solve_function);
    if (world.rank() != root_rank)
      return EXIT_SUCCESS;
    if (vm.count("verbose"))
      fmt::println("solving the pde took {} ms", duration.count());
    else
      fmt::println("{}", duration.count());
    return EXIT_SUCCESS;
  }

  auto [mdspan, data] = solve_function(false);
  auto t_dim = get_num_time_points(mdspan);
  auto x_dim = get_num_x_points(mdspan);

  for ([[maybe_unused]] auto i : ranges::views::iota(std::size_t{0}, t_dim)) {
    auto time_slice =
        ranges::views::iota(std::size_t{0}, x_dim) |
        ranges::views::transform([&](auto j) { return mdspan[i, j]; });
    auto formatted = fmt::format("{}", fmt::join(time_slice, ", "));
    fmt::println("{}", formatted);
  }

  if (world.rank() != root_rank)
    return EXIT_SUCCESS;

  if (world.rank() != root_rank)
    return EXIT_SUCCESS;

  return EXIT_SUCCESS;
}
