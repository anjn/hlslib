/// @author    Johannes de Fine Licht (johannes.definelicht@inf.ethz.ch)
/// @date      June 2017
/// @copyright This software is copyrighted under the BSD 3-Clause License. 

#include "AccumulateCommon.h"
#include "hlslib/SDAccel.h"
#include <random>
#ifdef HLSLIB_COMPILE_ACCUMULATE_FLOAT
#include "AccumulateFloat.h"
using Dist_t = std::uniform_real_distribution<Data_t>;
#elif defined(HLSLIB_COMPILE_ACCUMULATE_INT)
#include "AccumulateInt.h"
using Dist_t = std::uniform_int_distribution<Data_t>;
#else
#error "Must be compiled for either float or int"
#endif
#include <iostream>
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE(kKernelName, kKernelName) {

  std::cout << "Initializing OpenCL context..." << std::endl;
  hlslib::ocl::Context context;
  std::cout << "Done." << std::endl;

  std::cout << "Initializing device memory..." << std::flush;
  auto inputDevice = context.MakeBuffer<DataPack_t, hlslib::ocl::Access::read>(
      hlslib::ocl::MemoryBank::bank0, kSize * kIterations);
  auto outputDevice =
      context.MakeBuffer<DataPack_t, hlslib::ocl::Access::write>(
          hlslib::ocl::MemoryBank::bank0, kIterations);
  std::cout << " Done." << std::endl;
  
  std::cout << "Copying input to device..." << std::flush;
  std::vector<DataPack_t> inputHost(kSize * kIterations);
  std::random_device rd;
  std::default_random_engine re(rd());
  Dist_t dist(1, 10);
  for (int i = 0; i < kSize * kIterations; ++i) {
    for (int w = 0; w < kDataWidth; ++w) {
      inputHost[i][w] = dist(re);
    }
  }
  inputDevice.CopyFromHost(inputHost.data());
  std::cout << " Done." << std::endl;

  std::cout << "Creating kernel..." << std::flush;
  auto program = context.MakeProgram(kKernelFile);
  auto kernel = program.MakeKernel(kKernelName, inputDevice, outputDevice);
  std::cout << " Done." << std::endl;

  std::cout << "Executing kernel..." << std::flush;
  auto elapsed = kernel.ExecuteTask();
  std::cout << " Done." << std::endl;

  std::cout << "Kernel ran in " << elapsed.first << " seconds.\n";

  std::cout << "Verifying result..." << std::flush;
  std::vector<DataPack_t> outputHost(kIterations);
  outputDevice.CopyToHost(outputHost.data());
  const auto reference =
      NaiveAccumulate<DataPack_t, Operator, kSize, kIterations>(inputHost);
  for (int i = 0; i < kIterations; ++i) {
    for (int w = 0; w < kDataWidth; ++w) {
      const auto diff = std::abs(outputHost[i][w] - reference[i][w]);
      REQUIRE(diff < 1e-3); 
    }
  }
  std::cout << " Done." << std::endl;

  std::cout << "Kernel ran successfully." << std::endl;

}
