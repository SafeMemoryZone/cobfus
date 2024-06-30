#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SLEX_IMPLEMENTATION
#include "slex.h"

struct StrBuf {
  char *str;
  int len;
  int capacity;
};

struct ConvTableEntry {
  char *id;
  int id_len;
};

char *ignored_c_keywords[] = {"alignas",       "alignof",       "auto",
                              "bool",          "break",         "case",
                              "char",          "const",         "constexpr",
                              "continue",      "default",       "do",
                              "double",        "else",          "enum",
                              "extern",        "false",         "float",
                              "for",           "goto",          "if",
                              "inline",        "int",           "long",
                              "NULL",          "nullptr",       "register",
                              "restrict",      "return",        "short",
                              "signed",        "sizeof",        "static",
                              "static_assert", "struct",        "switch",
                              "thread_local",  "true",          "typedef",
                              "typeof",        "typeof_unqual", "union",
                              "unsigned",      "void",          "volatile",
                              "while",         "stderr",        "stdout",
                              "stdin",         "FILE",          "SEEK_END",
                              "SEEK_SET"};

char *ignored_c_funcs[] = {"main",    "abs",    "atoi",    "atof",   "bsearch",
                           "ceil",    "clock",  "cos",     "exit",   "fclose",
                           "fgets",   "fopen",  "fprintf", "fputs",  "fread",
                           "free",    "fwrite", "floor",   "malloc", "memcpy",
                           "memmove", "memset", "pow",     "printf", "snprintf",
                           "qsort",   "fseek",  "ftell",   "rand",   "realloc",
                           "scanf",   "sin",    "srand",   "sqrt",   "strcat",
                           "strncat", "strcmp", "strncmp", "strcpy", "strncpy",
                           "strlen",  "system", "time"};

char *additional_ignored_syms[1024];
int ignored_syms_len;

struct ConvTableEntry conv_table[1024];
int conv_table_len;

void *unwrap(void *val, char *err_msg) {
  if (val == NULL) {
    fprintf(stderr, "Error: %s\n", err_msg);
    exit(1);
  }
  return val;
}

void str_buf_append(struct StrBuf *buf, char *str, int str_len) {
  if (buf->len + str_len > buf->capacity) {
    buf->capacity = (buf->len + str_len) * 2;
    buf->str = realloc(buf->str, buf->capacity);
  }

  memcpy(buf->str + buf->len, str, str_len);
  buf->len += str_len;
}

void str_buf_free(struct StrBuf *buf) { free(buf->str); }

int find_renamed_id(char *id, int id_len) {
  for (int i = 0; i < conv_table_len; i++) {
    if (conv_table[i].id_len != id_len)
      continue;
    if (strncmp(conv_table[i].id, id, id_len) == 0) {
      return i;
    }
  }
  return -1;
}

// TODO: binary search
int can_rename_id(char *id, int id_len) {
  for (int i = 0; i < sizeof(ignored_c_keywords) / sizeof(char *); i++) {
    if (strlen(ignored_c_keywords[i]) != id_len)
      continue;
    if (strncmp(ignored_c_keywords[i], id, id_len) == 0)
      return 0;
  }

  for (int i = 0; i < sizeof(ignored_c_funcs) / sizeof(char *); i++) {
    if (strlen(ignored_c_funcs[i]) != id_len)
      continue;
    if (strncmp(ignored_c_funcs[i], id, id_len) == 0)
      return 0;
  }

  for (int i = 0; i < ignored_syms_len; i++) {
    if (strlen(additional_ignored_syms[i]) != id_len)
      continue;
    if (strncmp(additional_ignored_syms[i], id, id_len) == 0)
      return 0;
  }

  return 1;
}

int should_emit_space(int tok_ty1, int tok_ty2) {
  int p1 = tok_ty1 == SLEX_TOK_identifier || tok_ty1 == SLEX_TOK_int_lit ||
           tok_ty1 == SLEX_TOK_float_lit || tok_ty1 == SLEX_TOK_str_lit ||
           tok_ty1 == SLEX_TOK_char_lit;

  int p2 = tok_ty2 == SLEX_TOK_identifier || tok_ty2 == SLEX_TOK_int_lit ||
           tok_ty2 == SLEX_TOK_float_lit || tok_ty2 == SLEX_TOK_str_lit ||
           tok_ty2 == SLEX_TOK_char_lit;

  return p1 && p2;
}

