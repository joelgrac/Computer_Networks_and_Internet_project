#include "ndn.h"
#include <time.h>
extern int  num_objetos_locais;
extern char objetos_locais[][MAX_BUFFER];


//Interpreta os comandos digitados no terminal.

void interpretar_comando(const char *comando,
                         const char *server_ip, int server_port,
                         const char *meu_ip, int minha_porta_tcp)
{
    char net[16], ip_dest[INET_ADDRSTRLEN];
    int port_dest;

    if (strncmp(comando, "join ", 5) == 0) {
        if (sscanf(comando, "join %15s", net) == 1) {
            printf(">>> Comando: join (rede=%s)\n", net);
            NodeInfo lista[100];
            int qtd = obter_lista_nos(net, server_ip, server_port, lista, 100);
            if (qtd == 0) {
                // Lista vazia → primeiro nó da rede
                printf("[JOIN] Rede %s vazia. Sou o primeiro nó.\n", net);
                registrar_no(net, meu_ip, minha_porta_tcp, server_ip, server_port);
            } else {
                // Exibe a lista e conecta a um nó aleatório
                srand((unsigned)time(NULL));
                int idx = rand() % qtd;
                printf("[JOIN] Conectando ao nó %s:%d (aleatório)\n", lista[idx].ip, lista[idx].tcp);
                int sockfd = conectar_com_no(lista[idx].ip, lista[idx].tcp);
                if (sockfd >= 0) {
                    enviar_mensagem_entry(sockfd, meu_ip, minha_porta_tcp);
                    char safe_msg[MAX_BUFFER];
                    ssize_t n = read(sockfd, safe_msg, MAX_BUFFER - 1);
                    if (n > 0) {
                        safe_msg[n] = '\0';
                        printf("[JOIN] Resposta SAFE recebida: %s\n", safe_msg);
                        // Aqui, se necessário, pode-se adicionar o nó à lista de vizinhos com base na resposta SAFE.
                    } else {
                        printf("[JOIN] Falha ao receber resposta SAFE.\n");
                    }
                    close(sockfd);
                }
                registrar_no(net, meu_ip, minha_porta_tcp, server_ip, server_port);
            }
        }
    }
    
    else if (strncmp(comando, "direct join ", 12) == 0) {
        if (sscanf(comando, "direct join %15s %15s %d", net, ip_dest, &port_dest) == 3) {
            printf(">>> Comando: direct join (rede=%s, nó=%s:%d)\n", net, ip_dest, port_dest);
            if (strcmp(ip_dest, "0.0.0.0") == 0 && port_dest == 0) {
                // Primeiro nó
                printf("[DIRECT JOIN] Sou o primeiro nó da rede %s.\n", net);
                registrar_no(net, meu_ip, minha_porta_tcp, server_ip, server_port);
            } else {
                int sockfd = conectar_com_no(ip_dest, port_dest);
                if (sockfd >= 0) {
                    enviar_mensagem_entry(sockfd, meu_ip, minha_porta_tcp);
                    char safe_msg[MAX_BUFFER];
                    ssize_t n = read(sockfd, safe_msg, MAX_BUFFER - 1);
                    if (n > 0) {
                        safe_msg[n] = '\0';
                        printf("[DIRECT JOIN] Resposta SAFE recebida: %s\n", safe_msg);
                        adicionar_vizinho(ip_dest, port_dest, 1);
                    } else {
                        printf("[DIRECT JOIN] Falha ao receber resposta SAFE.\n");
                    }
                    close(sockfd);
                }
                registrar_no(net, meu_ip, minha_porta_tcp, server_ip, server_port);
            }
        }
        else {
            printf("Uso: direct join <net> <connectIP> <connectTCP>\n");
        }
    }
    
    else if (strncmp(comando, "show topology", 13) == 0) {
        printf(">>> Comando: show topology\n");
        mostrar_topologia();
    }
    else if (strncmp(comando, "create ", 7) == 0) {
        char nome[MAX_BUFFER];
        if (sscanf(comando, "create %s", nome) == 1) {
            armazenar_objeto_local(nome);
        } else {
            printf("Uso: create <nomeObjeto>\n");
        }
    }
    else if (strncmp(comando, "delete ", 7) == 0) {
        char nome[MAX_BUFFER];
        if (sscanf(comando, "delete %s", nome) == 1) {
            remover_objeto_local(nome);
        } else {
            printf("Uso: delete <nomeObjeto>\n");
        }
    }
    else if (strncmp(comando, "show names", 10) == 0) {
        listar_objetos_locais();
    }
    else if (strncmp(comando, "retrieve ", 9) == 0) {
        char nome[MAX_BUFFER];
        if (sscanf(comando, "retrieve %s", nome) == 1) {
            printf("[RETRIEVE] Buscando objeto '%s'...\n", nome);
            int found = 0;
            // Tenta cada vizinho até encontrar o objeto
            for (int i = 0; i < numero_vizinhos && !found; i++) {
                int sock = conectar_com_no(lista_vizinhos[i].ip, lista_vizinhos[i].tcp);
                if (sock < 0)
                    continue;
                enviar_mensagem_interest(sock, nome);

                char resp[MAX_BUFFER];
                ssize_t n = read(sock, resp, MAX_BUFFER - 1);
                if (n > 0) {
                    resp[n] = '\0';
                    printf("[RETRIEVE] Resposta do vizinho %s:%d: %s\n",
                           lista_vizinhos[i].ip, lista_vizinhos[i].tcp, resp);
                    char cmd[16], param[128];
                    if (sscanf(resp, "%15s %127s", cmd, param) == 2) {
                        if (strcmp(cmd, "OBJECT") == 0) {
                            armazenar_objeto_local(param);
                            printf("[RETRIEVE] Objeto '%s' encontrado e armazenado!\n", param);
                            found = 1;
                        } else if (strcmp(cmd, "NOOBJECT") == 0) {
                            printf("[RETRIEVE] Vizinho %s:%d não possui '%s'.\n",
                                   lista_vizinhos[i].ip, lista_vizinhos[i].tcp, param);
                        }
                    }
                }
                close(sock);
            }
            if (!found) {
                printf("[RETRIEVE] Nenhum vizinho retornou o objeto '%s'.\n", nome);
            }
        } else {
            printf("Uso: retrieve <nomeObjeto>\n");
        }
    }
    else if (strncmp(comando, "leave ", 6) == 0) {
        char net[16];
        if (sscanf(comando, "leave %15s", net) != 1) {
            printf("Uso: leave <nomeDaRede>\n");
            return;
        }
        printf(">>> Comando: leave (rede=%s)\n", net);
    
        // Se for salvaguarda, transferir dados e/ou conectar vizinhos
        if (sou_salvaguarda) {
            printf("[LEAVE] Nó é salvaguarda. Iniciando procedimentos de transferência.\n");
            printf("[LEAVE/SALVAGUARDA]Objetos locais:\n");
            listar_objetos_locais();
            // 1) Transferir dados para outro(s) vizinho(s)
            if (numero_vizinhos > 0) {
                //transfere todos os objetos locais para o primeiro vizinho
                int sock = conectar_com_no(lista_vizinhos[0].ip, lista_vizinhos[0].tcp);
                if (sock >= 0) {
    
                    for (int i = 0; i < num_objetos_locais; i++) {
                        enviar_mensagem_object(sock, objetos_locais[i]);
                    }
                    close(sock);
                }
            }
    
            // 2) conectar vizinhos entre si  (manter a topologia)
            for (int i = 0; i < numero_vizinhos; i++) {
                for (int j = i+1; j < numero_vizinhos; j++) {
                    // Conecta vizinhos
                    int sock = conectar_com_no(lista_vizinhos[i].ip, lista_vizinhos[i].tcp);
                    if (sock >= 0) {
                        enviar_mensagem_entry(sock,
                                              lista_vizinhos[j].ip,
                                              lista_vizinhos[j].tcp);
                        close(sock);
                    }
                }
            }
        }
    
        // Agora faz o LEAVE normal (avisando os vizinhos)
        for (int i = 0; i < numero_vizinhos; i++) {
            int sock = conectar_com_no(lista_vizinhos[i].ip, lista_vizinhos[i].tcp);
            if (sock < 0) {
                printf("[LEAVE] Erro ao conectar com vizinho %s:%d\n",
                       lista_vizinhos[i].ip, lista_vizinhos[i].tcp);
                continue;
            }
            // Envia LEAVE
            enviar_mensagem_leave(sock, meu_ip, minha_porta_tcp);
    
            // Aguarda SAFE
            char resp[MAX_BUFFER];
            ssize_t n = read(sock, resp, MAX_BUFFER - 1);
            if (n > 0) {
                resp[n] = '\0';
                printf("[LEAVE] Resposta do vizinho %s:%d -> %s\n",
                       lista_vizinhos[i].ip, lista_vizinhos[i].tcp, resp);
            } else {
                printf("[LEAVE] Falha ao receber SAFE do vizinho %s:%d\n",
                       lista_vizinhos[i].ip, lista_vizinhos[i].tcp);
            }
            close(sock);
        }
    
        // Remove registro no servidor
        remover_registro(net, meu_ip, minha_porta_tcp, server_ip, server_port);
    
        // Limpa lista de vizinhos
        numero_vizinhos = 0;
    
        printf("[LEAVE] Nó removido da rede '%s'.\n", net);
    }
    
     else {
        printf("Comando não reconhecido ou ainda não implementado.\n");
    }
}
 
void mostrar_topologia(void) {
    printf(">>> Topologia Atual:\n");
    if (numero_vizinhos == 0) {
        printf("Nenhum vizinho conectado.\n");
    }
    for (int i = 0; i < numero_vizinhos; i++) {
        printf("%s: %s:%d\n",
               lista_vizinhos[i].externo ? "Externo" : "Interno",
               lista_vizinhos[i].ip, lista_vizinhos[i].tcp);
    }
}
 
void remover_vizinho(const char *ip, int tcp) {
    for (int i = 0; i < numero_vizinhos; i++) {
        if (strcmp(lista_vizinhos[i].ip, ip) == 0 && lista_vizinhos[i].tcp == tcp) {
            for (int j = i; j < numero_vizinhos - 1; j++) {
                lista_vizinhos[j] = lista_vizinhos[j + 1];
            }
            numero_vizinhos--;
            printf("[TOPO] Vizinho removido: %s:%d\n", ip, tcp);
            return;
        }
    }
    printf("[TOPO] Vizinho não encontrado: %s:%d\n", ip, tcp);
}
