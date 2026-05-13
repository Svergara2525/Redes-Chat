/*
 * cliente_send.c
 *
 * Cliente emisor IPv6 para la arquitectura de mensajería instantánea.
 *
 * Funcionamiento:
 *   1. Se conecta al servidor central M1.
 *   2. Solicita la información del grupo con "GROUP <id>".
 *   3. Recibe la IPv6 y puerto TCP del servidor de grupo.
 *   4. Se conecta a M2 o M3.
 *   5. Envía un mensaje por TCP.
 *   6. Incluye timestamp_ms para medir latencia extremo a extremo.
 *   7. Opcionalmente repite el envío cada X segundos.
 *
 * Compilación:
 *   gcc -Wall -Wextra -o cliente_send cliente_send.c
 *
 * Uso:
 *   ./cliente_send <client_id> <group_id> "<mensaje>"
 *   ./cliente_send <client_id> <group_id> "<mensaje>" <intervalo_segundos>
 *
 * Ejemplos:
 *   ./cliente_send client-1 1 "Hola grupo 1"
 *   ./cliente_send client-1 1 "Hola grupo 1" 5
 *   ./cliente_send client-7 2 "Hola grupo 2" 2
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CENTRAL_IPV6 "2001:720:1d10:fff0:216:3eff:feec:ee82"
#define CENTRAL_PORT 3000

#define BUFFER_SIZE 512
#define MESSAGE_SIZE 1024

static volatile sig_atomic_t running = 1;

typedef struct {
    char group_server_ipv6[INET6_ADDRSTRLEN];
    int group_server_port;
    char multicast_ipv6[INET6_ADDRSTRLEN];
    int multicast_port;
} group_info_t;

static void handle_sigint(int sig)
{
    (void)sig;
    running = 0;
}

static long long current_time_ms(void)
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL) != 0) {
        perror("[CLIENTE] Error en gettimeofday");
        return -1;
    }

    return ((long long)tv.tv_sec * 1000LL) + ((long long)tv.tv_usec / 1000LL);
}

static void remove_newline(char *str)
{
    if (str == NULL) {
        return;
    }

    size_t len = strlen(str);

    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

static int connect_ipv6_tcp(const char *ipv6, int port)
{
    int sockfd;
    struct sockaddr_in6 server_addr;

    if (ipv6 == NULL || port <= 0) {
        fprintf(stderr, "[CLIENTE] Parámetros inválidos en connect_ipv6_tcp\n");
        return -1;
    }

    sockfd = socket(AF_INET6, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("[CLIENTE] Error creando socket IPv6");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons((uint16_t)port);

    if (inet_pton(AF_INET6, ipv6, &server_addr.sin6_addr) != 1) {
        fprintf(stderr, "[CLIENTE] IPv6 no válida: %s\n", ipv6);
        close(sockfd);
        return -1;
    }

    if (connect(sockfd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        fprintf(stderr,
                "[CLIENTE] Error conectando con [%s]:%d: %s\n",
                ipv6,
                port,
                strerror(errno));
        close(sockfd);
        return -1;
    }

    return sockfd;
}

static int request_group_info(int group_id, group_info_t *info)
{
    int sockfd;
    char request[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    if (info == NULL) {
        return -1;
    }

    memset(info, 0, sizeof(group_info_t));
    memset(request, 0, sizeof(request));
    memset(response, 0, sizeof(response));

    printf("[CLIENTE] Conectando con servidor central M1 [%s]:%d...\n",
           CENTRAL_IPV6,
           CENTRAL_PORT);

    sockfd = connect_ipv6_tcp(CENTRAL_IPV6, CENTRAL_PORT);

    if (sockfd < 0) {
        return -1;
    }

    snprintf(request, sizeof(request), "GROUP %d\n", group_id);

    printf("[CLIENTE] Solicitando información del grupo %d...\n", group_id);

    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror("[CLIENTE] Error enviando petición a M1");
        close(sockfd);
        return -1;
    }

    ssize_t bytes_read = recv(sockfd, response, sizeof(response) - 1, 0);

    if (bytes_read < 0) {
        perror("[CLIENTE] Error recibiendo respuesta de M1");
        close(sockfd);
        return -1;
    }

    if (bytes_read == 0) {
        fprintf(stderr, "[CLIENTE] M1 cerró la conexión sin responder\n");
        close(sockfd);
        return -1;
    }

    response[bytes_read] = '\0';
    remove_newline(response);

    printf("[CLIENTE] Respuesta de M1: %s\n", response);

    close(sockfd);

    if (strncmp(response, "ERROR", 5) == 0) {
        fprintf(stderr, "[CLIENTE] M1 devolvió error: %s\n", response);
        return -1;
    }

    int parsed = sscanf(response,
                        "%45s %d %45s %d",
                        info->group_server_ipv6,
                        &info->group_server_port,
                        info->multicast_ipv6,
                        &info->multicast_port);

    if (parsed != 4) {
        fprintf(stderr,
                "[CLIENTE] Formato de respuesta inválido. Esperado: <ipv6> <tcp_port> <multicast_ipv6> <multicast_port>\n");
        return -1;
    }

    printf("[CLIENTE] Grupo %d localizado:\n", group_id);
    printf("          Servidor grupo: [%s]:%d\n",
           info->group_server_ipv6,
           info->group_server_port);
    printf("          Multicast: [%s]:%d\n",
           info->multicast_ipv6,
           info->multicast_port);

    return 0;
}

static int send_message_to_group(const char *client_id,
                                 int group_id,
                                 const char *base_message,
                                 const group_info_t *info,
                                 int sequence)
{
    int sockfd;
    char final_message[MESSAGE_SIZE];
    char ack[BUFFER_SIZE];
    long long timestamp_ms;

    if (client_id == NULL || base_message == NULL || info == NULL) {
        return -1;
    }

    memset(final_message, 0, sizeof(final_message));
    memset(ack, 0, sizeof(ack));

    timestamp_ms = current_time_ms();

    if (timestamp_ms < 0) {
        return -1;
    }

    snprintf(final_message,
             sizeof(final_message),
             "client_id=%s; group_id=%d; seq=%d; timestamp_ms=%lld; message=%s\n",
             client_id,
             group_id,
             sequence,
             timestamp_ms,
             base_message);

    printf("[CLIENTE %s] Conectando con servidor de grupo [%s]:%d...\n",
           client_id,
           info->group_server_ipv6,
           info->group_server_port);

    sockfd = connect_ipv6_tcp(info->group_server_ipv6,
                              info->group_server_port);

    if (sockfd < 0) {
        return -1;
    }

    printf("[CLIENTE %s] Enviando mensaje:\n%s",
           client_id,
           final_message);

    if (send(sockfd, final_message, strlen(final_message), 0) < 0) {
        perror("[CLIENTE] Error enviando mensaje al servidor de grupo");
        close(sockfd);
        return -1;
    }

    ssize_t bytes_read = recv(sockfd, ack, sizeof(ack) - 1, 0);

    if (bytes_read < 0) {
        perror("[CLIENTE] Error recibiendo ACK del servidor de grupo");
        close(sockfd);
        return -1;
    }

    if (bytes_read > 0) {
        ack[bytes_read] = '\0';
        remove_newline(ack);
        printf("[CLIENTE %s] ACK recibido: %s\n",
               client_id,
               ack);
    } else {
        printf("[CLIENTE %s] El servidor de grupo cerró sin ACK\n",
               client_id);
    }

    close(sockfd);

    return 0;
}

int main(int argc, char *argv[])
{
    const char *client_id;
    int group_id;
    const char *message;
    int interval_seconds = 0;
    int sequence = 1;

    group_info_t group_info;

    signal(SIGINT, handle_sigint);

    if (argc != 4 && argc != 5) {
        fprintf(stderr, "Uso:\n");
        fprintf(stderr, "  %s <client_id> <group_id> \"<mensaje>\"\n", argv[0]);
        fprintf(stderr, "  %s <client_id> <group_id> \"<mensaje>\" <intervalo_segundos>\n", argv[0]);
        fprintf(stderr, "\nEjemplos:\n");
        fprintf(stderr, "  %s client-1 1 \"Hola grupo 1\"\n", argv[0]);
        fprintf(stderr, "  %s client-1 1 \"Hola grupo 1\" 5\n", argv[0]);
        fprintf(stderr, "  %s client-7 2 \"Hola grupo 2\" 2\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    client_id = argv[1];
    group_id = atoi(argv[2]);
    message = argv[3];

    if (group_id <= 0) {
        fprintf(stderr, "[CLIENTE] group_id inválido\n");
        exit(EXIT_FAILURE);
    }

    if (argc == 5) {
        interval_seconds = atoi(argv[4]);

        if (interval_seconds <= 0) {
            fprintf(stderr, "[CLIENTE] intervalo_segundos inválido\n");
            exit(EXIT_FAILURE);
        }
    }

    printf("=============================================\n");
    printf(" Cliente emisor iniciado\n");
    printf(" client_id: %s\n", client_id);
    printf(" grupo: %d\n", group_id);
    printf(" mensaje base: %s\n", message);

    if (interval_seconds > 0) {
        printf(" intervalo: cada %d segundos\n", interval_seconds);
    } else {
        printf(" modo: envío único\n");
    }

    printf("=============================================\n\n");

    if (request_group_info(group_id, &group_info) < 0) {
        fprintf(stderr, "[CLIENTE] No se pudo obtener la información del grupo\n");
        exit(EXIT_FAILURE);
    }

    do {
        if (send_message_to_group(client_id,
                                  group_id,
                                  message,
                                  &group_info,
                                  sequence) < 0) {
            fprintf(stderr,
                    "[CLIENTE %s] Error enviando mensaje número %d\n",
                    client_id,
                    sequence);
        }

        sequence++;

        if (interval_seconds > 0 && running) {
            printf("[CLIENTE %s] Esperando %d segundos...\n\n",
                   client_id,
                   interval_seconds);
            sleep((unsigned int)interval_seconds);
        }

    } while (interval_seconds > 0 && running);

    printf("\n[CLIENTE %s] Finalizando cliente emisor\n", client_id);

    return 0;
}
