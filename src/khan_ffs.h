#ifndef KHAN_FFS
#define KHAN_FFS

typedef struct _simple_rec {
  int exp_id;
  char* file_path;
  long file_buf_len;
  char* file_buf;
} simple_rec, *simple_rec_ptr;

static FMField simple_field_list[] =
{
  {"exp_id", "integer", sizeof(int), FMOffset(simple_rec_ptr, exp_id)},
  {"file_path", "string", sizeof(char*), FMOffset(simple_rec_ptr, file_path)},
  {"file_buf_len", "integer", sizeof(long), FMOffset(simple_rec_ptr, file_buf_len)},
  {"file_buf", "char[file_buf_len]", sizeof(char), FMOffset(simple_rec_ptr, file_buf)},
  {NULL, NULL, 0, 0}
};

static FMStructDescRec simple_format_list[] =
{
  {"simple", simple_field_list, sizeof(simple_rec), NULL},
  {NULL, NULL}
};

#endif
