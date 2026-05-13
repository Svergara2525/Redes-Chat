/*
 * cliente_recv.c
 *
 * Cliente receptor multicast IPv6.
 *
 * Se ejecuta en el HOST y se suscribe al multicast del grupo indicado.
 *
 * Además:
 *   - Calcula la latencia extremo a extremo usando timestamp_ms.
 *   - Comprueba si la latencia cumple el requisito < 200 ms.
 *   - Guarda las latencias en CSV:
 *       latencias_grupo_1.csv
 *       latencias_grupo_2.csv
 *
 * Grupo 1:
 *   multicast ff15::1 puerto 5001
 *
 * Grupo 2:
 *   multicast ff15::2 puerto 5002
 *
 * Compilación:
 *   gcc -Wall -Wextra -o cliente_recv cliente_recv.c
 *
 * Uso:
 *   ./cliente_recv <client_id> <group_id>
 *   ./cliente_recv <client_id> <group_id> <interfaz>
 *
 * Ejemplos:
 *   ./cliente_recv client-1 1
 *   ./cliente_recv client-1 2
 *   ./cliente_recv client-1 1 br6
 *   ./cliente_recv client-1 2 br6
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
#include <net/if.h>

#define BUFFER_SIZE 2048

#define DEFAULT_INTERFACE "br6"

#define GROUP_1_ID 1
#define GROUP_1_MULTICAST_IPV6 "ff15::1"
#define GROUP_1_MULTICAST_PORT 5001

#define GROUP_2_ID 2
#define GROUP_2_MULTICAST_IPV6 "ff15::2"
#define GROUP_2_MULTICAST_PORT 5002

static volatile sig_atomic_t running = 1;

static long long current_time_ms(void)
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL) != 0) {
        perror("[RECV] Error en gettimeofday");
        return -1;
    }

    return ((long long)tv.tv_sec * 1000LL) + ((long long)tv.tv_usec / 1000LL);
}

static long long extract_timestamp_ms(const char *message)
{
    const char *key;
    long long timestamp_ms;

    if (message == NULL) {
        return -1;
    }

    key = strstr(message, "timestamp_ms=");

    if (key == NULL) {
        return -1;
    }

    if (sscanf(key, "timestamp_ms=%lld", &timestamp_ms) != 1) {
        return -1;
    }

    return timestamp_ms;
}

static void save_latency_to_csv(const char *client_id,
                                int group_id,
                                long long latency_ms)
{
    char filename[64];
    FILE *file;
    int file_exists;

    if (client_id == NULL || latency_ms < 0) {
        return;
    }

    snprintf(filename, sizeof(filename), "latencias_grupo_%d.csv", group_id);

    file_exists = (access(filename, F_OK) == 0);

    file = fopen(filename, "a");

    if (file == NULL) {
        perror("[RECV] Error abriendo fichero CSV de latencias");
        return;
    }

    if (!file_exists) {
        fprintf(file, "client_id,group_id,latency_ms,meets_200ms\n");
    }

    fprintf(file,
            "%s,%d,%lld,%s\n",
            client_id,
            group_id,
            latency_ms,
            latency_ms < 200 ? "yes" : "no");

    fclose(file);
}

static void handle_sigint(int sig)
{
    (void)sig;
    running = 0;
}

static int get_group_config(int group_id,
                            const char **multicast_ipv6,
                            int *multicast_port)
{
    if (multicast_ipv6 == NULL || multicast_port == NULL) {
        return -1;
    }

    if (group_id == GROUP_1_ID) {
        *multicast_ipv6 = GROUP_1_MULTICAST_IPV6;
        *multicast_port = GROUP_1_MULTICAST_PORT;
        return 0;
    }

    if (group_id == GROUP_2_ID) {
        *multicast_ipv6 = GROUP_2_MULTICAST_IPV6;
        *multicast_port = GROUP_2_MULTICAST_PORT;
        return 0;
    }

    return -1;
}

static int create_multicast_receiver_socket(const char *multicast_ipv6,
                                            int multicast_port,
                                            const char *interface_name)
{
    int sockfd;
    int optval = 1;
    struct sockaddr_in6 local_addr;
    struct ipv6_mreq mreq;
    unsigned int ifindex;

    if (multicast_ipv6 == NULL || interface_name == NULL) {
        fprintf(stderr, "[RECV] Parámetros inválidos\n");
        return -1;
    }

    ifindex = if_nametoindex(interface_name);

    if (ifindex == 0) {
        fprintf(stderr,
                "[RECV] No se pudo obtener el índice de la interfaz '%s': %s\n",
                interface_name,
                strerror(errno));
        fprintf(stderr,
                "[RECV] Comprueba el nombre con: ip -6 addr\n");
        return -1;
    }

    sockfd = socket(AF_INET6, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        perror("[RECV] Error creando socket UDP IPv6");
        return -1;
    }

    /*
     * Necesario para poder lanzar varios clientes receptores
     * escuchando el mismo puerto multicast.
     */
    if (setsockopt(sockfd,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &optval,
                   sizeof(optval)) < 0) {
        perror("[RECV] Error en SO_REUSEADDR");
        close(sockfd);
        return -1;
    }

