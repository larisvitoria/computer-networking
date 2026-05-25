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
#include <sys/select.h>

// ── Protocolo ────────────────────────────────────────────────

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
        struct { uint8_t num_torres;                } ready;
        struct { uint8_t winner;                    } gameover;
    };
} __attribute__((packed));

inline double raio(Bomba b) {
    if (b == Bomba::PRECISAO) return 20.f;
    if (b == Bomba::OGIVA)    return 80.f;
    return 45.f;
}

// ── Socket ───────────────────────────────────────────────────

struct Net {
    int srv = -1, peer = -1;
    bool connected = false;
    std::string error;
    float connect_timer = 0.f;
    bool connecting = false;
    constexpr static float CONNECT_TIMEOUT = 3.f;  // segundos

    ~Net() { close(srv); close(peer); }

    bool startServer() {
        srv = ::socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1; setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        sockaddr_in a{}; a.sin_family = AF_INET; a.sin_port = htons(PORT);
        a.sin_addr.s_addr = INADDR_ANY;
        if (bind(srv, (sockaddr*)&a, sizeof(a)) || listen(srv, 1)) return false;
        fcntl(srv, F_SETFL, fcntl(srv, F_GETFL) | O_NONBLOCK);
        return true;
    }

    bool accept() {
        peer = ::accept(srv, nullptr, nullptr);
        if (peer < 0) return false;
        fcntl(peer, F_SETFL, fcntl(peer, F_GETFL) | O_NONBLOCK);
        connected = true; return true;
    }

    // Chame tickConnect(dt) todo frame enquanto tenta conectar como cliente
    void tickConnect(float dt) {
        if (!connecting || connected) return;
        connect_timer += dt;
        // Checa se conectou (select com timeout zero)
        fd_set wfds, efds;
        FD_ZERO(&wfds); FD_ZERO(&efds);
        FD_SET(peer, &wfds); FD_SET(peer, &efds);
        timeval tv{0,0};
        int r = select(peer+1, nullptr, &wfds, &efds, &tv);
        if (r > 0) {
            int err = 0; socklen_t len = sizeof(err);
            getsockopt(peer, SOL_SOCKET, SO_ERROR, &err, &len);
            if (err == 0) {
                connected = true; connecting = false; connect_timer = 0.f;
            } else {
                error = "Não foi possível conectar (IP inválido ou sem resposta).";
                connecting = false; connect_timer = 0.f;
                resetPeer();
            }
        } else if (connect_timer >= CONNECT_TIMEOUT) {
            error = "Timeout: sem resposta do servidor.";
            connecting = false; connect_timer = 0.f;
            resetPeer();
        }
    }

    bool connect(const std::string& ip) {
        if (connected) return true;
#ifndef DEV_MODE
        if (ip == "127.0.0.1") { error = "Loopback bloqueado."; return false; }
#endif
        if (peer < 0) {
            peer = ::socket(AF_INET, SOCK_STREAM, 0);
            fcntl(peer, F_SETFL, fcntl(peer, F_GETFL) | O_NONBLOCK);
        }
        sockaddr_in a{}; a.sin_family = AF_INET; a.sin_port = htons(PORT);
        if (inet_pton(AF_INET, ip.c_str(), &a.sin_addr) <= 0) {
            error = "IP inválido: " + ip;
            resetPeer(); return false;
        }
        int r = ::connect(peer, (sockaddr*)&a, sizeof(a));
        if (r == 0 || errno == EISCONN) { connected = true; connecting = false; return true; }
        if (errno == EINPROGRESS || errno == EALREADY) {
            connecting = true; connect_timer = 0.f; return false;  // aguarda tickConnect
        }
        error = "Falha ao conectar: " + std::string(strerror(errno));
        resetPeer(); return false;
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
