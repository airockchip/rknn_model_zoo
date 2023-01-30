/*
 * cnpy.h - a C-99 single-header library for reading and writing numpy .npy data files.
 *
 * Copyright (C) 2018, 2019 Lukas Himbert
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdbool.h> /* bool */
#include <limits.h> /* CHAR_BITS */
#include <stdint.h> /* exact width integers */
#include <fcntl.h> /* open, read */
#include <unistd.h> /* close, ftruncate */
#include <sys/mman.h> /* mmap, munmap */
#include <string.h> /* memcmp, strncmp */
#include <ctype.h> /* isdigit */
#include <assert.h> /* assert, static_assert */
#include <stdio.h> /* fprintf, stderr */
// #if __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS) && !defined(__clang__) /* TODO: check for clang version */
// #define CNPY_THREADSAFE
// #include <threads.h> /* thread_local */
// #endif
#include <errno.h> /* strerror, errno */
#include <stdarg.h> /* va_list, va_begin, va_end */
#include <math.h> /* log10 */
#include <complex.h> /* complex, creal, crealf, imag, imagf */


#if __STDC_VERSION__ >= 201112L
_Static_assert(CHAR_BIT == 8, "This code assumes 8-bit chars (bytes).");
_Static_assert(sizeof(float) == 4, "This code assumes that float == float32.");
_Static_assert(sizeof(double) == 8, "This code assumes that double == float64.");
_Static_assert(sizeof(size_t) >= 4, "This code assumes that size_t is at least 32 bit wide.");
#endif


#ifndef CNPY_MAX_DIM
#define CNPY_MAX_DIM 4 /* maximum number of dimensions; four ought to be enough for everybody... If not, just increase it with a -D flag. */
#endif


typedef enum {
  CNPY_LE, /* little endian (least significant byte to most significant byte) */
  CNPY_BE, /* big endian (most significant byte to least significant byte) */
  CNPY_NE, /* no / neutral endianness (each element is a single byte) */
  /* Host endianness is not supported because it is an incredibly bad idea to use it for storage. */
} cnpy_byte_order;


typedef enum {
  CNPY_B = 0, /* We want to use the values as index to the following arrays. */
  CNPY_I1,
  CNPY_I2,
  CNPY_I4,
  CNPY_I8,
  CNPY_U1,
  CNPY_U2,
  CNPY_U4,
  CNPY_U8,
  CNPY_F4,
  CNPY_F8,
  CNPY_C8,
  CNPY_C16,
} cnpy_dtype;


/* size of one unit of the datatypes in bytes */
static const uint8_t cnpy_dtype_sizes[] = {
  1,
  1,
  2,
  4,
  8,
  1,
  2,
  4,
  8,
  4,
  8,
  8,
  16,
};

#if __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(cnpy_dtype_sizes) / sizeof(cnpy_dtype_sizes[0]) == CNPY_C16+1, "cnpy_dtype_sizes and cnpy_dtype mismatch");
#endif


static const char * const cnpy_dtype_str[13] = {
  "b1",
  "i1",
  "i2",
  "i4",
  "i8",
  "u1",
  "u2",
  "u4",
  "u8",
  "f4",
  "f8",
  "c8",
  "c16",
};

#if __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(cnpy_dtype_str) / sizeof(cnpy_dtype_str[0]) == CNPY_C16+1, "cnpy_dtype_str and cnpy_dtype mismatch");
#endif


typedef enum {
  CNPY_C_ORDER, /* C order (row major) */
  CNPY_FORTRAN_ORDER, /* Fortran order (column major) */
} cnpy_flat_order;


typedef struct {
  cnpy_byte_order byte_order; /* byte order */
  cnpy_dtype dtype; /* type of stored data */
  cnpy_flat_order order; /* serialisation order */
  size_t n_dim; /* number of dimensions of the data */
  size_t dims[CNPY_MAX_DIM]; /* size of the array along each of the dimensions; having a static size is a bit wasteful, but it means that we do not need dynamic memory allocation. */
  char *raw_data; /* pointer to the raw data, including header. */
  size_t data_begin; /* offset where the actual data starts (first byte after header). */
  size_t raw_data_size; /* size of the whole data, including the full header */
} cnpy_array;


typedef enum {
  CNPY_SUCCESS, /* success */
  CNPY_ERROR_FILE, /* some error regarding handling of a file */
  CNPY_ERROR_MMAP, /* some error regarding mmaping a file */
  CNPY_ERROR_FORMAT, /* file format error while reading some file */
} cnpy_status;


/*
 * cnpy error handling
 */


#define CNPY_ERROR_STR_SIZE 256

// #ifdef CNPY_THREADSAFE
// thread_local
// #endif
char cnpy_error_str[CNPY_ERROR_STR_SIZE] = "cnpy successful";


static void cnpy_perror(char *str) {
  if (str != NULL && str[0] != '\0') {
    fprintf(stderr, "%s: ", str);
  }
  fprintf(stderr, "%s.\n", cnpy_error_str);
}


static cnpy_status cnpy_error(cnpy_status s, char *str, ...) {
  va_list args;
  va_start(args, str);
  vsnprintf(cnpy_error_str, CNPY_ERROR_STR_SIZE, str, args);
  va_end(args);
  return s;
}


static void cnpy_error_reset(void) {
  cnpy_error(CNPY_SUCCESS, "cnpy successful");
}


/*
 * Reader function
 */


/* Forward declaration of header parser */
static cnpy_status cnpy_parse(const char * const, size_t, cnpy_array*);


/*
 * Open an existing npy file.
 * Arguments:
 * fn - The name of the file.
 * writable - If true, then writing to the cnpy_array will cause the file to be changed.
 * arr - The resulting cnpy_array will be written to this address.
 * returns:
 * cnpy_status - CNPY_SUCCESS on success, something else on failure. In the case of a failure, *arr will not be changed.
 */
