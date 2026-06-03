#pragma once
#include <string>
#include <optional>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

constexpr uint16_t PORT = 55000;

enum class PktType : uint8_t { ATTACK=1, RESULT=2, READY=3, GAMEOVER=4 };
enum class Bomba   : uint8_t { PRECISAO=1, PADRAO=2, OGIVA=3 };
enum class Result  : uint8_t { ERRO=0, ACERTO=1, VITORIA=2 };

struct Packet {
    PktType type;
    uint8_t player;
    union {
        struct { Bomba tipo; double x, y, raio;   } __attribute__((packed)) attack;
        struct { Result res; uint8_t ovos_vivos;  } __attribute__((packed)) result;
        struct { uint8_t num_torres;              } ready;
        struct { uint8_t winner;                  } gameover;
    };
} __attribute__((packed));

inline double raio(Bomba b) {
    if (b == Bomba::PRECISAO) return 20.f;
    if (b == Bomba::OGIVA)    return 80.f;
    return 45.f;
}

struct Net {
    int srv = -1, peer = -1;
    bool connected = false;
    std::string error;

    ~Net() { close(srv); close(peer); }

    // Funcao Server Side
    bool startServer() {
        // Cria o socket usando IPv4 e TCP
        srv = ::socket(AF_INET, SOCK_STREAM, 0);
        // Cria a variável que serve de flag usada abaixo
        int opt = 1;
        // Configura o socket pra reusar o endereço em caso em crash
        setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        // Cria a estrutura vazia para armazenar dados do servidor
        sockaddr_in a{};
        a.sin_family = AF_INET; // Endereços IPv4 
        a.sin_port = htons(PORT); // Porta do servidor
        // Servidor aceita conexão de qualquer interface de rede da máquina
        a.sin_addr.s_addr = INADDR_ANY;
        // Atrela o socket criado ao IP configurado acima
        // Coloca o socket no modo de escuta e permite 1 conexão na fila de espera
        // Se um dos dois falhar, retorna falsa
        if (bind(srv, (sockaddr*)&a, sizeof(a)) || listen(srv, 1)) return false;
        // Pega as flags atuais e adiciona a não bloqueante (assíncrona)
        fcntl(srv, F_SETFL, fcntl(srv, F_GETFL) | O_NONBLOCK);
        // Se passou por todos os passos, retorna a flag true
        return true;
    }

    bool accept() {
        if (connected) return true;
        peer = ::accept(srv, nullptr, nullptr);
        if (peer < 0) return false;
        fcntl(peer, F_SETFL, fcntl(peer, F_GETFL) | O_NONBLOCK);
        connected = true; return true;
    }

    bool connect(const std::string& ip) {
        // Se já estiver conectado, não faz nada
        if (connected) return true;
#ifndef DEV_MODE
        if (ip == "127.0.0.1") { error = "Loopback bloqueado."; return false; }
#endif
        // Verifica se é valor inicial (socket não existe)
        if (peer < 0) {
            // Cria o socket usando IPv4 e TCP
            // peer armazena o ID do socket
            peer = ::socket(AF_INET, SOCK_STREAM, 0);
            // Transforma o socket do cliente em não bloqueante
            fcntl(peer, F_SETFL, fcntl(peer, F_GETFL) | O_NONBLOCK);
        }
        // Cria a estrutura vazia do servidor
        sockaddr_in a{};
        a.sin_family = AF_INET; // IPv4
        a.sin_port = htons(PORT); // Porta do servidor de destino 
        // Converter o IP digitado para binário
        inet_pton(AF_INET, ip.c_str(), &a.sin_addr);
        // Tenta conectar com o IP e porta
        int r = ::connect(peer, (sockaddr*)&a, sizeof(a));
        // Se conectou, já existia conexão
        if (r == 0 || errno == EISCONN) { connected = true; return true; }
        // Verifica flags em progresso
        if (errno == EINPROGRESS || errno == EALREADY) {
            // Checa se já conectou via SO_ERROR
            int err = 0;
            socklen_t len = sizeof(err);
            // Busca opções do socket para checar o erro atual
            getsockopt(peer, SOL_SOCKET, SO_ERROR, &err, &len);
            // Se não retornou erros, a conexão foi bem sucedida
            if (err == 0 && errno == EISCONN) { connected = true; return true; }
        }
        // Se chegou aqui, a conexão não foi feita
        return false;
    }

    bool send(const Packet& p) {
        return ::send(peer, &p, sizeof(p), MSG_NOSIGNAL) == sizeof(p);
    }

    std::optional<Packet> recv() {
        Packet p{};
        ssize_t n = ::recv(peer, &p, sizeof(p), MSG_DONTWAIT);
        if (n == sizeof(p)) return p;
        if (n == 0) connected = false;
        return std::nullopt;
    }

    void resetPeer() {
        if (peer >= 0) { ::close(peer); peer = -1; }
        connected = false;
        error.clear();
    }
};