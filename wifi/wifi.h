#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>


bool network_connect(const char *ip_address, int port);

bool network_send(const char* data);

void network_disconnect(void);

bool network_is_connected(void);



#endif