#include <iostream>

int foo(float *f, int *i) {
  *i = 1;
  *f = 0.f;

  return *i;
}

int main() {
  int x = 0;
  std::cout << x << std::endl; // Expect 0
  int x_ret = foo(reinterpret_cast<float *>(&x), &x);
  std::cout << x_ret << "\n";  // Expect 0?
  std::cout << x << std::endl; // Expect 0?
}