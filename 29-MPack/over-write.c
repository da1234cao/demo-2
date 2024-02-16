#include "mpack.h"
#include <stdio.h>

/*
{
    "name": "3-1",
    "number": 56,
    "students": [
        {
            "name": "zhangsan",
            "score": 76.8
        },
        {
            "name": "lisi",
            "score": 77
        }
    ]
}
*/

mpack_error_t class_information_serialize(char *data, size_t *size) {
  mpack_writer_t writer;
  mpack_writer_init(&writer, data, *size);

  mpack_build_map(&writer);

  mpack_write_cstr(&writer, "name");
  mpack_write_cstr(&writer, "3-1");

  mpack_write_cstr(&writer, "number");
  mpack_write_u8(&writer, 56);

  mpack_write_cstr(&writer, "students");

  mpack_build_array(&writer);

  for (unsigned int i = 0; i < 56; i++) {
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "name");
    mpack_write_cstr(&writer, "zhangsan");
    mpack_write_cstr(&writer, "score");
    mpack_write_float(&writer, 76.8);

    if (mpack_writer_error(&writer) != mpack_ok) {
      printf("error_%u: %d\n", i, mpack_writer_error(&writer));
    }
    mpack_finish_map(&writer);
  }

  mpack_complete_array(&writer);

  mpack_complete_map(&writer);

  if (mpack_writer_error(&writer) != mpack_ok) {
    printf("after write all students error: %d\n", mpack_writer_error(&writer));
  }

  *size = mpack_writer_buffer_used(&writer);
  mpack_error_t ret = mpack_writer_destroy(&writer);

  return ret;
}

mpack_error_t class_information_deserialize(const char *data, size_t length) {
  mpack_tree_t tree;
  mpack_tree_init_data(&tree, data, length);
  mpack_tree_parse(&tree);

  if (mpack_tree_error(&tree) != mpack_ok) {
    printf("parse tree error: %d\n", mpack_tree_error(&tree));
  }

  mpack_node_t root = mpack_tree_root(&tree);
  const char *name = mpack_node_str(mpack_node_map_cstr(root, "name"));
  size_t name_len = mpack_node_strlen(mpack_node_map_cstr(root, "name"));
  uint8_t number = mpack_node_u8(mpack_node_map_cstr(root, "number"));
  printf("name:%.*s\n", name_len, name);
  printf("number:%u\n", number);

  printf("students:\n");
  mpack_node_t students = mpack_node_map_cstr(root, "students");
  size_t student_num = mpack_node_array_length(students);
  for (unsigned int i = 0; i < student_num; i++) {
    mpack_node_t student = mpack_node_array_at(students, i);
    const char *name = mpack_node_str(mpack_node_map_cstr(student, "name"));
    size_t name_len = mpack_node_strlen(mpack_node_map_cstr(student, "name"));
    float score = mpack_node_float(mpack_node_map_cstr(student, "score"));
    printf("  name:%.*s\n", name_len, name);
    printf("  score:%.2f\n", score);
  }

  mpack_error_t ret = mpack_tree_destroy(&tree);
  return ret;
}

int main(int argc, char *argv[]) {
#define DATA_BUFFER_SIZE 35
  char data[DATA_BUFFER_SIZE] = {0};
  size_t size = DATA_BUFFER_SIZE;
  class_information_serialize(data, &size);
  class_information_deserialize(data, size);
}