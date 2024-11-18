// Y

int test1() { return 1; }

int test2() {
  { return 1; }
}

int test3() {
  if (true) {
    return 1;
  } else {
    return 1;
  }
}

int test4() {
  if (true) {
    return 1;
  } else {
  }

  return 1;
}

int test5() {
  {
    {
      {
        {
          {
            {
              {
                {
                  {
                    {
                      { return 1; }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

void runner(void) {}