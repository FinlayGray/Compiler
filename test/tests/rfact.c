// Y
// 3628800
// 39916800
// 479001600
// 1932053504

extern void print_int(int x);

int multiplyNumbers(int n) {
  int result;
  result = 0;

  if (n >= 1) {
    result = n * multiplyNumbers(n - 1);
  } else {
    result = 1;
  }
  return result;
}

int rfact(int n) { return multiplyNumbers(n); }

void runner(void) {
  print_int(rfact(10));
  print_int(rfact(11));
  print_int(rfact(12));
  print_int(rfact(13));
}