cnpy_status cnpy_open(const char * const fn, bool writable, cnpy_array *arr) {
  assert(arr != NULL);

  cnpy_array tmp_arr;

  /* open, mmap, and close the file */
  int fd = open(fn, writable? O_RDWR : O_RDONLY);
  if (fd == -1) {
    return cnpy_error(CNPY_ERROR_FILE, "Could not open file: %s", strerror(errno));
  }
  size_t raw_data_size = (size_t) lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  if (raw_data_size == 0) {
    close(fd); /* no point in checking for errors */
    return cnpy_error(CNPY_ERROR_FORMAT, "Empty file");
  }
  if (raw_data_size == SIZE_MAX) {
    /* This is just because the author is too lazy to check for overflow on every pos+1 calculation. */
    close(fd);
    return cnpy_error(CNPY_ERROR_FORMAT, "File size is SIZE_MAX = %zu, should be at least one byte smaller", SIZE_MAX);
  }

  void *raw_data = mmap(
    NULL, /* addr */
    raw_data_size, /* map size*/
    PROT_READ | PROT_WRITE, /* protection flags */
    writable? MAP_SHARED : MAP_PRIVATE, /* private/shared flag */
    fd, /* file */
    0 /* file offset */
  );

  if (raw_data == MAP_FAILED) {
    close(fd);
    return cnpy_error(CNPY_ERROR_MMAP, "mmap() of file failed: %s", strerror(errno));
  }

  /* It is ok to close the file; the file descriptor will be released once the raw_data is munmap()ed. */
  if (close(fd) != 0) {
    munmap(raw_data, raw_data_size);
    return cnpy_error(CNPY_ERROR_FILE, "Could not close file after mmap(): %s", strerror(errno));
  }

  /* parse the file */
  cnpy_status status = cnpy_parse(raw_data, raw_data_size, &tmp_arr);
  if (status != CNPY_SUCCESS) {
    munmap(raw_data, raw_data_size);
    return status;
  }
  *arr = tmp_arr;

  return CNPY_SUCCESS;
}


/*
 * Parsing the header.
 */


typedef struct {
  const char * const raw_data;
  const size_t raw_data_size;
  size_t pos;
  size_t full_header_size; /* size of the full header (including magic string and version). Also equal to the beginning of the actual data. */
  bool read_descr;
  cnpy_byte_order byte_order;
  cnpy_dtype dtype;
  bool read_fortran_order;
  cnpy_flat_order order;
  bool read_shape;
  size_t n_dim;
  size_t dims[CNPY_MAX_DIM];
} cnpy_parser_state;


/*
 * parser forward declarations and definitions
 */


static cnpy_status cnpy_parse_pre_header(cnpy_parser_state *);
static cnpy_status cnpy_parse_header(cnpy_parser_state *);
static void cnpy_parse_skip_whitespace(cnpy_parser_state *);
static cnpy_status cnpy_parse_key_value_pair(cnpy_parser_state *);
static cnpy_status cnpy_parse_value_descr(cnpy_parser_state *);
static cnpy_status cnpy_parse_value_shape(cnpy_parser_state *);
static cnpy_status cnpy_parse_value_fortran_order(cnpy_parser_state *);
static cnpy_status cnpy_parse_check_data_size(const cnpy_array);
static size_t cnpy_atonz(const char * const, size_t, size_t *);


static cnpy_status cnpy_parse(const char * const raw_data, size_t raw_data_size, cnpy_array *arr) {
  assert(raw_data != NULL);
  assert(arr != NULL);

  cnpy_parser_state s = {
    .raw_data = raw_data,
    .raw_data_size = raw_data_size,
    .pos = 0,
    .full_header_size = ~0,
    .read_descr = false,
    .read_fortran_order = false,
    .read_shape = false,
    .n_dim = 0,
  };
  cnpy_status status = CNPY_SUCCESS;

  if (status == CNPY_SUCCESS) {
    status = cnpy_parse_pre_header(&s);
  }
  if (status == CNPY_SUCCESS) {
    status = cnpy_parse_header(&s);
  }
  if (status == CNPY_SUCCESS) {
    assert(s.read_descr && s.read_fortran_order && s.read_shape);
    size_t size = 1;
    for (size_t i = 0; i < s.n_dim; i += 1) {
      if (__builtin_mul_overflow(size, s.dims[i], &size)) {
        return cnpy_error(CNPY_ERROR_FORMAT, "Size of data overflows");
      }
    }
    arr->byte_order = s.byte_order;
    arr->dtype = s.dtype;
    arr->order = s.order;
    arr->n_dim = s.n_dim;
    arr->raw_data = (void *) raw_data;
    arr->data_begin = s.full_header_size;
    arr->raw_data_size = raw_data_size;
    for (size_t i = 0; i < arr->n_dim; i += 1) {
      arr->dims[i] = s.dims[i];
    }
  }
  if (status == CNPY_SUCCESS) {
    status = cnpy_parse_check_data_size(*arr);
  }

  return status;
}


static cnpy_status cnpy_parse_pre_header(cnpy_parser_state *s) {
  if (s->raw_data_size < 16) {
    /* The header is aligned to 16 bytes, so a valid file is least this large. */
    return cnpy_error(CNPY_ERROR_FORMAT, "File is too short to contain a header.");
  }

  char magic_str[6] = { '\x93', 'N', 'U', 'M', 'P', 'Y' };
  const size_t magic_str_len = 6;
  if (memcmp(s->raw_data, magic_str, magic_str_len) != 0) {
    return cnpy_error(CNPY_ERROR_FORMAT, "File does not start with magic string");
  }
  s->pos += magic_str_len;

  uint8_t version_major = ((uint8_t *) s->raw_data)[s->pos];
  s->pos += 1;
  uint8_t version_minor = ((uint8_t *) s->raw_data)[s->pos];
  s->pos += 1;

  assert(s->pos == 8);

  /* read header size as little-endian unsigned ints */
  size_t header_size = 0;
  if (version_major == 1 && version_minor == 0) {
    header_size =
        (size_t) (((uint8_t *) s->raw_data)[8])
      | (size_t) (((uint8_t *) s->raw_data)[9]) << 8;
    s->pos = 10;
  }
  else if (version_major == 2 && version_minor == 0) {
    header_size =
        (size_t) (((uint8_t *) s->raw_data)[8])
      | (size_t) (((uint8_t *) s->raw_data)[9])  << 8
      | (size_t) (((uint8_t *) s->raw_data)[10]) << 16
      | (size_t) (((uint8_t *) s->raw_data)[11]) << 24;
    s->pos = 12;
  }
  else {
    return cnpy_error(CNPY_ERROR_FORMAT, "Unsupported file format version %u.%u", version_major, version_minor);
  }

  /* Compare header and file size */
  if (__builtin_add_overflow(s->pos, header_size, &(s->full_header_size))) {
    return cnpy_error(CNPY_ERROR_FORMAT, "Claimed header size overflows");
  }
  if (!(s->full_header_size <= s->raw_data_size)) { /* For size 0 arrays, it should be <=. */
    return cnpy_error(CNPY_ERROR_FORMAT, "Claimed header size %zu is larger than the file size %zu", s->full_header_size, s->raw_data_size);
  }

  /* Check alignment */
  if (s->full_header_size % 16 != 0) {
    return cnpy_error(CNPY_ERROR_FORMAT, "Data start %zu is not an integer multiple of 16", s->full_header_size);
  }

  return CNPY_SUCCESS;
}


