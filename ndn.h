#ifndef NDN_H
#define NDN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h> 

#define DEFAULT_REG_IP "192.136.138.142"
#define DEFAULT_REG_UDP 59000
#define MAX_CLIENTES 100
#define MAX_BUFFER 1024
#define MAX_OBJETOS_LOCAIS 100
#define PORTA_UDP 59000
extern int sou_salvaguarda;
extern int  num_objetos_locais;
extern char objetos_locais[MAX_OBJETOS_LOCAIS][MAX_BUFFER];
extern int sou_salvaguarda;
extern int g_cacheSize;

// Estrutura para guardar informações de nós recebidos do servidor ("NODESLIST")
typedef struct {
    char ip[INET_ADDRSTRLEN];
    int  tcp;
} NodeInfo;

// Estrutura para thread do servidor TCP (este nó escuta conexões de outros nós)
typedef struct {
    char ip[INET_ADDRSTRLEN];
    int  tcp_port;
} ServidorTCPArgs;

// Estrutura para vizinhos
typedef struct {
    char ip[INET_ADDRSTRLEN];
    int tcp;
    int externo; // 1 se for vizinho externo, 0 se interno
} Vizinho;

// Variáveis globais (definidas em ndn_tcp.c)
extern int numero_vizinhos;
extern Vizinho lista_vizinhos[MAX_CLIENTES];

// Funções de ndn_udp.c

int  enviar_mensagem_udp(const char *server_ip, int server_port,
                         const char *msg, char *resposta, size_t resp_size);
int  obter_lista_nos(const char *net,
                     const char *server_ip, int server_port,
                     NodeInfo *nodes, int max_nodes);
int  registrar_no(const char *net, const char *meu_ip, int minha_porta_tcp,
                  const char *server_ip, int server_port);
int  remover_registro(const char *net, const char *meu_ip, int minha_porta_tcp,
                      const char *server_ip, int server_port);

// Funções para topologia
void mostrar_topologia(void);
void adicionar_vizinho(const char *ip, int tcp, int externo);
void remover_vizinho(const char *ip, int tcp);


// Funções de ndn_tcp.c

void *thread_servidor_tcp(void *arg);
int  conectar_com_no(const char *ip, int porta);
void iniciar_cliente_tcp(const char *ip, int port);

// Mensagens de topologia
void enviar_mensagem_entry(int sockfd, const char *ip, int tcp);
void enviar_mensagem_safe(int sockfd, const char *ip, int tcp);

// Mensagens NDN
void enviar_mensagem_interest(int sockfd, const char *name);
void enviar_mensagem_object(int sockfd, const char *name);
void enviar_mensagem_noobject(int sockfd, const char *name);
void enviar_mensagem_leave(int sockfd, const char *ip, int tcp);


// Funções de commands.c

void interpretar_comando(const char *comando,
                         const char *server_ip, int server_port,
                         const char *meu_ip, int minha_porta_tcp);


// Funções de armazenamento (storage.c)

int  possui_objeto_local(const char *nome);
void armazenar_objeto_local(const char *nome);
void remover_objeto_local(const char *nome);
void listar_objetos_locais(void);

#endif // NDN_H