struct StrBuf obfuscate(char *buf, int buf_size, int target_ln_len) {
  struct StrBuf str = {0};

  SlexContext lexer;
  char store[1024];
  int curr_ln_len = 0;

  slex_init_context(&lexer, buf, buf + buf_size, store, 1024);
  int prev_tok_ty = SLEX_ERR_unknown_tok;

  for(;;) {
    if(!slex_get_next_token(&lexer)) {
      int ln;
      int col;
      slex_get_parse_ptr_location(&lexer, buf, &ln, &col);
      fprintf(stderr, "Error: File contains an error at %d:%d", ln, col);
      exit(1);
    }

    int tok_ty = lexer.tok_ty;
    if(tok_ty == SLEX_TOK_eof) break;

    char *tok = lexer.first_tok_char;
    int tok_len = lexer.last_tok_char - lexer.first_tok_char + 1;
    char tmp[256];

    if (tok_ty == SLEX_TOK_identifier && can_rename_id(tok, tok_len)) {
      int found_off = find_renamed_id(tok, tok_len);
      if (found_off == -1) {
        found_off = conv_table_len;
        conv_table[conv_table_len++] = (struct ConvTableEntry){tok, tok_len};
      }
      tok_len = snprintf(tmp, 10, "_%d", found_off);
      tok = tmp;
    }

    // TODO: skip int

    /* if (tok_len == 3 && strncmp(tok, "int", 3) == 0) */
    /*   continue; */

    if (target_ln_len != 0 && curr_ln_len + tok_len > target_ln_len) {
      curr_ln_len = 0;
      str_buf_append(&str, "\n", 1);
    } else if (should_emit_space(tok_ty, prev_tok_ty)) {
      str_buf_append(&str, " ", 1);
      curr_ln_len += 1;
    }

    str_buf_append(&str, tok, tok_len);

    curr_ln_len += tok_len;
    prev_tok_ty = tok_ty;
  }

  return str;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr,
            "Error: Usage: ./cobfus <input_file> [-o <output_file>] [-l "
            "<target_ln_len>] [-i ...]\n");
    return 1;
  }

  char *input_path = argv[1];

  char *output_path = NULL;
  int max_line_len = 0;
  int i = 2;

  while (i < argc) {
    if (i < argc - 1 && strcmp(argv[i], "-o") == 0) {
      output_path = argv[i + 1];
      i += 2;
    } else if (i < argc - 1 && strcmp(argv[i], "-l") == 0) {
      max_line_len = atoi(argv[i + 1]);
      i += 2;
    } else if (i < argc - 1 && strcmp(argv[i], "-i") == 0) {
      i += 1;
      for (; i < argc; i++) {
        additional_ignored_syms[ignored_syms_len++] = argv[i];
      }
    } else {
      fprintf(stderr, "Warning: Ignoring arg '%s'\n", argv[i]);
      i++;
    }
  }

  FILE *fd = unwrap(fopen(input_path, "r"), "Unable to open input file");
  fseek(fd, 0L, SEEK_END);
  int fsize = ftell(fd);
  fseek(fd, 0L, SEEK_SET);

  if (fsize == 0) {
    fprintf(stderr, "Error: Input file is empty\n");
    fclose(fd);
    return 1;
  }

  char *buf = malloc(fsize + 1);
  fread(buf, 1, fsize, fd);
  fclose(fd);

  struct StrBuf obfuscated_contents = obfuscate(buf, fsize, max_line_len);
  free(buf);

  if (!output_path) {
    char buf[1024];
    snprintf(buf, 1024, "%s.obfus", input_path);
    fd = unwrap(fopen(buf, "w"), "Unable to open generated output file");
    fwrite(obfuscated_contents.str, 1, obfuscated_contents.len, fd);
    fclose(fd);
  } else {
    fd = unwrap(fopen(output_path, "w"), "Unable to open output file");
    fwrite(obfuscated_contents.str, 1, obfuscated_contents.len, fd);
    fclose(fd);
  }

  str_buf_free(&obfuscated_contents);
  return 0;
}
