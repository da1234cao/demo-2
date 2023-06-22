#include "ssl.h"
#include "net.h"

status ssl_init(connection *con) {
  if (con->ctx = SSL_CTX_new(SSLv23_client_method())) {
    SSL_CTX_set_verify(con->ctx, SSL_VERIFY_NONE, NULL);
    SSL_CTX_set_verify_depth(con->ctx, 0);
    SSL_CTX_set_mode(con->ctx, SSL_MODE_AUTO_RETRY);
    con->ssl = SSL_new(con->ctx);
    return OK;
  }
  return ERROR;
}

status ssl_connect(connection *con) {
  socket_connect(con);
  int r;
  SSL_set_fd(con->ssl, con->fd);
  char *host = copy_url_part(con->url, &con->url_parts, UF_HOST);
  SSL_set_tlsext_host_name(con->ssl, host);
  if ((r = SSL_connect(con->ssl)) != 1) {
    switch (SSL_get_error(con->ssl, r)) {
    case SSL_ERROR_WANT_READ:
      return RETRY;
    case SSL_ERROR_WANT_WRITE:
      return RETRY;
    default:
      return ERROR;
    }
  }
  return OK;
}

status ssl_read(connection *con) {
  while (con->is_recv_all == 0 && (sizeof(con->recv_buf) - con->recv_n > 0)) {
    int n = SSL_read(con->ssl, con->recv_buf, sizeof(con->recv_buf));
    if (n < 0) {
      switch (SSL_get_error(con->ssl, n)) {
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
        continue;
      default:
        return ERROR;
      }
    }
    con->recv_n += n;
    http_parser_execute(&con->parser, &con->settings, con->recv_buf,
                        con->recv_n);
  }
  return OK;
}

status ssl_write(connection *con) {
  int n = 0;
  while (n < strlen(con->send_uf)) {
    int send_n =
        SSL_write(con->ssl, con->send_uf + n, strlen(con->send_uf) - n);
    if (send_n < 0) {
      switch (SSL_get_error(con->ssl, send_n)) {
      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
        continue;
      default:
        return ERROR;
      }
    }
    n += send_n;
  }
  return OK;
}

status ssl_close(connection *c) {
  SSL_shutdown(c->ssl);
  SSL_clear(c->ssl);
  return OK;
}

size_t ssl_readable(connection *c) { return SSL_pending(c->ssl); }