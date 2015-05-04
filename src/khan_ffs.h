#ifndef KHAN_FFS
#define KHAN_FFS

typedef struct _python_list {
    int dynamic_size;
    int* ordered_method_list;
} python_list, *python_list_ptr;

typedef struct _simple_rec {
  int exp_id;
  char* file_path;
  char* db_id;
  int meta_compare_py;
  long file_buf_len;
  char* file_buf;
} simple_rec, *simple_rec_ptr;

static FMField simple_field_list[] =
{
  {"exp_id", "integer", sizeof(int), FMOffset(simple_rec_ptr, exp_id)},
  {"file_path", "string", sizeof(char*), FMOffset(simple_rec_ptr, file_path)},
  {"db_id", "string", sizeof(char*), FMOffset(simple_rec_ptr, db_id)},
  {"meta_compare_py", "integer", sizeof(int), FMOffset(simple_rec_ptr, meta_compare_py)},
  {"file_buf_len", "integer", sizeof(long), FMOffset(simple_rec_ptr, file_buf_len)},
  {"file_buf", "char[file_buf_len]", sizeof(char), FMOffset(simple_rec_ptr, file_buf)},
  {NULL, NULL, 0, 0}
};

static FMField python_list_field_list[] = 
{
    {"dynamic_size", "integer", sizeof(int), FMOffset(python_list_ptr, dynamic_size)},
    {"ordered_method_list", "integer[dynamic_size]", sizeof(int), FMOffset(python_list_ptr, ordered_method_list)},
    {NULL, NULL, 0, 0}
};

static FMStructDescRec python_format_list[] =
{
    {"python_list", python_list_field_list, sizeof(python_list), NULL},
    {NULL, NULL}
};

static FMStructDescRec simple_format_list[] =
{
  {"simple", simple_field_list, sizeof(simple_rec), NULL},
  {NULL, NULL}
};

#endif
