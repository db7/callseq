Experimental callback sequencer for CHICKEN Scheme

Install the package

    chicken-install callseq

Then define the functions you want to make external:

```
(import (chicken platform)
        callseq)

(define-sequenced-callback (foo (int a)) int
                           (* a a))
(return-to-host)
```

We expect CHICKEN to be embedded into some C program, so you have to return to
host after loading the module.

CHICKEN is initialized via `callseq_init()`, after that you can call
the external function:

```
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
```

See `example/` directory and `bench/` directory.

# License

Files in vendor/ contain license information in each file. The remainder of this
repository is distributed under 0-clause BSD license (see LICENSE file).