static cnpy_status cnpy_parse_header(cnpy_parser_state *s) {
  assert(s != NULL);
  assert(s->raw_data != NULL);

  /* read opening bracket */
  cnpy_parse_skip_whitespace(s);
  if (s->pos < s->full_header_size && s->raw_data[s->pos] == '{') {
    s->pos += 1;
  }
  else {
    return cnpy_error(CNPY_ERROR_FORMAT, "Missing '{' while parsing header");
  }

  /* read up to three key-value pairs (there should be exactly three). */
  for (size_t i = 0; i < 3; i += 1) {
    cnpy_parse_skip_whitespace(s);
    cnpy_status status = cnpy_parse_key_value_pair(s);
    if (status != CNPY_SUCCESS) {
      return status;
    }

    /* we expect a comma between the pairs, and optionally after the last one. */
    cnpy_parse_skip_whitespace(s);
    if (s->pos < s->full_header_size && s->raw_data[s->pos] == ',') {
      s->pos += 1;
    }
    else if (i == 2) {
      /* It's ok, we don't need a comma after the last key value pair. */
    }
    else {
      return cnpy_error(CNPY_ERROR_FORMAT, "Missing ',' after %s key-value pair", (i == 0)? "first" : "second");
    }

  }

  assert(s->read_descr && s->read_fortran_order && s->read_shape);

  /* parse the closing '}'. */
  cnpy_parse_skip_whitespace(s);
  if (s->pos < s->full_header_size && s->raw_data[s->pos] == '}') {
    s->pos += 1;
  }

  /* parse padding spaces and a final '\n' */
  cnpy_parse_skip_whitespace(s);

  if (s->pos != s->full_header_size) {
    return cnpy_error(CNPY_ERROR_FORMAT, "junk after end of header dictionary at position %zu (reported end is %zu)", s->pos, s->full_header_size);
  }
  if (s->raw_data[s->full_header_size - 1] != '\n') {
    return cnpy_error(CNPY_ERROR_FORMAT, "Missing \\n at end of header");
  }

  return CNPY_SUCCESS;
}


static void cnpy_parse_skip_whitespace(cnpy_parser_state *s) {
  while(s->pos < s->full_header_size && isspace(s->raw_data[s->pos])) {
    s->pos += 1;
  }
}


static cnpy_status cnpy_parse_key_value_pair(cnpy_parser_state *s) {
  /* We expect a key, which starts with a string delimiter. */
  char c = '\0';
  if (s->pos < s->full_header_size) {
    c = s->raw_data[s->pos];
    s->pos += 1;
    if (c != '\'' && c != '"') {
      return cnpy_error(CNPY_ERROR_FORMAT, "Expected a string delimiter, got '%c'", c);
    }
  }
  else {
    return cnpy_error(CNPY_ERROR_FORMAT, "Expected a string delimiter, got nothing");
  }

  /*
   * Now we expect a string.
   * We first read until the next matching string delimiter.
   * Note that we do not need to deal with escaping or unicode, because all valid .npy-keys are non-escaped ASCII.
   */
  size_t key_start = s->pos;
  size_t key_end;
  while (s->pos < s->full_header_size && s->raw_data[s->pos] != c) {
    s->pos += 1;
  }
  if (s->pos < s->full_header_size) {
    key_end = s->pos - 1;
    s->pos += 1;
  }
  else {
    return cnpy_error(CNPY_ERROR_FORMAT, "Unterminated key-string");
  }

  /* We postpone the key, and first read a colon, which seperates key from value. */
  cnpy_parse_skip_whitespace(s);
  if (s->pos < s->full_header_size && s->raw_data[s->pos] == ':') {
    s->pos += 1;
  }
  else {
    return cnpy_error(CNPY_ERROR_FORMAT, "expected ':' after key");
  }
  cnpy_parse_skip_whitespace(s);

  /*
   * We are now ready to parse the value.
   * The key tells us which kind of value we should expect.
   */
  size_t key_len = (key_end - key_start) + 1;
  cnpy_status status;
  if (key_len == 5 && memcmp(s->raw_data + key_start, "descr", key_len) == 0) {
    if (s->read_descr) {
      return cnpy_error(CNPY_ERROR_FORMAT, "Key 'descr' appears twice");
    }
    status = cnpy_parse_value_descr(s);
    s->read_descr = true;
  }
  else if (key_len == 5 && memcmp(s->raw_data + key_start, "shape", key_len) == 0) {
    if (s->read_shape) {
      return cnpy_error(CNPY_ERROR_FORMAT, "Key 'shape' appears twice");
    }
    status = cnpy_parse_value_shape(s);
    s->read_shape = true;
  }
  else if (key_len == 13 && memcmp(s->raw_data + key_start, "fortran_order", key_len) == 0) {
    if (s->read_fortran_order) {
      return cnpy_error(CNPY_ERROR_FORMAT, "Key 'fortran_order' appears twice");
    }
    status = cnpy_parse_value_fortran_order(s);
    s->read_fortran_order = true;
  }
  else {
    return cnpy_error(CNPY_ERROR_FORMAT, "Unknown key in header");
  }

  return status;
}


