/* frome:
 * https://raw.githubusercontent.com/curl/curl/master/docs/examples/https.c
 * Simple HTTPS GET
 */
#include <curl/curl.h>
#include <stdio.h>

int main(void) {
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_DEFAULT);

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "https://example.com/");

#ifdef SKIP_PEER_VERIFICATION
    /*
     * If you want to connect to a site who is not using a certificate that is
     * signed by one of the certs in the CA bundle you have, you can skip the
     * verification of the server's certificate. This makes the connection
     * A LOT LESS SECURE.
     *
     * If you have a CA cert for the server stored someplace else than in the
     * default bundle, then the CURLOPT_CAPATH option might come handy for
     * you.
     */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
    /*
     * If the site you are connecting to uses a different host name that what
     * they have mentioned in their server certificate's commonName (or
     * subjectAltName) fields, libcurl will refuse to connect. You can skip
     * this check, but this will make the connection less secure.
     */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

    /* cache the CA cert bundle in memory for a week */
    curl_easy_setopt(curl, CURLOPT_CA_CACHE_TIMEOUT, 604800L);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    /* always cleanup */
    curl_easy_cleanup(curl);
  }

  curl_global_cleanup();

  return 0;
}