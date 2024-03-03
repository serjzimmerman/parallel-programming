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

#include "popl.hpp"

#include <mpi.h>

#include <chrono>
#include <climits>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

class mpi_initizlier {
public:
  mpi_initizlier(int argc, char **argv) { MPI_Init(&argc, &argv); }
  ~mpi_initizlier() { MPI_Finalize(); }
};

constexpr auto expected_size = 2;
constexpr auto sender_rank = 0;
constexpr auto receiver_rank = 1;
constexpr auto tag = 0;

enum class send_type {
  send,  //! https://rookiehpc.org/mpi/docs/mpi_send/index.html
  ssend, //! https://rookiehpc.org/mpi/docs/mpi_ssend/index.html
  rsend, //! https://rookiehpc.org/mpi/docs/mpi_rsend/index.html
  bsend, //! https://rookiehpc.org/mpi/docs/mpi_bsend/index.html
};

auto get_comm_size(MPI_Comm communicator) -> int {
  auto size = int{};
  MPI_Comm_size(communicator, &size);
  return size;
}

auto get_pid(MPI_Comm communicator) -> int {
  auto pid = int{};
  MPI_Comm_rank(communicator, &pid);
  return pid;
}

auto run_sender(send_type type, uint64_t num_bytes)
    -> std::chrono::duration<double, std::micro> {
  auto function = [type]() -> decltype(&MPI_Send) {
    switch (type) {
    case send_type::send:
      return MPI_Send;
      break;
    case send_type::ssend:
      return MPI_Ssend;
      break;
    case send_type::rsend:
      return MPI_Rsend;
      break;
    case send_type::bsend:
      return MPI_Bsend;
      break;
    }
    throw std::logic_error{"unreachable"};
  }();

  auto value_to_fill_with = std::numeric_limits<uint8_t>::max();
  auto values_to_send = std::vector<uint8_t>(num_bytes, value_to_fill_with);

  auto buffer = std::vector<uint8_t>(num_bytes << 2, 0);
  MPI_Buffer_attach(buffer.data(), buffer.size());

  auto begin = std::chrono::high_resolution_clock::now();
  function(values_to_send.data(), values_to_send.size(), MPI_CHAR,
           receiver_rank, tag /* some tag */, MPI_COMM_WORLD);
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = end - begin;

  {
    auto dummy_size = 0;
    MPI_Buffer_detach(buffer.data(), &dummy_size);
  }

  return duration;
}

void run_receiver(uint64_t num_bytes) {
  auto sleep_period = std::chrono::milliseconds{2000};
  std::this_thread::sleep_for(sleep_period);

  auto values_to_receive = std::vector<uint8_t>(num_bytes, 0);
  MPI_Recv(values_to_receive.data(), num_bytes, MPI_CHAR, sender_rank, tag,
           MPI_COMM_WORLD, nullptr);
}

} // namespace

auto main(int argc, char **argv) -> int {
  auto mpi = mpi_initizlier{argc, argv};

  auto op = popl::OptionParser{"available options"};
  auto help_option =
      op.add<popl::Switch>("h", "help", "Print this help message");

  auto verbose_option =
      op.add<popl::Switch>("v", "verbose", "Verbose messages");

  auto num_bytes = op.add<popl::Implicit<uint64_t>>(
      "b", "bytes", "Number of bytes to send", 1024);

  auto send_type_op = op.add<popl::Value<std::string>>(
      "t", "type", "Either <send>, <ssend>, <rsend> or <bsend>");

  op.parse(argc, argv);

  auto send_type = [&send_type_op]() -> ::send_type {
    if (send_type_op->value() == "send")
      return send_type::send;
    if (send_type_op->value() == "ssend")
      return send_type::ssend;
    if (send_type_op->value() == "rsend")
      return send_type::rsend;
    if (send_type_op->value() == "bsend")
      return send_type::bsend;
    throw std::invalid_argument{"invalid type option passed"};
  }();

  if (help_option->is_set()) {
    std::cout << op << "\n ";
    return EXIT_SUCCESS;
  }

  auto size = get_comm_size(MPI_COMM_WORLD);
  auto pid = get_pid(MPI_COMM_WORLD);

  if (size != expected_size)
    throw std::runtime_error{"this program needs to run on two nodes"};

  MPI_Barrier(MPI_COMM_WORLD);

  if (pid == sender_rank) {
    auto duration = run_sender(send_type, num_bytes->value());
    if (verbose_option->is_set()) {
      std::cout << "sent " << num_bytes->value() << " bytes with <"
                << send_type_op->value() << ">\n"
                << "wall time: " << duration.count() << std::fixed
                << " microseconds\n";
    } else {
      std::cout << num_bytes->value() << " " << std::fixed << duration.count()
                << "\n";
    }
  } else {
    run_receiver(num_bytes->value());
  }
}
