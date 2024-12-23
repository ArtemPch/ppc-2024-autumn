#include <gtest/gtest.h>

#include <boost/mpi/communicator.hpp>
#include <boost/mpi/environment.hpp>
#include <random>
#include <vector>

#include "mpi/petrov_a_Shell_sort/include/ops_mpi.hpp"

namespace petrov_a_Shell_sort_mpi {

std::vector<int> generate_random_vector(int n, int min_val = -100, int max_val = 100,
                                        unsigned seed = std::random_device{}()) {
  static std::mt19937 gen(seed);
  std::uniform_int_distribution<int> dist(min_val, max_val);

  std::vector<int> vec(n);
  std::generate(vec.begin(), vec.end(), [&]() { return dist(gen); });
  return vec;
}

void template_test(const std::vector<int>& input_data) {
  boost::mpi::communicator world;
  std::vector<int> data = input_data;
  std::vector<int> result_data;

  int vector_size = static_cast<int>(data.size());
  std::shared_ptr<ppc::core::TaskData> taskDataPar = std::make_shared<ppc::core::TaskData>();

  taskDataPar->inputs.emplace_back(reinterpret_cast<uint8_t*>(&vector_size));
  taskDataPar->inputs_count.emplace_back(1);

  if (world.rank() == 0) {
    taskDataPar->inputs.emplace_back(reinterpret_cast<uint8_t*>(data.data()));
    taskDataPar->inputs_count.emplace_back(data.size());

    result_data.resize(vector_size);
    taskDataPar->outputs.emplace_back(reinterpret_cast<uint8_t*>(result_data.data()));
    taskDataPar->outputs_count.emplace_back(result_data.size());
  }

  auto taskParallel = std::make_shared<TestTaskMPI>(taskDataPar);

  bool success = taskParallel->validation();
  boost::mpi::broadcast(world, success, 0);
  if (success) {
    taskParallel->pre_processing();
    taskParallel->run();
    taskParallel->post_processing();

    if (world.rank() == 0) {
      std::sort(data.begin(), data.end());
      EXPECT_EQ(data, result_data);
    }
  }
}

}  // namespace petrov_a_Shell_sort_mpi

TEST(petrov_a_Shell_sort_mpi, test_sorted_ascending) {
  petrov_a_Shell_sort_mpi::template_test({1, 2, 3, 4, 5, 6, 7, 8});
}

TEST(petrov_a_Shell_sort_mpi, test_sorted_descending) {
  petrov_a_Shell_sort_mpi::template_test({8, 7, 6, 5, 4, 3, 2, 1});
}

TEST(petrov_a_Shell_sort_mpi, test_random_data) {
  petrov_a_Shell_sort_mpi::template_test({3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 9});
}