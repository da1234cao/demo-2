#include <iostream>

template<typename T>
using call_back = void (*) (T);

void print_int(int num) {std::cout << num << std::endl;}

int main(int argc, char* argv[]){
    call_back<int> func = print_int;
    func(233);
    return 0;
}