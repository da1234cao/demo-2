#include "loki/ScopeGuard.h"
#include <iostream>

int *foo() {
  int *i = new int{10};
  Loki::ScopeGuard free_i = Loki::MakeGuard([&i]() {
    delete i;
    i = 0;
    std::cout << "free i" << std::endl;
  });

  std::cout << *i << '\n';
  //   free_i.Dismiss();

  return i;
}

int main() {
  int *j = foo();
  std::cout << j << '\n';
}