// Y
// 0
// 1
// 2
// 3
// 4
// 5

extern void print_int(int x);

void runner(void) {
  int i;

  i = 0;

  while (i <= 5) {
    print_int(i % 6);
    i = i + 1;
  }
}