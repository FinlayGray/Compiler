// Y
// 0
// 1
// 1
// 2
// 3
// 5
// 8
// 13
// 21
// 34
// 55
// 6765

extern void print_int(int x);

int fibonacci(int n) {
  int first;
  int second;
  int next;
  int c;
  int total;

  if (n == 0) {
    return 0;
  }
  n = n - 1;

  first = 0;
  second = 1;
  c = 1;
  total = 0;

  while (c < n) {
    if (c <= 1) {
      next = c;
    } else {
      next = first + second;
      first = second;
      second = next;
    }
    c = c + 1;
    total = total + next;
  }

  return total + 1;
}

void runner(void) {
  int i;

  i = -1;

  while ((i = i + 1) <= 10) print_int(fibonacci(i));

  print_int(fibonacci(20));
}