static cnpy_status cnpy_parse_value_descr(cnpy_parser_state *s) {
  assert(!s->read_descr);

  char c_str = '\0';
  char c_endianness = '\0';
  char c_type = '\0';
  size_t n_bytes = 0;

  /* Read a string delimiter. */
  if (s->pos < s->full_header_size) {
    c_str = s->raw_data[s->pos];
    if (c_str == '\'' || c_str == '"') {
      s->pos += 1;
    }
    else {
      return cnpy_error(CNPY_ERROR_FORMAT, "Expected a string delimiter, got '%c'", c_str);
    }
  }
  else {
    return cnpy_error(CNPY_ERROR_FORMAT, "Expected a string delimiter, got nothing");
  }

  /* Read an endianness character. */
  if (s->pos < s->full_header_size) {
    c_endianness = s->raw_data[s->pos];
    s->pos += 1;
  }
  else {
    return cnpy_error(CNPY_ERROR_FORMAT, "Expected an endianness character, got nothing");
  }
  switch (c_endianness) {
    case '<':
      s->byte_order = CNPY_LE;
      break;
    case '>':
      s->byte_order = CNPY_BE;
      break;
    case '|':
      s->byte_order = CNPY_NE;
      break;
    case '=':
      /* host byte order is intentionally unsupported. */
      return cnpy_error(CNPY_ERROR_FORMAT, "Host byte order '=' is intentionally unsupported");
    default:
      return cnpy_error(CNPY_ERROR_FORMAT, "Invalid byte_order '%c'", c_endianness);
  }

  /* Read a datatype character (we check the result later). */
  if (s->pos < s->full_header_size) {
    c_type = s->raw_data[s->pos];
    s->pos += 1;
  }
  else {
    return cnpy_error(CNPY_ERROR_FORMAT, "Expected an datatype character, got nothing");
  }

  /* Read the number of bytes which the datatype occupies (we check the result later). */
  if (s->pos < s->full_header_size) {
    size_t read = cnpy_atonz(s->raw_data + s->pos, (size_t) (s->full_header_size - s->pos),  &n_bytes);
    if (read > 0) {
      s->pos += read;
    }
    else {
      return cnpy_error(CNPY_ERROR_FORMAT, "Expected a datatype width, got something else");
    }
  }
  else {
    return cnpy_error(CNPY_ERROR_FORMAT, "Expected a datatype width, got nothing");
  }

  /* Read the closing string delimiter. */
  if (s->pos < s->full_header_size) {
    if (s->raw_data[s->pos] == c_str) {
      s->pos += 1;
    }
    else {
      return cnpy_error(CNPY_ERROR_FORMAT, "Expected closing string delimiter '%c', got '%c'", c_str, s->raw_data[s->pos]);
    }
  }
  else {
    return cnpy_error(CNPY_ERROR_FORMAT, "Expected closing string delimiter '%c', got nothing", c_str);
  }

  /* Now we interpret the results. */
  if (n_bytes == 1) {
    s->byte_order = CNPY_NE; /* If the datatype is only one byte wide, we do not care about byte order. */
  }

  /* Now get the actual datatype */
  if (c_type == 'b') {
    if (n_bytes == 1) {
      s->dtype = CNPY_B;
    }
    else {
      return cnpy_error(CNPY_ERROR_FORMAT, "Unsupported byte width %zu for bool", n_bytes);
    }
  }
  else if (c_type == 'i') {
    if (n_bytes == 1) {
      s->dtype = CNPY_I1;
    }
    else if (n_bytes == 2) {
      s->dtype = CNPY_I2;
    }
    else if (n_bytes == 4) {
      s->dtype = CNPY_I4;
    }
    else if (n_bytes == 8) {
      s->dtype = CNPY_I8;
    }
    else {
      return cnpy_error(CNPY_ERROR_FORMAT, "Unsupported byte width %zu for ints", n_bytes);
    }
  }
  else if (c_type == 'u') {
    if (n_bytes == 1) {
      s->dtype = CNPY_U1;
    }
    else if (n_bytes == 2) {
      s->dtype = CNPY_U2;
    }
    else if (n_bytes == 4) {
      s->dtype = CNPY_U4;
    }
    else if (n_bytes == 8) {
      s->dtype = CNPY_U8;
    }
    else {
      return cnpy_error(CNPY_ERROR_FORMAT, "Unsupported byte width %zu for uints", n_bytes);
    }
  }
  else if (c_type == 'f') {
    if (n_bytes == 4) {
      s->dtype = CNPY_F4;
    }
    else if (n_bytes == 8) {
      s->dtype = CNPY_F8;
    }
    else {
      return cnpy_error(CNPY_ERROR_FORMAT, "Unsupported byte width %zu for floats", n_bytes);
    }
  }
  else if (c_type == 'c') {
    if (n_bytes == 8) {
      s->dtype = CNPY_C8;
    }
    else if (n_bytes == 16) {
      s->dtype = CNPY_C16;
    }
    else {
      return cnpy_error(CNPY_ERROR_FORMAT, "Unsupported byte width %zu for complex", n_bytes);
    }
  }
  else {
    return cnpy_error(CNPY_ERROR_FORMAT, "Unsupported datatype '%c'", c_type);
  }

  if (cnpy_dtype_sizes[s->dtype] > 1 && s->byte_order == CNPY_NE) {
    return cnpy_error(CNPY_ERROR_FORMAT, "dtype size > 1 but no endianness given");
  }

  return CNPY_SUCCESS;
}


static cnpy_status cnpy_parse_value_shape(cnpy_parser_state *s) {
  assert(!s->read_shape);

  /* Read an opening parenthesis. */
  char c = '\0';
  if (s->pos < s->full_header_size) {
    c = s->raw_data[s->pos];
    if (c == '(' || c == '[') {
      s->pos += 1;
    }
    else {
      return cnpy_error(CNPY_ERROR_FORMAT, "Expected opening parenthesis, got '%c'", c);
    }
  }
  else {
    return cnpy_error(CNPY_ERROR_FORMAT, "Expected opening paranthesis, got nothing");
  }

  /* read many numbers, followed by commas (the last one being optional) */
  s->n_dim = 0;
  bool read_comma = false;
  do {
    /* Read an unsigned int and extend the dimensions accordingly. */
    size_t read = cnpy_atonz(s->raw_data + s->pos, s->full_header_size - s->pos, s->dims + s->n_dim); /* No underflow because s->full_header_size >= s_pos. */
    if (read > 0) {
      s->pos += read;
      s->n_dim += 1;
    }
    else {
      break;
    }
    cnpy_parse_skip_whitespace(s);
    /* Try to read a comma. */
    if (s->pos < s->full_header_size&& s->raw_data[s->pos] == ',') {
      s->pos += 1;
      cnpy_parse_skip_whitespace(s);
      read_comma = true;
    }
  } while(s->pos < s->full_header_size && s->n_dim < CNPY_MAX_DIM && read_comma);

  /* Read the closing bracket. */
  if (s->pos < s->full_header_size) {
    if (c == '(' && s->raw_data[s->pos] == ')') {
      s->pos += 1;
    }
    else if (c == '[' && s->raw_data[s->pos] == ']') {
      s->pos += 1;
    }
    else if (isdigit(s->raw_data[s->pos])) { /* This must be because the npy file has too many dimensions. */
      return cnpy_error(CNPY_ERROR_FORMAT, "File has too many dimensions. Please recompile with larger CNPY_MAX_DIM");
    }
    else {
      return cnpy_error(CNPY_ERROR_FORMAT, "Expected closing bracket, got '%c'", c);
    }
  }
  else {
    return cnpy_error(CNPY_ERROR_FORMAT, "Expected closing bracket, got nothing");
  }

  return CNPY_SUCCESS;
}


