#include <chicken.h>
#include <stdio.h>

void callseq_init(void);
int foo(int a);

int main() {
  callseq_init();
  int x = 4;
  int y = foo(x);
  printf("(foo x) = (foo %d) = %d\n", x, y);
  return 0;
}
