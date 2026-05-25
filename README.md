# 🥚 Pici Hunt

> **Trabalho de Sockets — Redes de Computadores I**  
> Universidade Federal do Ceará · Departamento de Engenharia de Teleinformática  
> Curso de Engenharia da Computação

---

## 📖 Sobre o Jogo

**Pici Hunt** é um jogo de dois jogadores em rede local inspirado nos clássicos *Batalha Naval* e *Stardew Valley*, ambientado no **Campus do Pici da UFC**.

Cada jogador esconde **3 ovos** em pontos reais do campus (DETI, CT, RU, Lagoa...) e deve procurar os ovos do adversário antes que os seus sejam encontrados. Desenvolvido em C++, a comunicação entre os dois computadores é feita em tempo real via **TCP Sockets** usando a **API POSIX**.

---

## 🗺️ Como Funciona

```
┌──────────────┐         TCP (porta 55000)          ┌──────────────┐
│  Jogador 1   │ ◄────────────────────────────────► │  Jogador 2   │
│    (HOST)    │         Packets binários           │   (CLIENT)   │
│   Gabriela   │                                    │   Larissa    │
└──────────────┘                                    └──────────────┘
```

### Fases da Partida

| Fase | Descrição |
|------|-----------|
| **CONNECT** | Jogador 1 pressiona `H` (HOST) e aguarda. Jogador 2 pressiona `C` (CLIENT), digita o IP e pressiona Enter. |
| **PLACE** | Cada jogador clica em 3 pontos do mapa para esconder seus ovos. Os pontos ficam visíveis apenas para quem os colocou. |
| **WAITING** | Tela de espera animada enquanto o outro jogador termina de posicionar os ovos. |
| **BATTLE** | Turnos alternados - clique no mapa para buscar em um local. Escolha o tipo de busca com as teclas `1`, `2`, `3`. |
| **GAMEOVER** | Quem encontrar todos os ovos do adversário primeiro vence. |

---

## 🔍 Tipos de Busca

| Tecla | Bomba | Raio | Quantidade |
|-------|-------|------|------------|
| `1` | **Precisa** | 20 px | 5 disparos |
| `2` | **Padrão** | 45 px | 3 disparos |
| `3` | **Expandida** | 80 px | 1 disparo |

---

## 🚀 Como Compilar e Executar

### Dependências

- `g++` com suporte a C++17
- [SFML 2.x](https://www.sfml-dev.org/) (Graphics, Window, System, Audio)

**Ubuntu/Debian:**
```bash
sudo apt install g++ libsfml-dev
```

**Arch Linux:**
```bash
sudo pacman -S sfml
```

**macOS (Homebrew):**
```bash
brew install sfml
```

---

### Compilar

```bash
# Build de release (recomendado para jogar em rede)
make

# Build de desenvolvimento (permite loopback 127.0.0.1 para testes locais)
make dev

# Ver todas as opções
make help
```

---

### Executar — Dois computadores na mesma rede

**Computador 1 (HOST — Gabriela):**
```bash
./pici_hunt
# Na tela inicial, pressione H ou clique em Gabriela
# Aguarde a conexão do Player 2
```

**Computador 2 (CLIENT — Larissa):**
```bash
./pici_hunt
# Na tela inicial, pressione C ou clique em Larissa
# Digite o IP do computador 1 (ex: 192.168.1.10)
# Pressione Enter
```

> **Como descobrir o IP do HOST:**  
> ```bash
> ip addr show   # Linux
> ipconfig       # Windows
> ifconfig       # macOS
> ```
> Use o IP da interface de rede local (geralmente `192.168.x.x` ou `10.x.x.x`).

---

### Executar — Teste local (modo DEV)

```bash
# Compila com DEV_MODE (libera loopback)
make dev

# Terminal 1
./pici_hunt_dev   # → Pressione H

# Terminal 2
./pici_hunt_dev   # → Pressione C → IP: 127.0.0.1 → Enter
```

---

## 📁 Estrutura do Projeto

```
computer-networking/
├── main.cpp          # Ponto de entrada, loop principal, renderização e eventos
├── game.hpp          # Estado do jogo, lógica de turnos, pontos do campus
├── net.hpp           # Camada de rede: sockets TCP POSIX (não-bloqueante)
├── Makefile          # Compilação (release e dev)
└── assets/
    ├── font.ttf      # Fonte principal
    ├── music.mp3     # Trilha sonora
    ├── click.wav     # Som de clique
    ├── winner.wav    # Som de vitória
    ├── gameover.wav  # Som de derrota
    ├── mapa_pici.png # Mapa do Campus do Pici (UFC)
    ├── logo.png      # Logo do jogo
    ├── gabriela.png  # Avatar do Player 1
    ├── larissa.png   # Avatar do Player 2
    ├── egg.png       # Sprite do ovo
    └── *.png         # Fotos dos pontos do campus
```

---

## 🌐 Protocolo de Rede

A comunicação usa **TCP** na porta `55000` com pacotes binários de tamanho fixo (`struct Packet`).

```
┌────────┬────────┬──────────────────────────────────┐
│  type  │ player │            payload               │
│ 1 byte │ 1 byte │         (union, variável)        │
└────────┴────────┴──────────────────────────────────┘
```

### Tipos de Pacote

| Tipo | Direção | Conteúdo |
|------|---------|----------|
| `READY` | ambos | Informa que o jogador terminou de posicionar os ovos |
| `ATTACK` | atacante → defensor | Coordenadas `(x, y)` e raio da bomba |
| `RESULT` | defensor → atacante | `ERRO` / `ACERTO` / `VITORIA` + ovos restantes |
| `GAMEOVER` | servidor | ID do vencedor |

### Handshake de Início de Batalha

```
Player 1  ──── READY ────►  Player 2
Player 1  ◄─── READY ────   Player 2
              (ambos receberam READY → transição para BATTLE)
```

---

## 📍 Pontos do Campus

| ID | Nome | Sigla |
|----|------|-------|
| 1 | Açude | ACUDE |
| 2 | Biblioteca CT | BCCP |
| 3 | Educação Física | EF |
| 4 | RU Velho | RUV |
| 5 | ICA | ICA |
| 6 | RU Novo | RUN |
| 7 | Sistemas e Mídias Digitais | SMD |
| 8 | DETI | DETI |
| 9 | Bloco 707 | B707 |
| 10 | Centro de Tecnologia | CT |
| 11 | Cantina da Química | CANTQ |
| 12 | Centro de Ciências | CC |
| 13 | Ciências Agrárias | CCA |
| 14 | Entrada Humberto Monte | EHM |
| 15 | Entrada Mister Hull | EMH |
| 16 | Entrada Pernambuco | EP |
| 17 | Cantina da Física | CANTF |

---

## 🛠️ Tecnologias

- **C++17**
- **SFML 2.x** — janela, gráficos, áudio
- **POSIX Sockets** — TCP não-bloqueante (`O_NONBLOCK`, `MSG_NOSIGNAL`, `MSG_DONTWAIT`)
- **Make** — sistema de build

---

## 👥 Autores

Projeto desenvolvido para a disciplina **Redes de Computadores I** — UFC/DETI por:

Gabriela Bezerra Pereira

Larissa Vitória Santos Menezes