static cnpy_status cnpy_parse_value_fortran_order(cnpy_parser_state *s) {
  assert(!s->read_fortran_order);

  size_t tmp = 0;
  if (__builtin_add_overflow(s->pos, 4, &tmp)) {
    return cnpy_error(CNPY_ERROR_FORMAT, "Overflow while reading 'True' or 'False'");
  }
  if (tmp < s->full_header_size && strncmp(s->raw_data + s->pos, "True", 4) == 0) {
    s->order = CNPY_FORTRAN_ORDER;
    s->pos += 4;
    return CNPY_SUCCESS;
  }

  if (__builtin_add_overflow(s->pos, 5, &tmp)) {
    return cnpy_error(CNPY_ERROR_FORMAT, "Overflow while reading 'True' or 'False'");
  }
  if (tmp < s->full_header_size && strncmp(s->raw_data + s->pos, "False", 5) == 0) {
    s->order = CNPY_C_ORDER;
    s->pos += 5;
    return CNPY_SUCCESS;
  }

  return cnpy_error(CNPY_ERROR_FORMAT, "Expected 'True' or 'False', got something else");
}


static cnpy_status cnpy_parse_check_data_size(cnpy_array arr) {
  size_t data_size = cnpy_dtype_sizes[arr.dtype];
  size_t tmp = 0;
  for (size_t i = 0; i < arr.n_dim; i += 1) {
    if (__builtin_mul_overflow(data_size, arr.dims[i], &data_size)) {
      return cnpy_error(CNPY_ERROR_FORMAT, "Total claimed size of the data overflows");
    }
  }
  if (arr.n_dim == 0 || data_size == 0) {
    return cnpy_error(CNPY_ERROR_FORMAT, "Empty data unsupported");
  }
  if (__builtin_add_overflow(arr.data_begin, data_size, &tmp)) {
    return cnpy_error(CNPY_ERROR_FORMAT, "Total claimed size of the data file overflows");
  }
  if (tmp != arr.raw_data_size) {
    return cnpy_error(CNPY_ERROR_FORMAT, "Claimed size of the data %zu does not match header size %zu and file size %zu", data_size, arr.data_begin, arr.raw_data_size);
  }
  return CNPY_SUCCESS;
}


static size_t cnpy_atonz(const char * const s, size_t n, size_t *result) {
  size_t r = 0;
  size_t i = 0;
  for (i = 0; i < n && isdigit(s[i]); i += 1) {
    size_t d = (size_t) (s[i] - '0');
    /* update r_new = 10 * r_old + current_digit checking for overflow. */
    r = 10 * r + d;
    // size_t tmp;
    // if (__builtin_mul_overflow(r, 10, &tmp) || __builtin_add_overflow(tmp, d, &r)) {
    //   i = 0;
    //   break;
    // }
  }
  if (i > 0) {
    *result = r;
  }
  return i;
}


/*
 * Create a new cnpy array.
 */


/*
 * How large is the serialized full header of a cnpy array with the given metadata?
 */
#if __STDC_VERSION__ >= 201112L
_Static_assert((3 * sizeof(size_t) + 1) * CNPY_MAX_DIM < 65535 - (57 + 3 + 5), "too many dimensions"); /* To avoid overflow in the next function; note that 3 = ceil(log10(256)). */
#endif
size_t cnpy_predict_full_header_size(cnpy_dtype dtype, cnpy_flat_order order, size_t n_dim, const size_t * const dims) {
  size_t full_header_size = 57 + strlen(cnpy_dtype_str[dtype]) + 1 * n_dim + ((order == CNPY_FORTRAN_ORDER)? 4 : 5);
  for (size_t i = 0; i < n_dim; i += 1) {
    full_header_size += (size_t) log10((double) dims[i]) + 1;
  }
  if (full_header_size % 16 > 0) {
    full_header_size += 16 - (full_header_size % 16);
  }
  return full_header_size;
}


