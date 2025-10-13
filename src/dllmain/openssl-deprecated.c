#include <openssl/ssl.h>

long SSL_SESSION_get_time(const SSL_SESSION *s)
{
    return (long)SSL_SESSION_get_time_ex(s);
}
