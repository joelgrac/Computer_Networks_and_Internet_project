#include "ndn.h"

int enviar_mensagem_udp(const char *server_ip, int server_port,
                        const char *msg, char *resposta, size_t resp_size)
{
    if (resposta) {
        resposta[0] = '\0';
    }
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("[UDP] Erro ao criar socket");
        return -1;
    }
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0) {
        perror("[UDP] Erro no inet_pton");
        close(sockfd);
        return -1;
    }
    ssize_t sent = sendto(sockfd, msg, strlen(msg), 0,
                          (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (sent < 0) {
        perror("[UDP] Erro ao enviar");
        close(sockfd);
        return -1;
    }
    printf("[UDP] Enviado -> %s:%d | \"%s\"\n", server_ip, server_port, msg);
    struct sockaddr_in fromaddr;
    socklen_t fromlen = sizeof(fromaddr);
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    (void) setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ssize_t n = recvfrom(sockfd,
                         (resposta ? resposta : NULL),
                         (resposta ? resp_size - 1 : 0),
                         0,
                         (struct sockaddr *)&fromaddr,
                         &fromlen);
    if (n > 0 && resposta) {
        resposta[n] = '\0';
        printf("[UDP] Resposta <- %s\n", resposta);
    }
    close(sockfd);
    return (n >= 0) ? (int)n : -1;
}

int obter_lista_nos(const char *net,
                    const char *server_ip, int server_port,
                    NodeInfo *nodes, int max_nodes)
{
    char msg[MAX_BUFFER];
    snprintf(msg, sizeof(msg), "NODES %s", net);
    char resposta[MAX_BUFFER];
    int ret = enviar_mensagem_udp(server_ip, server_port, msg, resposta, sizeof(resposta));
    if (ret < 0) {
        return 0;
    }
    char cabecalho[64];
    snprintf(cabecalho, sizeof(cabecalho), "NODESLIST %s", net);
    if (strncmp(resposta, cabecalho, strlen(cabecalho)) != 0) {
        if (strncmp(resposta, "NODESLIST", 9) != 0) {
            printf("[REG] Resposta inesperada do servidor: %s\n", resposta);
            return 0;
        }
    }
    char *ptr = strstr(resposta, "\n");
    if (!ptr) {
        return 0;
    }
    ptr++; // pula a linha do cabeçalho
    int count = 0;
    while (*ptr && count < max_nodes) {
        char ip_str[INET_ADDRSTRLEN];
        int porta = 0;
        if (sscanf(ptr, "%s %d", ip_str, &porta) == 2) {
            strncpy(nodes[count].ip, ip_str, INET_ADDRSTRLEN - 1);
            nodes[count].ip[INET_ADDRSTRLEN - 1] = '\0';
            nodes[count].tcp = porta;
            count++;
        }
        char *linha_fim = strstr(ptr, "\n");
        if (!linha_fim) {
            break;
        }
        ptr = linha_fim + 1;
    }
    if (count == 0) {
        printf("primeiro nó da rede\n");
    } else {
        printf("NODE LIST:\n");
        for (int i = 0; i < count; i++) {
            printf("IP: %s  PORTA: %d\n", nodes[i].ip, nodes[i].tcp);
        }
    }
    return count;
}

int registrar_no(const char *net, const char *meu_ip, int minha_porta_tcp,
                 const char *server_ip, int server_port)
{
    char msg[MAX_BUFFER];
    snprintf(msg, sizeof(msg), "REG %s %s %d\n", net, meu_ip, minha_porta_tcp);
    char resposta[MAX_BUFFER];
    int ret = enviar_mensagem_udp(server_ip, server_port, msg, resposta, sizeof(resposta));
    if (ret < 0)
        return -1;
    if (strncmp(resposta, "OKREG", 5) == 0) {
        printf("[REG] Registrado com sucesso na rede %s\n", net);
        return 0;
    } else {
        printf("[REG] Falha no registro. Resposta do servidor: %s\n", resposta);
        return -1;
    }
}

int remover_registro(const char *net, const char *meu_ip, int minha_porta_tcp,
                     const char *server_ip, int server_port)
{
    char msg[MAX_BUFFER];
    snprintf(msg, sizeof(msg), "UNREG %s %s %d\n", net, meu_ip, minha_porta_tcp);
    char resposta[MAX_BUFFER];
    int ret = enviar_mensagem_udp(server_ip, server_port, msg, resposta, sizeof(resposta));
    if (ret < 0)
        return -1;
    if (strncmp(resposta, "OKUNREG", 7) == 0) {
        printf("[REG] Remoção de registro bem-sucedida\n");
        return 0;
    } else {
        printf("[REG] Falha ao remover registro. Resposta: %s\n", resposta);
        return -1;
    }
}
