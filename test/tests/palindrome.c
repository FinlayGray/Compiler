// Y
// true
// true
// false

extern void print_bool(bool x);

bool palindrome(int number) {
  int t;
  int rev;
  int rmndr;
  bool result;

  rev = 0;
  result = false;

  t = number;

  while (number > 0) {
    rmndr = number % 10;
    rev = rev * 10 + rmndr;
    number = number / 10;
  }

  if (t == rev) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

void runner(void) {
  print_bool(palindrome(45677654));
  print_bool(palindrome(12321));
  print_bool(palindrome(23423423));
}