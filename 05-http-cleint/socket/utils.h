#pragma once
#include "main.h"

char *copy_url_part(const char *url, const struct http_parser_url *parts,
                    enum http_parser_url_fields field);

void print_connection(const connection *con);