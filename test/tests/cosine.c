// Y
// -1
// 0.5
// -0.5

extern void print_float(float x);

float cosine(float x) {
  float cos;
  float n;
  float term;
  float eps;
  float alt;

  eps = 0.000001;
  n = 1.0;
  cos = 1.0;
  term = 1.0;
  alt = -1.0;

  while (term > eps) {
    term = term * x * x / n / (n + 1);
    cos = cos + alt * term;
    alt = -alt;
    n = n + 2;
  }
  return cos;
}

void runner(void) {
  float pi;

  pi = 3.1415926535;
  print_float(cosine(pi));
  print_float(cosine(pi / 3));
  print_float(cosine(2 * pi / 3));
}