// Y
// 10
// 5
// 20
// 13

extern void print_int(int X);

int test;
float f;
bool b;

int While(int n) {
  int result;
  test = 11 + 1;
  result = 0;
  while (result < n) result = result + 1;

  return result;
}

void runner(void) {
  print_int(While(10));
  print_int(While(5));
  print_int(While(20));
  print_int(While(13));
}