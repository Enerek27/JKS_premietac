#ifndef PRESENTATION_STUF_H
#define PRESENTATION_STUF_H


#include <stdbool.h>
#define PORT 2020




bool network_connect(const char* ip_adrress);

bool network_send_data(const char *data);

void network_disconnect();

bool network_is_connected();


#endif