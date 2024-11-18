// Y
// 55
// 5050
// 500500
// 50005000

extern void print_int(int X);

int addNumbers(int n) {
  int result;
  result = 0;

  if (n != 0) {
    result = n + addNumbers(n - 1);
  } else {
    result = n;
  }
  return result;
}

int recursion_driver(int num) { return addNumbers(num); }

void runner(void) {
  print_int(recursion_driver(10));
  print_int(recursion_driver(100));
  print_int(recursion_driver(1000));
  print_int(recursion_driver(10000));
}