/* data needs to be at least maxsize + 1 long. */
void cnpy_write_header(char *data, size_t maxsize, cnpy_byte_order byte_order, cnpy_dtype dtype, cnpy_flat_order order, size_t n_dim, const size_t * const dims) {
  assert(maxsize > 0);
  assert(maxsize < SIZE_MAX - 1);
  size_t predicted_full_header_size = cnpy_predict_full_header_size(dtype, order, n_dim, dims);
  assert(predicted_full_header_size <= maxsize);
  size_t written = 0;
  size_t tmp;

  /* NOTE: We need (maxsize + 1 - written) everywhere because snprintf writes a final \0!
   * This also means that the final snprintf() may write beyond the header.
   * This is ok, because we require prod(dims) > 1, so there is at least 1 byte of data after the header. */

  /* magic string */
  written += tmp = snprintf(data + written, maxsize + 1 - written, "\x93NUMPY");
  assert(tmp == 6);
  /* numpy format version */
  data[written] = 1;
  data[written + 1] = 0;
  written += 2;
  /* size of the header (excluding format string */
  data[written]     = (uint8_t)  (predicted_full_header_size - 10);
  data[written + 1] = (uint8_t) ((predicted_full_header_size - 10) << 8);
  written += 2;

  /* descr */
  char byte_order_char = 0;
  switch (byte_order) {
    case CNPY_LE:
      byte_order_char = '<';
      break;
    case CNPY_BE:
      byte_order_char = '>';
      break;
    case CNPY_NE:
      byte_order_char = '|';
      break;
    default:
      assert(false);
  }
  written += tmp = snprintf(data + written, maxsize + 1 - written, "{'descr': '%c%s', ", byte_order_char, cnpy_dtype_str[dtype]);
  assert(tmp == 15 + strlen(cnpy_dtype_str[dtype]));

  /* fortran_order */
  written += tmp = snprintf(data + written, maxsize + 1 - written, "'fortran_order': %s, ", (order == CNPY_FORTRAN_ORDER)? "True" : "False");
  assert(tmp == 19 + ((order == CNPY_FORTRAN_ORDER)? 4 : 5));

  /* shape */
  written += tmp = snprintf(data + written, maxsize + 1 - written, "'shape': (");
  assert(tmp == 10);
  for (size_t i = 0; i < n_dim; i += 1) {
    written += tmp = snprintf(data + written, maxsize + 1 - written, "%zu", dims[i]);
    assert(tmp == (size_t)log10(dims[i]) + 1);
    if (i < n_dim - 1) {
      written += tmp = snprintf(data + written, maxsize + 1 - written, ",");
      assert(tmp == 1);
    }
  }
  written += tmp = snprintf(data + written, maxsize + 1 - written, ")}");
  assert(tmp == 2);

  /* padding */
  assert(predicted_full_header_size - written - 1 < 16);
  for (; written < predicted_full_header_size - 1; written += 1) {
   data[written] = ' ';
  }

  /* final newline character */
  data[written] = '\n';
  written += 1;

  assert(written == predicted_full_header_size);
}


/*
 * Create a new .npy array (possibly backed by a file).
 * If fn is NULL, an anonymous mapping will be created.
 */
cnpy_status cnpy_create(const char * const fn, cnpy_byte_order byte_order, cnpy_dtype dtype, cnpy_flat_order order, size_t n_dim, const size_t * const dims, cnpy_array *arr) {
  assert(arr != NULL);

#ifndef MAP_ANONYMOUS
  if (fn == NULL) {
    return cnpy_error(CNPY_ERROR_MMAP, "BUG: Tried to create an anonymous mmap even though it is not supported");
  }
#endif

  /* clean up and check the data passed by the user */
  assert(n_dim <= CNPY_MAX_DIM);
  if (cnpy_dtype_sizes[dtype] == 1) {
    byte_order = CNPY_NE;
  }

  /* Predict file size */
  size_t full_header_size = cnpy_predict_full_header_size(dtype, order, n_dim, dims);
  size_t data_size = cnpy_dtype_sizes[dtype];
  for (size_t i = 0; i < n_dim; i += 1) {
    if (__builtin_mul_overflow(data_size, dims[i], &data_size)) {
      return cnpy_error(CNPY_ERROR_FORMAT, "Overflow when calculating required file size");
    }
  }
  assert(data_size > 0);
  size_t raw_data_size;
  if (__builtin_add_overflow(full_header_size, data_size, &raw_data_size)) {
    return cnpy_error(CNPY_ERROR_FORMAT, "Overflow when calculating required file size");
  }

  /* Open the file (if necessary) */
  int fd = -1;
  if (fn != NULL) {
    //TODO: O_RDWR | O_CREAT | O_EXCL  is thread save
    fd = open(fn, O_RDWR | O_CREAT, (mode_t) 0644);
    if (fd == -1) {
      return cnpy_error(CNPY_ERROR_FILE, "Could not open file: %s", strerror(errno));
    }
    if (ftruncate(fd, raw_data_size) != 0) {
      return cnpy_error(CNPY_ERROR_FILE, "Could not resize file: %s", strerror(errno));
    }
  }

  /* mmap() */
  char *raw_data = mmap(
    NULL, /* addr */
    raw_data_size, /* map size*/
    PROT_READ | PROT_WRITE, /* protection flags */
#ifdef MAP_ANONYMOUS
    MAP_SHARED | ((fn == NULL) ? MAP_ANONYMOUS : 0), /* private/shared flag */
#else
    MAP_SHARED, /* private/shared flag */
#endif
    fd, /* file */
    0 /* file offset */
  );

  if (raw_data == MAP_FAILED) {
    if (fd != -1) {
      close(fd); /* No point checking for error */
    }
    return cnpy_error(CNPY_ERROR_MMAP, "mmap() failed: %s", strerror(errno));
  }

  /* Close the file, if necessary. */
  if (fd != -1 && close(fd) != 0) {
    munmap(raw_data, raw_data_size); /* No point checking for error */
    return cnpy_error(CNPY_ERROR_FILE, "Could not close file after mmap(): %s", strerror(errno));
  }

  /* Write the header */
  cnpy_write_header(raw_data, full_header_size, byte_order, dtype, order, n_dim, dims);

  /* Set all entries to zero.
   * memset() works because the binary representation of (positive) 0 is all 0-bytes. */
  memset(raw_data + full_header_size, 0, raw_data_size - full_header_size);

  /* Prepare the array structure */
  cnpy_array tmp = {
    .byte_order = byte_order,
    .dtype = dtype,
    .order = order,
    .n_dim = n_dim,
    .raw_data = raw_data,
    .data_begin = full_header_size,
    .raw_data_size = raw_data_size,
  };
  for (size_t i = 0; i < n_dim; i += 1) {
    tmp.dims[i] = dims[i];
  }

  /* Final check for correctness */
  cnpy_parse_check_data_size(tmp);

  *arr = tmp;

  return CNPY_SUCCESS;
}


/*
 * Close a cnpy_array.
 */


static cnpy_status cnpy_close(cnpy_array *arr) {
  assert(arr != NULL);
  assert(arr->raw_data != NULL);

  /* just munmap() the data. */
  if (munmap(arr->raw_data, arr->raw_data_size)) {
    return cnpy_error(CNPY_ERROR_MMAP, "munmap() failed: %s", strerror(errno));
  }
  arr->raw_data = NULL;
  return CNPY_SUCCESS;
}


/*
 * Getters and setters.
 *
 * The index argument must have length at least arr.n_dim.
 * Member index[i] must be smaller than member arr.dims[i].
 *
 */

