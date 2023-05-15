#pragma once
#include "connection.h"
#include "3rd/http_parser.h"

struct sock {
  status (*connect)(connection *c);
  status (*read)(connection *c);
  status (*write)(connection *c);
  status (*close)(connection *c);
  status (*readable)(connection *c);
};