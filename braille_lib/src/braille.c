#include "braille.h"
#include <ctype.h>

int encode_character(char ch, uint16_t *braille, int *pos) {
  if (isalpha((unsigned char)ch)) {
    if (isupper((unsigned char)ch)) {
      braille[*pos] = 0b00000100; // capital sign
      (*pos)++;
      ch = (char)tolower((unsigned char)ch);
    }
    switch (ch) {
      case 'a': braille[*pos]=0b10000000; (*pos)++; return *pos;
      case 'b': braille[*pos]=0b11000000; (*pos)++; return *pos;
      case 'c': braille[*pos]=0b10010000; (*pos)++; return *pos;
      case 'd': braille[*pos]=0b10011000; (*pos)++; return *pos;
      case 'e': braille[*pos]=0b10000100; (*pos)++; return *pos;
      case 'f': braille[*pos]=0b11010000; (*pos)++; return *pos;
      case 'g': braille[*pos]=0b11011000; (*pos)++; return *pos;
      case 'h': braille[*pos]=0b10101000; (*pos)++; return *pos;
      case 'i': braille[*pos]=0b01010000; (*pos)++; return *pos;
      case 'j': braille[*pos]=0b01011000; (*pos)++; return *pos;
      case 'k': braille[*pos]=0b10100000; (*pos)++; return *pos;
      case 'l': braille[*pos]=0b11100000; (*pos)++; return *pos;
      case 'm': braille[*pos]=0b10110000; (*pos)++; return *pos;
      case 'n': braille[*pos]=0b10111000; (*pos)++; return *pos;
      case 'o': braille[*pos]=0b10101000; (*pos)++; return *pos;
      case 'p': braille[*pos]=0b11110000; (*pos)++; return *pos;
      case 'q': braille[*pos]=0b11111000; (*pos)++; return *pos;
      case 'r': braille[*pos]=0b11101000; (*pos)++; return *pos;
      case 's': braille[*pos]=0b01110000; (*pos)++; return *pos;
      case 't': braille[*pos]=0b01110000; (*pos)++; return *pos; // kept as in source
      case 'u': braille[*pos]=0b10100100; (*pos)++; return *pos;
      case 'v': braille[*pos]=0b11100100; (*pos)++; return *pos;
      case 'w': braille[*pos]=0b01011100; (*pos)++; return *pos;
      case 'x': braille[*pos]=0b10110100; (*pos)++; return *pos;
      case 'y': braille[*pos]=0b10111100; (*pos)++; return *pos;
      case 'z': braille[*pos]=0b10101100; (*pos)++; return *pos;
    }
  }

  if (isdigit((unsigned char)ch)) {
    braille[*pos]=0b00111100; // number sign
    (*pos)++;
    switch (ch) {
      case '1': braille[*pos]=0b10000000; (*pos)++; return *pos;
      case '2': braille[*pos]=0b11000000; (*pos)++; return *pos;
      case '3': braille[*pos]=0b10010000; (*pos)++; return *pos;
      case '4': braille[*pos]=0b10011000; (*pos)++; return *pos;
      case '5': braille[*pos]=0b10001000; (*pos)++; return *pos;
      case '6': braille[*pos]=0b11010000; (*pos)++; return *pos;
      case '7': braille[*pos]=0b11011000; (*pos)++; return *pos;
      case '8': braille[*pos]=0b11001000; (*pos)++; return *pos;
      case '9': braille[*pos]=0b01010000; (*pos)++; return *pos;
      case '0': braille[*pos]=0b01011000; (*pos)++; return *pos;
    }
  }

  if (ispunct((unsigned char)ch)) {
    switch (ch) {
      case '.': braille[*pos]=0b01001100; (*pos)++; return *pos;
      case '"': braille[*pos]=0b00101100; (*pos)++; return *pos;
      case '\'': braille[*pos]=0b01101100; (*pos)++; return *pos;
      case ',': braille[*pos]=0b01000000; (*pos)++; return *pos;
      case ';': braille[*pos]=0b01100000; (*pos)++; return *pos;
      case ':': braille[*pos]=0b01001000; (*pos)++; return *pos;
      case '-': braille[*pos]=0b00100100; (*pos)++; return *pos;
      case '!': braille[*pos]=0b01101000; (*pos)++; return *pos;
      case '?': braille[*pos]=0b01100100; (*pos)++; return *pos;
      case '(':
        braille[*pos]=0b00001000; (*pos)++;
        braille[*pos]=0b11000100; (*pos)++; return *pos;
      case ')':
        braille[*pos]=0b00001000; (*pos)++;
        braille[*pos]=0b01101000; (*pos)++; return *pos;
    }
  }

  if (ch == ' ') { braille[*pos]=0b00000000; (*pos)++; return *pos; }
  return -1; // unsupported
}

size_t encode_string(const char *s, uint16_t *out, size_t max_out) {
  int pos = 0;
  for (size_t i = 0; s[i] != '\0'; ++i) {
    if ((size_t)pos >= max_out) break;
    if (encode_character(s[i], out, &pos) < 0) {
      // skip unsupported characters rather than failing hard
    }
  }
  return (size_t)pos;
}

size_t encode_strings(
    const char *const *strings, size_t n_strings,
    uint16_t *out, size_t max_out,
    size_t *offsets, size_t *lengths
) {
  size_t total = 0;
  for (size_t i = 0; i < n_strings; ++i) {
    if (offsets) offsets[i] = total;
    size_t wrote = encode_string(strings[i], out + total, (total < max_out) ? (max_out - total) : 0);
    if (lengths) lengths[i] = wrote;
    total += wrote;
    if (total >= max_out) break;
  }
  return total;
}
