// Y
// 3

extern void print_int(int x);

void runner(void) {
  int t;
  t = 3;

  {
    int t;
    t = 4;
  }

  print_int(t);
}