#ifdef SO_REUSEPORT
    if (setsockopt(sockfd,
                   SOL_SOCKET,
                   SO_REUSEPORT,
                   &optval,
                   sizeof(optval)) < 0) {
        perror("[RECV] Aviso: no se pudo configurar SO_REUSEPORT");
    }
#endif

    memset(&local_addr, 0, sizeof(local_addr));

    local_addr.sin6_family = AF_INET6;
    local_addr.sin6_addr = in6addr_any;
    local_addr.sin6_port = htons((uint16_t)multicast_port);

    /*
     * El receptor escucha en el puerto multicast.
     * No se hace bind a ff15::1 o ff15::2 directamente,
     * sino a :: y luego se une al grupo multicast con IPV6_JOIN_GROUP.
     */
    if (bind(sockfd,
             (struct sockaddr *)&local_addr,
             sizeof(local_addr)) < 0) {
        perror("[RECV] Error en bind UDP");
        close(sockfd);
        return -1;
    }

    memset(&mreq, 0, sizeof(mreq));

    if (inet_pton(AF_INET6,
                  multicast_ipv6,
                  &mreq.ipv6mr_multiaddr) != 1) {
        fprintf(stderr,
                "[RECV] Dirección multicast IPv6 inválida: %s\n",
                multicast_ipv6);
        close(sockfd);
        return -1;
    }

    mreq.ipv6mr_interface = ifindex;

    if (setsockopt(sockfd,
                   IPPROTO_IPV6,
                   IPV6_JOIN_GROUP,
                   &mreq,
                   sizeof(mreq)) < 0) {
        perror("[RECV] Error en IPV6_JOIN_GROUP");
        fprintf(stderr,
                "[RECV] Interfaz usada: %s índice %u\n",
                interface_name,
                ifindex);
        close(sockfd);
        return -1;
    }

    printf("[RECV] Suscripción multicast correcta\n");
    printf("[RECV] Grupo multicast: [%s]:%d\n",
           multicast_ipv6,
           multicast_port);
    printf("[RECV] Interfaz: %s índice %u\n",
           interface_name,
           ifindex);

    return sockfd;
}

