// Y
// 3
// 1
// 3
// 1
// 4
// 1
// 3
// 0

extern void print_int(int x);

void runner(void) {
  int t;

  (t = 3) || (t = 4);
  print_int(t);

  if ((t = 3) || (t = 4)) {
    print_int(1);
  }
  print_int(t);

  if ((t = 3) && (t = 4)) {
    print_int(1);
  }
  print_int(t);

  if ((t = 0) || (t = 3)) {
    print_int(1);
  }
  print_int(t);

  if ((t = 0) && (t = 3)) {
    print_int(1);
  }
  print_int(t);
}