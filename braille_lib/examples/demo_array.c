#include <stdio.h>
#include <stdint.h>
#include "braille.h"

int main(void) {
  const char *inputs[] = {
    "Hello, World!",
    "CS101",
    "data > code"
  };
  const size_t N = sizeof(inputs)/sizeof(inputs[0]);

  uint16_t buffer[1024];
  size_t offsets[N], lengths[N];
  size_t total = encode_strings(inputs, N, buffer, 1024, offsets, lengths);

  printf("Total cells: %zu\n", total);
  for (size_t i = 0; i < N; ++i) {
    printf("Input %zu: \"%s\" -> %zu cells at [%zu]\n", i, inputs[i], lengths[i], offsets[i]);
    for (size_t j = 0; j < lengths[i]; ++j) {
      printf("0x%04x ", (unsigned)buffer[offsets[i]+j]);
    }
    printf("\n");
  }
  return 0;
}
