#include <stdio.h>
#include <string.h>
#define STB_C_LEXER_IMPLEMENTATION

#include "stb_c_lexer.h"

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
                              "while"};

char *ignored_c_funcs[] = {
    "main",   "abs",     "atoi",   "atof",   "bsearch", "ceil",    "clock",
    "cos",    "exit",    "fclose", "fgets",  "fopen",   "fprintf", "fputs",
    "fread",  "free",    "fwrite", "floor",  "malloc",  "memcpy",  "memmove",
    "memset", "pow",     "printf", "qsort",  "rand",    "realloc", "scanf",
    "sin",    "srand",   "sqrt",   "strcat", "strncat", "strcmp",  "strncmp",
    "strcpy", "strncpy", "strlen", "system", "time"};

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

int add_renamed_id(char *id, int id_len, struct StrBuf *buf) {
  int found_off = -1;
  for (int i = 0; i < conv_table_len; i++) {
    if (conv_table[i].id_len != id_len)
      continue;
    if (strncmp(conv_table[i].id, id, id_len) == 0) {
      found_off = i;
      break;
    }
  }

  char tmp[10];
  if (found_off != -1) {
    int len = snprintf(tmp, 10, "_%d", found_off);
    str_buf_append(buf, tmp, len);
    return len;
  }

  conv_table[conv_table_len] = (struct ConvTableEntry){id, id_len};
  int len = snprintf(tmp, 10, "_%d", conv_table_len);
  conv_table_len++;
  str_buf_append(buf, tmp, len);
  return len;
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
  int p1 = tok_ty1 == CLEX_id || tok_ty1 == CLEX_intlit ||
           tok_ty1 == CLEX_floatlit || tok_ty1 == CLEX_dqstring ||
           tok_ty1 == CLEX_sqstring;

  int p2 = tok_ty2 == CLEX_id || tok_ty2 == CLEX_intlit ||
           tok_ty2 == CLEX_floatlit || tok_ty2 == CLEX_dqstring ||
           tok_ty2 == CLEX_sqstring;

  return p1 && p2;
}

struct StrBuf obfuscate(char *buf, int buf_size, int target_ln_len) {
  struct StrBuf str = {0};

  stb_lexer lexer;
  char store[1024];
  int curr_ln_len = 0;

  stb_c_lexer_init(&lexer, buf, NULL, store, 1024);
  int prev_tok_ty = CLEX_parse_error;

  while (stb_c_lexer_get_token(&lexer)) {
    char *tok = lexer.where_firstchar;
    int tok_len = lexer.where_lastchar - lexer.where_firstchar + 1;
    int tok_ty = lexer.token;

    printf("Tok: %.*s\n", tok_len, tok);
    getc(stdin);

    // TODO: skip int

    /* if (tok_len == 3 && strncmp(tok, "int", 3) == 0) */
    /*   continue; */

    if (target_ln_len != 0 && curr_ln_len + tok_len > target_ln_len) {
      curr_ln_len = 0;
      str_buf_append(&str, "\n", 1);
    } else {
      if (should_emit_space(tok_ty, prev_tok_ty)) {
        str_buf_append(&str, " ", 1);
        tok_len += 1;
      }
    }

    if (tok_ty == CLEX_id) {
      if (can_rename_id(tok, tok_len)) {
        tok_len = add_renamed_id(tok, tok_len, &str); // new len
        goto increase;
      }
    }

    str_buf_append(&str, tok, tok_len);

  increase:
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
  buf[fsize] = '\0';

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
