// Y
// 12
// 7
// 7
// 7
// 18
// 9

extern void print_int(int arg);

int addition(int n, int m) {
  int result;
  result = n + m;

  if (n == 4) {
    print_int(n + m);
  } else {
    print_int(n * m);
  }

  return result;
}

void runner(void) {
  print_int(addition(3, 4));
  print_int(addition(4, 3));
  print_int(addition(6, 3));
}