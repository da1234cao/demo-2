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