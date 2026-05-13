/*
 * cliente_consulta.c
 *
 * Cliente IPv6 básico para preguntar al servidor central M1
 * por la información de un grupo.
 *
 * Uso:
 *   ./cliente_consulta <grupo>
 *
 * Ejemplo:
 *   ./cliente_consulta 1
 *   ./cliente_consulta 2
 *
 * Compilación:
 *   gcc -Wall -Wextra -o cliente_consulta cliente_consulta.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CENTRAL_IPV6 "2001:720:1d10:fff0:216:3eff:feec:ee82"
#define CENTRAL_PORT 3000
#define BUFFER_SIZE 256

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in6 server_addr;
    char request[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <grupo>\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s 1\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int group_id = atoi(argv[1]);

    sockfd = socket(AF_INET6, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("[CLIENTE] Error creando socket IPv6");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(CENTRAL_PORT);

    if (inet_pton(AF_INET6, CENTRAL_IPV6, &server_addr.sin6_addr) != 1) {
        fprintf(stderr, "[CLIENTE] IPv6 del servidor central no válida\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("[CLIENTE] Conectando con M1 [%s]:%d...\n",
           CENTRAL_IPV6,
           CENTRAL_PORT);

    if (connect(sockfd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        perror("[CLIENTE] Error en connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    snprintf(request, sizeof(request), "GROUP %d\n", group_id);

    printf("[CLIENTE] Enviando petición: %s", request);

    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror("[CLIENTE] Error en send");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    memset(response, 0, sizeof(response));

    ssize_t bytes_read = recv(sockfd, response, sizeof(response) - 1, 0);

    if (bytes_read < 0) {
        perror("[CLIENTE] Error en recv");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (bytes_read == 0) {
        printf("[CLIENTE] El servidor cerró la conexión sin responder\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    response[bytes_read] = '\0';

    printf("[CLIENTE] Respuesta recibida:\n%s", response);

    close(sockfd);

    return 0;
}
