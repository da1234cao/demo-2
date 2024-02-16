#include "mpack.h"
#include <stdio.h>

/*
[
  {
    "name": "zhangsan",
    "score": 76.8
  },
  {
    "name": "lisi",
    "score": 77
  }
  ...
]

*/

mpack_error_t class_information_serialize(char *data, size_t *size) {
  mpack_writer_t writer;
  mpack_writer_init(&writer, data, *size);

  mpack_build_array(&writer);

  for (unsigned int i = 0; i < 56; i++) {
    mpack_build_map(&writer);
    mpack_write_cstr(&writer, "name");
    mpack_write_cstr(&writer, "zhangsan");
    mpack_write_cstr(&writer, "score");
    mpack_write_float(&writer, 76.8);
    mpack_complete_map(&writer);
  }

  mpack_complete_array(&writer);

  if (mpack_writer_error(&writer) != mpack_ok) {
    printf("after write all students error: %d\n", mpack_writer_error(&writer));
  }

  *size = mpack_writer_buffer_used(&writer);
  mpack_error_t ret = mpack_writer_destroy(&writer);

  return ret;
}

mpack_error_t class_information_deserialize(const char *data, size_t length) {
  mpack_reader_t reader;
  mpack_reader_init_data(&reader, data, length);

  uint32_t students_num = mpack_expect_array(&reader);
  printf("students num: %u\n", students_num);
  for (unsigned int i = 0; i < students_num; i++) {
    uint32_t elem_cnt = mpack_expect_map(&reader);
    mpack_expect_cstr_match(&reader, "name");
    char name[20];
    size_t name_len = mpack_expect_str_buf(&reader, name, 20);
    mpack_expect_cstr_match(&reader, "score");
    float score = mpack_expect_float(&reader);

    if (mpack_reader_error(&reader) != mpack_ok) {
      break;
    }

    printf("name:%.*s\n", name_len, name);
    printf("score:%.2f\n", score);
    mpack_done_map(&reader);
  }

  return mpack_ok;
}

int main(int argc, char *argv[]) {
#define DATA_BUFFER_SIZE 100
  char data[DATA_BUFFER_SIZE] = {0};
  size_t size = DATA_BUFFER_SIZE;
  class_information_serialize(data, &size);
  class_information_deserialize(data, size);
}