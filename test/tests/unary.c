// Y
// 5
// -1
// 5
// -5

extern void print_float(float x);

float unary(int n, float m) {
  float result;
  float sum;

  sum = 0.0;

  result = n + m;
  print_float(result);
  sum = sum + result;

  result = n + -m;
  print_float(result);
  sum = sum + result;

  result = n + --m;
  print_float(result);
  sum = sum + result;

  result = -n + -m;
  print_float(result);
  sum = sum + result;

  return sum;
}

void runner(void) { unary(2, 3); }