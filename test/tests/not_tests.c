// Y
// 1
// 1
// 1
// 3

extern void print_int(int x);

void runner(void) {
  int x;
  float y;
  bool z;

  x = 0;
  y = 0;
  z = false;

  if (!x) {
    print_int(1);
  }

  if (!y) {
    print_int(1);
  }

  if (!z) {
    print_int(1);
  }

  x = 1;
  y = 1;
  z = true;

  if (!x) {
    print_int(2);
  }

  if (!y) {
    print_int(2);
  }

  if (!z) {
    print_int(2);
  }

  if (!0) {
    print_int(3);
  }
}