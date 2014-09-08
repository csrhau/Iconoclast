#include <chrono>
#include <exception>
#include <iostream>
#include <thread>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>


void standard_candle(int size_, int interval_, int iterations_) {
  float *a = new float[size_ * size_];
  float *b = new float[size_ * size_];
  float *c = new float[size_ * size_];
  for (int iter = 0; iter < iterations_; ++iter) {
    std::cout << " iterate " << std::endl;
    //Initialize numbers to some arbitrary value
    for (int i = 0; i < size_ * size_; ++i) {
      a[i] = float(i) / (float(i)+1.0);
    }
    for (int i = 0; i < size_; ++i) {
      for (int j = 0; j < size_; ++j) {
        float acc = 0;
        for (int k = 0; k < size_; ++k) {
          acc += a[i * size_ + k] + b[k * size_ + j];
        }
        c[i*size_+j] = acc;
      }
    }
    std::cout << " sleep " << std::endl;
    if (interval_) { // Sometimes we don't actually want this
      std::this_thread::sleep_for(std::chrono::milliseconds(interval_));  
    }
    std::cout << "busy add wait " << std::endl;
    for (uint64_t count = 0; count < 1073741824; ++count) {}
    std::cout << "busy sub wait " << std::endl;
    for (uint64_t count = 1073741824; count > 0; --count) {}
    std::cout << "busy add mul wait " << std::endl;
    for (uint64_t count = 0; (count * 2) < 2147483648; ++count) {}
    std::cout << "busy add /sub wait" << std::endl;
    for(uint64_t count_up = 0, count_down = 2147483648; count_up!=count_down; ++count_up, --count_down) {}
  }
  delete [] a;
  delete [] b;
  delete [] c;
}


int main(int argc, char* argv[]) {
  // Parse arguments
  int size, interval, iterations;
  try {
    namespace po = boost::program_options;
    po::options_description desc("Allowed Options");
    desc.add_options()("help", "produce help message")
      ("size", po::value<int>(&size)->default_value(750), 
      "Size of square matrix to compute")
      ("interval", po::value<int>(&interval)->default_value(1000), 
      "sleep interval")
      ("iterations", po::value<int>(&iterations)->default_value(10), 
      "cycle iterations"); 
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    if (vm.count("help")) {
      std::cerr << "Usage:\n" << desc << std::endl;
      return 1;
    }
  } catch (std::exception& ex) {
    std::cerr << "Exception Caught:" << ex.what() << std::endl; 
    return 1;
  } catch (...) {
    std::cerr << "Unknown Error! Exiting." << std::endl; 
    return 1;
  }
  std::cout << "Standard Candle application.\n"
            << "\tMatrix size: " << size << "x" << size
            << "\tSleep interval: " << interval << "ms"
            << "\tIterations: " << iterations << std::endl;

  standard_candle(size, interval, iterations);
  return 0;
}
