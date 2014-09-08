#include <chrono>
#include <exception>
#include <iostream>
#include <sstream>
#include <cassert>

void print_usage(char* binary_) {
  std::cout << "Usage: " << binary_ << " <Size> <Iterations>" << std::endl;
}

void standard_candle(int size_, int iterations_) {
  float *a = new float[size_ * size_];
  float *b = new float[size_ * size_];
  float *c = new float[size_ * size_];
  for (int iter = 0; iter < iterations_; ++iter) {
    std::cout << "Iterate\n";
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
  }
  delete [] a;
  delete [] b;
  delete [] c;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    print_usage(argv[0]);
    return 0;
  }
  int size = atoi(argv[1]);
  int iterations = atoi(argv[2]);
  assert(size > 0 && iterations > 0);
  std::cout <<  " Performing " << size << "x" << size << " matmul for " 
            << iterations << " iterations." << std::endl;

  // Parse arguments
  standard_candle(size, iterations);
  return 0;
}
