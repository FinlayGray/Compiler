// Y
// 3628800
// 39916800
// 479001600
// 1932053504

extern void print_int(int x);

int factorial(int n) {
  int i;
  int factorial;

  factorial = 1;
  i = 1;

  while (i <= n) {
    factorial = factorial * i;  // factorial = factorial*i;
    i = i + 1;
  }

  return factorial;
}

void runner(void) {
  print_int(factorial(10));
  print_int(factorial(11));
  print_int(factorial(12));
  print_int(factorial(13));
}
