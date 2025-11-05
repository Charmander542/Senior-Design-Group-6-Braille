#ifndef BRAILLE_H
#define BRAILLE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Convert a single character; updates *pos and returns new pos or -1 on error
int encode_character(char ch, uint16_t *braille, int *pos);

// Convert a NUL-terminated C string into braille cells.
// Returns the number of cells written (<= max_out). Does not NUL-terminate.
size_t encode_string(const char *s, uint16_t *out, size_t max_out);

// Convert an array of strings. out is a flat buffer; offsets[i] gives the
// starting index in out for strings[i]; lengths[i] is cells written for strings[i].
// Returns total cells written across all strings.
size_t encode_strings(
    const char *const *strings, size_t n_strings,
    uint16_t *out, size_t max_out,
    size_t *offsets, size_t *lengths
);

#ifdef __cplusplus
}
#endif
#endif
