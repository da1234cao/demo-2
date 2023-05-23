#pragma once
#include "3rd/http_parser.h"
#include "connection.h"

struct sock {
  status (*init)(connection *c);
  status (*connect)(connection *c);
  status (*read)(connection *c);
  status (*write)(connection *c);
  status (*close)(connection *c);
  size_t (*readable)(connection *c);
};