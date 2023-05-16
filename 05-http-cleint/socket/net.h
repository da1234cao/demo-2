#pragma once
#include "connection.h"

int socket_connect(connection *con);
status socket_write(connection *con);
status socket_read(connection *con);
void construct_request(connection *con);