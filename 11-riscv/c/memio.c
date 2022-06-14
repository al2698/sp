#include <stdio.h>

#define reg_t int

#define CLINT_MTIME 0x200BFF8

int main() {
  int *p = 0x1000000;
  *p = 999;

  reg_t *r = 0x20000000;
  *r = 30;

  *(reg_t*) CLINT_MTIME = 88;

  (*(reg_t*) CLINT_MTIME) ++;
  (*(reg_t*) CLINT_MTIME) += 10;
  ((reg_t) CLINT_MTIME) += 1;
  *(reg_t*) CLINT_MTIME += *(reg_t*) CLINT_MTIME + 10;
  *CLINT_MTIME += *CLINT_MTIME + 10;
}
