// Y
// 0
// 2
// 1

extern void print_int(int x);

int num;

void runner(void) {
  print_int(num);

  num = 2;

  print_int(num);

  {
    int num;

    num = 1;
    print_int(num);
  }
}