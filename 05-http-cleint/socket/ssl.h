#pragma once
#include "connection.h"

status ssl_init(connection *c);
status ssl_connect(connection *con);
status ssl_close(connection *con);
status ssl_read(connection *con);
status ssl_write(connection *con);
size_t ssl_readable(connection *con);