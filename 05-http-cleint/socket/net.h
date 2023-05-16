#pragma once
#include "connection.h"

status socket_init(connection *con);
status socket_connect(connection *con);
status socket_write(connection *con);
status socket_read(connection *con);