static size_t cnpy_flatten_index(cnpy_array arr, const size_t * const index) {
  assert(index != NULL);

  size_t ind = 0;
  if (arr.order == CNPY_FORTRAN_ORDER) {
    /* ind = index[0] + arr_dims[0] * index[1] + ... + arr_dims[0] * ... * arr_dims[N-1] * index[N] */
    size_t off = 1;
    for (size_t i = 0; i < arr.n_dim; i += 1) {
      assert(index[i] < arr.dims[i]);
      size_t tmp;
      assert(!__builtin_mul_overflow(index[i], off, &tmp));
      assert(!__builtin_add_overflow(ind, tmp, &ind));
      assert(!__builtin_mul_overflow(off, arr.dims[i], &off));
    }
  }
  else if (arr.order == CNPY_C_ORDER) {
    /* ind = index[0] * arr_dims[1] * ... arr_dims[N] + ... + index[N-1] * arr_dims[N] + index[N] */
    size_t off = 1;
    for (size_t i = 0; i < arr.n_dim; i += 1) {
      size_t j = arr.n_dim - 1 - i;
      assert(index[i] < arr.dims[i]);
      size_t tmp;
      assert(!__builtin_mul_overflow(index[j], off, &tmp));
      assert(!__builtin_add_overflow(ind, tmp, &ind));
      assert(!__builtin_mul_overflow(off, arr.dims[j], &off));
    }
  }
  else {
    assert(false);
  }

  return ind;
}


/*
 * Generic byte order conversion functions
 */


/* Copy n bytes from src to dst in the same order (forwards) */
static void cnpy_cpy_f(size_t n, const char * const restrict src, char * restrict dst) {
  for (size_t i = 0; i < n; i += 1) {
    dst[i] = src[i];
  }
}


/* Copy n bytes from src to dst in reverse byte order */
static void cnpy_cpy_r(size_t n, const char * const restrict src, char *restrict dst) {
  for (size_t i = 0; i < n; i += 1) {
    dst[i] = src[n - 1 - i];
  }
}


/* Copy n bytes from src to dst, changing byte order to / from byte_order */
static void cnpy_cpy(const cnpy_array arr, const char * const restrict src, char * restrict dst) {
  assert(arr.dtype != CNPY_C8 && arr.dtype != CNPY_C16);
  switch (arr.byte_order) {
    case CNPY_NE:
      cnpy_cpy_f(cnpy_dtype_sizes[arr.dtype], src, dst);
      break;
    case CNPY_BE:
#if BYTE_ORDER == LITTLE_ENDIAN
      cnpy_cpy_r(cnpy_dtype_sizes[arr.dtype], src, dst);
#elif BYTE_ORDER == BIG_ENDIAN
      cnpy_cpy_f(cnpy_dtype_sizes[arr.dtype], src, dst);
#else
#error "Unsupported byte order."
#endif
      break;
    case CNPY_LE:
#if BYTE_ORDER == LITTLE_ENDIAN
      cnpy_cpy_f(cnpy_dtype_sizes[arr.dtype], src, dst);
#elif BYTE_ORDER == BIG_ENDIAN
      cnpy_cpy_r(cnpy_dtype_sizes[arr.dtype], src, dst);
#else
#error "Unsupported byte order."
#endif
      break;
    default:
      assert(false);
  }
}


/* Copy n bytes from src to dst, changing byte order to / from byte_order; version for complex numbers */
static void cnpy_cpy_complex(const cnpy_array arr, const char * const restrict src, char * restrict dst) {
  assert(arr.dtype == CNPY_C8 || arr.dtype == CNPY_C16);
  size_t n = cnpy_dtype_sizes[arr.dtype];
  switch (arr.byte_order) {
    case CNPY_BE:
#if BYTE_ORDER == LITTLE_ENDIAN
      cnpy_cpy_r(n / 2, src, dst);
      cnpy_cpy_r(n / 2, src + n / 2, dst + n / 2);
#elif BYTE_ORDER == BIG_ENDIAN
      cnpy_cpy_f(n, src, dst);
#else
#error "Unsupported byte order."
#endif
      break;
    case CNPY_LE:
#if BYTE_ORDER == LITTLE_ENDIAN
      cnpy_cpy_f(n, src, dst);
#elif BYTE_ORDER == BIG_ENDIAN
      cnpy_cpy_r(n / 2, src, dst);
      cnpy_cpy_r(n / 2, src + n / 2, dst + n / 2);
#else
#error "Unsupported byte order."
#endif
      break;
    default:
      assert(false);
  }
}


#define cnpy_get_addr(array, index) \
    (array.raw_data + array.data_begin + cnpy_dtype_sizes[array.dtype] * cnpy_flatten_index(arr, index))


/*
 * Type specific accessors with automatic endianness conversion
 */

bool cnpy_get_b(const cnpy_array arr, const size_t * const index) {
  assert(arr.dtype == CNPY_B);
  bool ret;
  cnpy_cpy(arr, cnpy_get_addr(arr, index), (char*) &ret);
  return ret;
}

void cnpy_set_b(cnpy_array arr, const size_t *const index, bool x) {
  assert(arr.dtype == CNPY_B);
  cnpy_cpy(arr, (char*) &x, cnpy_get_addr(arr, index));
}

int8_t cnpy_get_i1(const cnpy_array arr, const size_t * const index) {
  assert(arr.dtype == CNPY_I1);
  int8_t ret;
  cnpy_cpy(arr, cnpy_get_addr(arr, index), (char*) &ret);
  return ret;
}

void cnpy_set_i1(cnpy_array arr, const size_t * const index, int8_t x) {
  assert(arr.dtype == CNPY_I1);
  cnpy_cpy(arr, (char*) &x, cnpy_get_addr(arr, index));
}

int16_t cnpy_get_i2(const cnpy_array arr, const size_t * const index) {
  assert(arr.dtype == CNPY_I2);
  int16_t ret;
  cnpy_cpy(arr, cnpy_get_addr(arr, index), (char*) &ret);
  return ret;
}

void cnpy_set_i2(cnpy_array arr, const size_t * const index, int16_t x) {
  assert(arr.dtype == CNPY_I2);
  cnpy_cpy(arr, (char*) &x, cnpy_get_addr(arr, index));
}

