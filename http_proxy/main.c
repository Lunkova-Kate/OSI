#include "proxy.h"

int main()
{
    if (start_proxy_server(LISTENING_PORT) != SUCCESS)
    {
        fprintf(stderr, "Failed to start proxy server\n");
        return 1;
    }

    return 0;
}
/* gcc proxy.c main.c -o proxy -Werror -pedantic -Wextra -O2 -lpthread
 ./proxy  */
// curl -x http://localhost:8080 http://google.com

/* wget
http://dl-cdn.alpinelinux.org/alpine/v3.18/releases/x86_64/alpine-standard-3.18.4-x86_64.iso
wget -e http_proxy=http://localhost:8080 http://dl-cdn.alpinelinux.org/alpine/v3.18/releases/x86_64/alpine-standard-3.18.4-x86_64.iso
 */

/* /tmp/proxy_test
 md5sum ubuntu-22.04.3-desktop-amd64.iso ubuntu-22.04.3-desktop-amd64.iso.1




rm ubuntu-22.04.3-desktop-amd64.iso
rm ubuntu-22.04.3-desktop-amd64.iso.1
 valgrind --leak-check=full ./proxy
 */
