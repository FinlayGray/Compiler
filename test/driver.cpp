#include <cstdio>
#include <iomanip>
#include <iostream>

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

extern "C" DLLEXPORT void print_int(int x) { std::cout << x << std::endl; }
extern "C" DLLEXPORT void print_float(float x) { std::cout << x << std::endl; }
extern "C" DLLEXPORT void print_bool(bool x) {
  std::cout << std::boolalpha << x << std::endl;
}

extern "C" {
void runner(void);
}

int main() { runner(); }