int32_t cnpy_get_i4(const cnpy_array arr, const size_t * const index) {
  assert(arr.dtype == CNPY_I4);
  int32_t ret;
  cnpy_cpy(arr, cnpy_get_addr(arr, index), (char*) &ret);
  return ret;
}

void cnpy_set_i4(cnpy_array arr, const size_t * const index, int32_t x) {
  assert(arr.dtype == CNPY_I4);
  cnpy_cpy(arr, (char*) &x, cnpy_get_addr(arr, index));
}

int64_t cnpy_get_i8(const cnpy_array arr, const size_t * const index) {
  assert(arr.dtype == CNPY_I8);
  int64_t ret;
  cnpy_cpy(arr, cnpy_get_addr(arr, index), (char*) &ret);
  return ret;
}

void cnpy_set_i8(cnpy_array arr, const size_t * const index, int64_t x) {
  assert(arr.dtype == CNPY_I8);
  cnpy_cpy(arr, (char*) &x, cnpy_get_addr(arr, index));
}

uint8_t cnpy_get_u1(const cnpy_array arr, const size_t * const index) {
  assert(arr.dtype == CNPY_U1);
  uint8_t ret;
  cnpy_cpy(arr, cnpy_get_addr(arr, index), (char*) &ret);
  return ret;
}

void cnpy_set_u1(cnpy_array arr, const size_t * const index, uint8_t x) {
  assert(arr.dtype == CNPY_U1);
  cnpy_cpy(arr, (char*) &x, cnpy_get_addr(arr, index));
}

uint16_t cnpy_get_u2(cnpy_array arr, const size_t * const index) {
  assert(arr.dtype == CNPY_U2);
  uint16_t ret;
  cnpy_cpy(arr, cnpy_get_addr(arr, index), (char*) &ret);
  return ret;
}

void cnpy_set_u2(cnpy_array arr, const size_t * const index, uint16_t x) {
  assert(arr.dtype == CNPY_U2);
  cnpy_cpy(arr, (char*) &x, cnpy_get_addr(arr, index));
}

uint32_t cnpy_get_u4(const cnpy_array arr, const size_t * const index) {
  assert(arr.dtype == CNPY_U4);
  uint32_t ret;
  cnpy_cpy(arr, cnpy_get_addr(arr, index), (char*) &ret);
  return ret;
}

void cnpy_set_u4(cnpy_array arr, const size_t * const index, uint32_t x) {
  assert(arr.dtype == CNPY_U4);
  cnpy_cpy(arr, (char*) &x, cnpy_get_addr(arr, index));
}

uint64_t cnpy_get_u8(const cnpy_array arr, const size_t * const index) {
  assert(arr.dtype == CNPY_U8);
  uint64_t ret;
  cnpy_cpy(arr, cnpy_get_addr(arr, index), (char*) &ret);
  return ret;
}

void cnpy_set_u8(cnpy_array arr, const size_t * const index, uint64_t x) {
  assert(arr.dtype == CNPY_U8);
  cnpy_cpy(arr, (char*) &x, cnpy_get_addr(arr, index));
}

float cnpy_get_f4(const cnpy_array arr, const size_t * const index) {
  assert(arr.dtype == CNPY_F4);
  float ret;
  cnpy_cpy(arr, cnpy_get_addr(arr, index), (char*) &ret);
  return ret;
}

void cnpy_set_f4(cnpy_array arr, const size_t * const index, float x) {
  assert(arr.dtype == CNPY_F4);
  cnpy_cpy(arr, (char*) &x, cnpy_get_addr(arr, index));
}

double cnpy_get_f8(const cnpy_array arr, const size_t * const index) {
  assert(arr.dtype == CNPY_F8);
  double ret;
  cnpy_cpy(arr, (char*) cnpy_get_addr(arr, index), (char*) &ret);
  return ret;
}

void cnpy_set_f8(cnpy_array arr, const size_t * const index, double x) {
  assert(arr.dtype == CNPY_F8);
  cnpy_cpy(arr, (char*) &x, cnpy_get_addr(arr, index));
}

complex float cnpy_get_c8(const cnpy_array arr, const size_t * const index) {
  assert(arr.dtype == CNPY_C8);
  complex float ret;
  cnpy_cpy_complex(arr, cnpy_get_addr(arr, index), (char*) &ret);
  return ret;
}

void cnpy_set_c8(const cnpy_array arr, const size_t * const index, complex float x) {
  assert(arr.dtype == CNPY_C8);
  cnpy_cpy_complex(arr, (char*) &x, cnpy_get_addr(arr, index));
}

complex double cnpy_get_c16(const cnpy_array arr, const size_t * const index) {
  assert(arr.dtype == CNPY_C16);
  complex double ret;
  cnpy_cpy_complex(arr, cnpy_get_addr(arr, index), (char*) &ret);
  return ret;
}

void cnpy_set_c16(const cnpy_array arr, const size_t * const index, complex double x) {
  assert(arr.dtype == CNPY_C16);
  cnpy_cpy_complex(arr, (char*) &x, cnpy_get_addr(arr, index));
}


/*
 * Iteration
 */


/* Set index to zero */
void cnpy_reset_index(const cnpy_array arr, size_t *index) {
  assert(index != NULL);

  for (size_t i = 0; i < arr.n_dim; i += 1) {
    index[i] = 0;
  }
}


/* Get next index if possible */
bool cnpy_next_index(const cnpy_array arr, size_t *index) {
  assert(index != NULL);
  bool updated = false;
  size_t i = 0;
  switch (arr.order) {
    case CNPY_FORTRAN_ORDER:
      for (i = 0; i < arr.n_dim; i += 1) {
        assert(index[i] < arr.dims[i]);
        if (index[i] + 1 < arr.dims[i]) {
          index[i] += 1;
          updated = true;
          break;
        }
      }
      if (updated) {
        for (size_t j = 0; j < i; j += 1) {
          index[j] = 0;
        }
      }
      break;
    case CNPY_C_ORDER:
      for (i = arr.n_dim - 1; i < arr.n_dim; i -= 1) {
        assert(index[i] < arr.dims[i]);
        if (index[i] + 1 < arr.dims[i]) {
          index[i] += 1;
          updated = true;
          break;
        }
      }
      if (updated) {
        for (size_t j = i + 1; j < arr.n_dim; j += 1) {
          index[j] = 0;
        }
      }
      break;
    default:
      assert(false);
  }
  return updated;
}
