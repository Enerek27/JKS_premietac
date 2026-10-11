#include "wifi.h"

#include <arpa/inet.h>

#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int client_socket = -1;

bool network_connect(const char *ip_address, int port) {
  if (network_is_connected()) {
    printf("NETWORK is connected\n");
    return true;
  }

  if (ip_address == NULL || port <= 0) {
    printf("NETWORK wrong address\n");
    return false;
  }

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("NETWORK error in socket\n");
    return false;
  }

  struct timeval timeout = {.tv_sec = 3, .tv_usec = 0};
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);

  if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) <= 0) {
    printf("NETWORK zly format adresy: %s\n", ip_address);
    close(sock);
    return false;
  }

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    printf("NETWORK nepodarilo sa pripojit (%s:%d)\n", ip_address, port);
    close(sock);
    return false;
  }

  client_socket = sock;
  printf("NETWORK Uspene pripojene %s:%d\n", ip_address, port);
  return true;
}

bool network_send(const char *data) {
  if (!network_is_connected()) {
    printf("NETWORK No connection to server\n");
    return false;
  }

  if (data == NULL) {
    return false;
  }

  size_t len = strlen(data);
  ssize_t bytes_sent = send(client_socket, data, len, MSG_NOSIGNAL);

  if (bytes_sent < 0) {
    printf("NETWORK chyba pri odosielani \n");
    network_disconnect();
    return false;
  }

  bool ret = (bytes_sent == (ssize_t)len);
  return ret;
}

void network_disconnect(void) {
    if (client_socket >= 0) {
        close(client_socket);
        client_socket = -1;
        printf("NETWORK odpojene od RP\n");
    }

}

bool network_is_connected(void) { 
    bool ret = (client_socket >= 0);
    return ret;
}