int main(int argc, char *argv[])
{
    const char *client_id;
    int group_id;
    const char *interface_name;

    const char *multicast_ipv6;
    int multicast_port;

    int sockfd;
    char buffer[BUFFER_SIZE];

    signal(SIGINT, handle_sigint);

    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Uso:\n");
        fprintf(stderr, "  %s <client_id> <group_id>\n", argv[0]);
        fprintf(stderr, "  %s <client_id> <group_id> <interfaz>\n", argv[0]);
        fprintf(stderr, "\nEjemplos:\n");
        fprintf(stderr, "  %s client-1 1\n", argv[0]);
        fprintf(stderr, "  %s client-1 2\n", argv[0]);
        fprintf(stderr, "  %s client-1 1 br6\n", argv[0]);
        fprintf(stderr, "  %s client-1 2 br6\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    client_id = argv[1];
    group_id = atoi(argv[2]);

    if (argc == 4) {
        interface_name = argv[3];
    } else {
        interface_name = DEFAULT_INTERFACE;
    }

    if (get_group_config(group_id,
                         &multicast_ipv6,
                         &multicast_port) < 0) {
        fprintf(stderr,
                "[RECV %s] Grupo inválido: %d. Usa grupo 1 o 2.\n",
                client_id,
                group_id);
        exit(EXIT_FAILURE);
    }

    printf("=============================================\n");
    printf(" Cliente receptor multicast iniciado\n");
    printf(" client_id: %s\n", client_id);
    printf(" grupo: %d\n", group_id);
    printf(" multicast: [%s]:%d\n", multicast_ipv6, multicast_port);
    printf(" interfaz: %s\n", interface_name);
    printf("=============================================\n\n");

    sockfd = create_multicast_receiver_socket(multicast_ipv6,
                                              multicast_port,
                                              interface_name);

    if (sockfd < 0) {
        fprintf(stderr,
                "[RECV %s] No se pudo crear el receptor multicast\n",
                client_id);
        exit(EXIT_FAILURE);
    }

    printf("[RECV %s] Esperando mensajes multicast del grupo %d...\n\n",
           client_id,
           group_id);

    while (running) {
        struct sockaddr_in6 sender_addr;
        socklen_t sender_len = sizeof(sender_addr);
        char sender_ip[INET6_ADDRSTRLEN];
        int sender_port;

        memset(buffer, 0, sizeof(buffer));
        memset(&sender_addr, 0, sizeof(sender_addr));
        memset(sender_ip, 0, sizeof(sender_ip));

        ssize_t bytes_read = recvfrom(sockfd,
                                      buffer,
                                      sizeof(buffer) - 1,
                                      0,
                                      (struct sockaddr *)&sender_addr,
                                      &sender_len);

        if (bytes_read < 0) {
            if (errno == EINTR && !running) {
                break;
            }

            perror("[RECV] Error en recvfrom");
            continue;
        }

        buffer[bytes_read] = '\0';

        inet_ntop(AF_INET6,
                  &sender_addr.sin6_addr,
                  sender_ip,
                  sizeof(sender_ip));

        sender_port = ntohs(sender_addr.sin6_port);

        long long recv_time_ms = current_time_ms();
        long long send_time_ms = extract_timestamp_ms(buffer);
        long long latency_ms = -1;

        if (recv_time_ms >= 0 && send_time_ms >= 0) {
            latency_ms = recv_time_ms - send_time_ms;
        }

        printf("--------------------------------------------------\n");
        printf("[RECV %s] Mensaje recibido del grupo %d\n",
               client_id,
               group_id);
        printf("[RECV %s] Origen: [%s]:%d\n",
               client_id,
               sender_ip,
               sender_port);

        if (latency_ms >= 0) {
            printf("[RECV %s] Latencia extremo a extremo: %lld ms\n",
                   client_id,
                   latency_ms);

            save_latency_to_csv(client_id, group_id, latency_ms);

            if (latency_ms < 200) {
                printf("[RECV %s] Cumple requisito < 200 ms\n",
                       client_id);
            } else {
                printf("[RECV %s] NO cumple requisito < 200 ms\n",
                       client_id);
            }
        } else {
            printf("[RECV %s] No se pudo calcular latencia, falta timestamp_ms\n",
                   client_id);
        }

        printf("[RECV %s] Contenido:\n%s",
               client_id,
               buffer);
        printf("--------------------------------------------------\n\n");
    }

    printf("\n[RECV %s] Cerrando receptor multicast...\n", client_id);

    close(sockfd);

    return 0;
}
