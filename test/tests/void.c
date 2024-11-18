// Y
// 0
// 1
// 2
// 3
// 4
// 5
// 6
// 7
// 8
// 9
// 10

extern void print_int(int X);

void Void(void) {
  int result;
  result = 0;
  print_int(result);
  while (result < 10) {
    result = result + 1;
    print_int(result);
  }

  return;
}

void runner(void) { Void(); }