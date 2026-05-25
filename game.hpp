#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

#include "net.hpp"

// ── Pici Campus Map ──────────────────────────────────────────

constexpr float  MAP_W=800.f, MAP_H=600.f;

struct Ponto {
    uint8_t id;
    const char* nome;
    const char* sigla;
    float x;
    float y;
};

inline const std::vector<Ponto>& pontos() {
    static const std::vector<Ponto> v = {
        {1,  "Açude",                  "ACUDE", 602.f, 191.f},
        {2,  "Biblioteca CT",          "BCCP",   449.f, 155.f},
        {3,  "Educação Física",        "EF",    508.f, 526.f},
        {4,  "RU Velho",               "RUV",   511.f, 234.f},
        {5,  "ICA",                    "ICA",   534.f, 269.f},
        {6,  "RU Novo",                "RUN",   297.f, 313.f},
        {7,  "Sistemas e Mídias Digitais", "SMD", 248.f, 371.f},
        {8,  "DETI",                   "DETI",  310.f, 244.f},
        {9,  "Bloco 707",              "B707",  403.f, 187.f},
        {10, "Centro de Tecnologia",   "CT",    383.f, 215.f},
        {11, "Cantina da Química",     "CANTQ", 344.f, 295.f},
        {12, "Centro de Ciências",     "CC",    456.f, 280.f},
        {13, "Ciências Agrárias",      "CCA",   327.f, 181.f},
        {14, "Entrada Humberto Monte", "EHM",   649.f, 32.f},
        {15, "Entrada Mister Hull",    "EMH",   538.f, 25.f},
        {16, "Entrada Pernambuco",     "EP",    390.f, 532.f},
        {17, "Cantina da Física",      "CANTF",    402.f, 326.f}
    };

    return v;
}

inline const Ponto* getPonto(uint8_t id) {
    for (const auto& p : pontos()) {
        if (p.id == id) return &p;
    }

    return nullptr;
}

// ── Distance in screen pixels ────────────────────────────────

inline float distPx(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;

    return std::sqrt(dx * dx + dy * dy);
}

// ── Game state ───────────────────────────────────────────────

constexpr int MAX_OVOS = 3;

enum class Fase {
    CONNECT,
    PLACE,
    WAITING,
    BATTLE,
    GAMEOVER
};

struct Ovo {
    uint8_t ponto_id;
    double x;
    double y;
    bool destruido = false;
};

struct Explosion {
    double x;
    double y;
    double raio;
    bool hit;
    float vida = 3.f;
};

struct Game {
    Fase    fase      = Fase::CONNECT;
    uint8_t player    = 1;
    bool    meu_turno = false;
    uint8_t winner    = 0;

    std::vector<Ovo> ovos;
    int op_ovos = MAX_OVOS;

    int ammo[3] = {5, 3, 1};   // PRECISAO, PADRAO, AMPLIADA
    int arma    = 1;

    std::vector<Explosion> explosions;

    bool placeOvo(uint8_t pid) {
        if ((int)ovos.size() >= MAX_OVOS) return false;

        for (const auto& o : ovos) {
            if (o.ponto_id == pid) return false;
        }

        const Ponto* p = getPonto(pid);
        if (!p) return false;

        ovos.push_back({pid, p->x, p->y});

        return true;
    }

    Packet buildAttack(float x, float y) {
        Bomba tipo =
            (arma == 0) ? Bomba::PRECISAO :
            (arma == 2) ? Bomba::OGIVA :
                          Bomba::PADRAO;

        Packet p{};
        p.type = PktType::ATTACK;
        p.player = player;

        p.attack = {tipo, x, y, raio(tipo)};

        return p;
    }

    // Todos os ovos dentro do raio são atingidos
    Packet resolveAttack(const Packet& in) {
        Packet res{};
        res.type = PktType::RESULT;
        res.player = player;

        bool any_hit = false;

        for (auto& o : ovos) {
            if (o.destruido) continue;

            if (distPx(in.attack.x, in.attack.y, o.x, o.y) <= in.attack.raio) {
                o.destruido = true;
                any_hit = true;
            }
        }

        int vivos = 0;

        for (const auto& o : ovos) {
            if (!o.destruido) vivos++;
        }

        Result r =
            !any_hit ? Result::ERRO :
            vivos == 0 ? Result::VITORIA :
                         Result::ACERTO;

        res.result = {r, (uint8_t)vivos};

        return res;
    }

    void tick(float dt) {
        for (auto& e : explosions) {
            e.vida -= dt;
        }

        explosions.erase(
            std::remove_if(
                explosions.begin(),
                explosions.end(),
                [](const auto& e) {
                    return e.vida <= 0.f;
                }
            ),
            explosions.end()
        );
    }
};
