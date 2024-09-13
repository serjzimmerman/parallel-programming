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

#include <atomic>
#include <iostream>
#include <omp.h>

auto main() -> int {
  auto shared_var = std::atomic<int>(0);
  auto current_id = std::atomic<int>(0);

#pragma omp parallel for shared(shared_var, current_id, std::cout)
  for (auto i = 0; i < omp_get_num_threads(); ++i) {
    auto tid = omp_get_thread_num();

    // Busy waiting. Ugly spinlock goes brrrrr
    while (current_id != tid) {
    }

    ++shared_var;

    std::cout << "thread id: " << tid << ", var: " << shared_var << std::endl;

    ++current_id;
  }
}
