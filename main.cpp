#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include "net.hpp"
#include "game.hpp"

// ============================================================
//  main.cpp — Pici Hunt
//  Fases: CONNECT → PLACE → WAITING → BATTLE → GAMEOVER
// ============================================================

static sf::Font font;

sf::Text txt(const std::string& s, unsigned sz,
             float x, float y, sf::Color c = sf::Color::White) {
    sf::Text t; t.setFont(font); t.setString(s);
    t.setCharacterSize(sz); t.setFillColor(c); t.setPosition(x, y); return t;
}

sf::Texture makePlaceholder(unsigned w, unsigned h, sf::Color cor, const std::string& l = "") {
    sf::RenderTexture rt; rt.create(w, h); rt.clear(cor);
    if (!l.empty()) { auto t=txt(l,h/3,w/2.f-h/6.f,h/3.f); rt.draw(t); }
    rt.display();
    sf::Texture tex; tex.loadFromImage(rt.getTexture().copyToImage()); return tex;
}

// ── Fade entre telas ─────────────────────────────────────────

struct Transition {
    float alpha = 255.f;
    float speed = 255.f / 0.30f;
    bool  active = false;
    Fase  next = Fase::CONNECT;

    void trigger(Fase f) { next=f; active=true; }

    void tick(float dt, Fase& fase) {
        if (!active && alpha<=0.f) return;
        if (active) {
            alpha += speed*dt;
            if (alpha >= 255.f) { alpha=255.f; fase=next; active=false; }
        } else {
            alpha -= speed*dt;
            if (alpha < 0.f) alpha=0.f;
        }
    }

    void draw(sf::RenderWindow& w) {
        if (alpha<=0.f) return;
        sf::RectangleShape o({800,690});
        o.setFillColor(sf::Color(0,0,0,(uint8_t)alpha));
        w.draw(o);
    }
};

// ── Lobby — dimensões ─────────────────────────────────────────
//
//  Janela: 800 × 690
//  Logo (400×187 → exibida em 320×150): centrada no topo
//  Cards (190×255 cada): lado a lado, centrados
//  Campo IP: abaixo dos cards quando cliente selecionado
//  Status: rodapé
//
constexpr float LOGO_W=320, LOGO_H=150;
constexpr float CARD_W=190, CARD_H=255;
constexpr float CARD_LEFT1=170, CARD_LEFT2=460, CARD_TOP=265;

const sf::FloatRect CARD_GAB = {CARD_LEFT1, CARD_TOP, CARD_W, CARD_H};
const sf::FloatRect CARD_LAR = {CARD_LEFT2, CARD_TOP, CARD_W, CARD_H};

void drawConnect(sf::RenderWindow& w,
                 sf::Sprite& bg, sf::Sprite& logo,
                 sf::Sprite& spGab, sf::Sprite& spLar,
                 int sel, const std::string& ip, const std::string& status)
{
    w.draw(bg);

    sf::RectangleShape overlay({800,690});
    overlay.setFillColor(sf::Color(0,0,0,80));
    w.draw(overlay);

    // Logo centralizada
    logo.setScale(LOGO_W/logo.getLocalBounds().width,
                  LOGO_H/logo.getLocalBounds().height);
    logo.setPosition((800-LOGO_W)/2.f, 50);
    w.draw(logo);

    w.draw(txt("Select your player", 16, 299, 220, sf::Color(200,200,200)));

    // Cards
    auto drawCard = [&](const sf::FloatRect& r, sf::Sprite& sp,
                        const std::string& nome, const std::string& papel,
                        bool sel_, sf::Color cor)
    {
        // Sombra quando selecionado
        if (sel_) {
            sf::RectangleShape glow({r.width+16, r.height+16});
            glow.setPosition(r.left-8, r.top-8);
            glow.setFillColor(sf::Color(cor.r, cor.g, cor.b, 40));
            glow.setOutlineColor(cor); glow.setOutlineThickness(3.f);
            w.draw(glow);
        }

        // Sprite
        sp.setPosition(r.left, r.top);
        sp.setScale(r.width/sp.getLocalBounds().width,
                    r.height/sp.getLocalBounds().height);
        w.draw(sp);

        // Faixa de nome
        sf::RectangleShape faixa({r.width, 52});
        faixa.setPosition(r.left, r.top+r.height-52);
        faixa.setFillColor(sf::Color(8,8,18,210));
        w.draw(faixa);

        w.draw(txt(nome,  15, r.left+8, r.top+r.height-50));
        w.draw(txt(papel, 12, r.left+8, r.top+r.height-30, cor));

        if (sel_)
            w.draw(txt("Selected", 11, r.left+8, r.top+r.height+15, cor));
    };

    drawCard(CARD_GAB, spGab, "Gabriela", "HOST  [H]",    sel==1, sf::Color(80,220,120));
    drawCard(CARD_LAR, spLar, "Larissa",  "CLIENT  [C]", sel==2, sf::Color(255,200,60));

    // Campo de IP — aparece apenas para o cliente
    if (sel == 2) {
        float by = CARD_TOP + CARD_H + 55;
        w.draw(txt("Server IP:", 13, CARD_LEFT1, by-18, sf::Color(180,180,180)));
        sf::RectangleShape box({460, 36});
        box.setPosition(CARD_LEFT1, by);
        box.setFillColor(sf::Color(12,12,28,230));
        box.setOutlineColor(sf::Color(255,200,60,200));
        box.setOutlineThickness(1.5f);
        w.draw(box);
        w.draw(txt(ip.empty()?"_":ip+"_", 17, CARD_LEFT1+10, by+7));
        w.draw(txt("Press ENTER to connect", 12,
                   CARD_LEFT1, by+42, sf::Color(130,130,130)));
    }

    if (!status.empty())
        w.draw(txt(status, 14, CARD_LEFT1, 658, sf::Color(170,170,170)));
}

// ── Tela PLACE ───────────────────────────────────────────────

void drawPlace(sf::RenderWindow& w, sf::Sprite& mapa,
               sf::Sprite& eggSpr, const Game& g)
{
    w.draw(mapa);
    for (auto& p : pontos()) {
        bool occ = false;
        for (auto& o : g.ovos) if (o.ponto_id==p.id) { occ=true; break; }

        if (occ) {
            // Ovo posicionado
            eggSpr.setColor(sf::Color::White);
            eggSpr.setScale(24.f/eggSpr.getLocalBounds().width,
                            28.f/eggSpr.getLocalBounds().height);
            eggSpr.setOrigin(eggSpr.getLocalBounds().width/2,
                             eggSpr.getLocalBounds().height/2);
            eggSpr.setPosition(p.x, p.y);
            w.draw(eggSpr);
        } else {
            // Ponto disponível — círculo sutil
            sf::CircleShape c(8.f); c.setOrigin(8,8); c.setPosition(p.x, p.y);
            c.setFillColor(sf::Color(0,200,100,140));
            c.setOutlineColor(sf::Color::White); c.setOutlineThickness(1);
            w.draw(c);
        }
        // Label do ponto
        sf::Text lbl = txt(p.sigla, 11, p.x+14, p.y-6, sf::Color(240,240,240));
        w.draw(lbl);
    }

    sf::RectangleShape bar({MAP_W,48}); bar.setPosition(0,MAP_H);
    bar.setFillColor(sf::Color(15,15,35,240)); w.draw(bar);
    w.draw(txt("Esconda " + std::to_string(MAX_OVOS) + " ovos no campus.  (" +
               std::to_string(g.ovos.size()) + "/" +
               std::to_string(MAX_OVOS) + "  colocados)", 25, 100, MAP_H+25));
}

// ── Tela BATTLE ──────────────────────────────────────────────

void drawBattle(sf::RenderWindow& w, sf::Sprite& mapa,
                sf::Sprite& eggSpr, const Game& g)
{
    w.draw(mapa);

    // Meus ovos
    for (auto& o : g.ovos) {
        const Ponto* p = getPonto(o.ponto_id); if (!p) continue;
        eggSpr.setOrigin(eggSpr.getLocalBounds().width/2,
                         eggSpr.getLocalBounds().height/2);
        eggSpr.setScale(28.f/eggSpr.getLocalBounds().width,
                        32.f/eggSpr.getLocalBounds().height);
        eggSpr.setPosition(p->x, p->y);
        eggSpr.setColor(o.destruido ? sf::Color(90,90,90,180) : sf::Color::White);
        w.draw(eggSpr);
    }

    // Explosões
    for (auto& e : g.explosions) {
        float px=(e.x), py=(e.y), r=(float)(e.raio*.5f);
        uint8_t a=(uint8_t)(std::min(1.f,e.vida/3.f)*150);
        sf::CircleShape c(r); c.setOrigin(r,r); c.setPosition(px,py);
        c.setFillColor(e.hit ? sf::Color(255,120,0,a) : sf::Color(180,180,180,a));
        w.draw(c);
    }

    // HUD
    sf::RectangleShape bar({MAP_W,90}); bar.setPosition(0,MAP_H);
    bar.setFillColor(sf::Color(15,15,35,240)); w.draw(bar);

    const char* armas[]={"Precisa (10m)","Comum (50m)","Expandida (150m)"};
    bool mt = g.meu_turno;
    std::string linha1 = mt
        ? ">>> SEU TURNO <<<   [1] x"+std::to_string(g.ammo[0])+
          "  [2] x"+std::to_string(g.ammo[1])+
          "  [3] x"+std::to_string(g.ammo[2])+
          "   Busca: "+armas[g.arma]
        : "Aguardando oponente...";
    int vivos=0; for (auto& o:g.ovos) if(!o.destruido) vivos++;
    w.draw(txt(linha1, 20, mt ? 100:275, MAP_H+15,  mt ? sf::Color(80,240,120) : sf::Color(180,180,180)));
    w.draw(txt("Seus ovos: "+std::to_string(vivos)+
               "   |   Ovos do oponente: "+std::to_string(g.op_ovos),
               17, 230, MAP_H+45));
}

// ── Tela GAMEOVER ────────────────────────────────────────────

void drawGameOver(sf::RenderWindow& w, const Game& g) {
    w.clear(sf::Color(12,12,22));
    bool won = (g.winner == g.player);

    // Faixa de resultado
    sf::RectangleShape faixa({800, 110});
    faixa.setPosition(0, 230);
    faixa.setFillColor(won ? sf::Color(20,80,30,220) : sf::Color(80,15,15,220));
    w.draw(faixa);

    w.draw(txt(won ? "WINNER!" : "GAME OVER", 52, won?295:260, 252,
               won ? sf::Color(80,255,120) : sf::Color(255,80,80)));

    w.draw(txt(won ? "Todos os ovos do oponente foram encontrados!"
                   : "Seus ovos foram todos encontrados.", 18,
               won ? 180 : 230, 355, sf::Color(210,210,210)));

    w.draw(txt("Pressione ESC para sair", 16, 288, 430, sf::Color(130,130,130)));
}

// ── Main ─────────────────────────────────────────────────────

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 690), "Pici Hunt");
    window.setFramerateLimit(60);

    if (!font.loadFromFile("assets/font.ttf"))
        font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");

    // ── Texturas ─────────────────────────────────────────────
    sf::Texture mapTex, logoTex, gabTex, larTex, eggTex, transpText;

    if (!mapTex.loadFromFile("assets/mapa_pici.png")) {
        sf::RenderTexture rt; rt.create(800,600); rt.clear(sf::Color(40,80,40));
        sf::RectangleShape ln; ln.setFillColor(sf::Color(55,95,55));
        for(int x=0;x<=800;x+=80){ln.setSize({1,600});ln.setPosition(x,0);rt.draw(ln);}
        for(int y=0;y<=600;y+=60){ln.setSize({800,1});ln.setPosition(0,y);rt.draw(ln);}
        rt.display(); mapTex=rt.getTexture();
    }
    if (!logoTex.loadFromFile("assets/logo.png"))
        logoTex = makePlaceholder(320,150,sf::Color(20,50,80),"PICI HUNT");
    if (!gabTex.loadFromFile("assets/gabriela.png"))
        gabTex  = makePlaceholder(190,255,sf::Color(40,80,60),"G");
    if (!larTex.loadFromFile("assets/larissa.png"))
        larTex  = makePlaceholder(190,255,sf::Color(70,50,20),"L");
    if (!eggTex.loadFromFile("assets/egg.png"))
        eggTex  = makePlaceholder(28,32,sf::Color(255,230,100),"O");
    if (!transpText.loadFromFile("assets/transparent.png"))
        transpText  = makePlaceholder(28,32,sf::Color(255,230,100),"O");

    sf::Sprite mapa(mapTex), logo(logoTex), spGab(gabTex), spLar(larTex), eggSpr(eggTex), transSpr(transpText);

    // Backgrounds do lobby — slideshow entre as imagens disponíveis
    int num_img = 7;
    sf::Texture bgTexs[num_img];
    sf::Sprite  bgSprites[num_img];
    const char* bgFiles[num_img] = {
        "assets/lagoa.png",
        "assets/fisica.png",
        "assets/seara.png",
        "assets/deti.png",
        "assets/portaria.png",
        "assets/ct.png",
        "assets/ru_velho.png"
    };
    for (int i=0;i<num_img;i++) {
        if (!bgTexs[i].loadFromFile(bgFiles[i]))
            bgTexs[i] = makePlaceholder(800,690,sf::Color(15+i*10,20,40+i*10));
        bgSprites[i].setTexture(bgTexs[i]);
        // Garante que todos cubram 800×690
        bgSprites[i].setScale(800.f/bgTexs[i].getSize().x,
                              690.f/bgTexs[i].getSize().y);
    }
    int   bg_idx   = 0;
    float bg_timer = 0.f;

    // Fading between slides
    float bg_fade_alpha = 0.f;   // 0 = só current, 255 = só next
    bool  bg_fading     = false;
    int   bg_next_idx   = 1;

    constexpr float BG_INTERVAL = 3.f;   // segundos por imagem

    // ── Áudio ────────────────────────────────────────────────
    sf::SoundBuffer clickBuf, winBuf, loseBuf;
    sf::Sound       clickSnd, winSnd, loseSnd;

    if (clickBuf.loadFromFile("assets/click.wav"))    clickSnd.setBuffer(clickBuf);
    if (winBuf.loadFromFile("assets/winner.wav"))     winSnd.setBuffer(winBuf);
    if (loseBuf.loadFromFile("assets/gameover.wav"))  loseSnd.setBuffer(loseBuf);

    sf::Music music;
    if (music.openFromFile("assets/music.mp3")) {
        music.setLoop(true); music.setVolume(38.f); music.play();
    }

    bool gameover_snd_played = false;

    auto playClick = [&]() {
        if (clickBuf.getDuration()>sf::Time::Zero) clickSnd.play();
    };

    // ── Estado ───────────────────────────────────────────────
    Game g;
    Net  net;
    Transition fade;
    std::string ip_input, status;
    bool hosting  = false;
    int  char_sel = 0;
    bool connect_request = false;
    float waiting_timer = 0.f;

    // ready_sent: este jogador já colocou os 3 ovos e avisou o outro lado.
    // opponent_ready: o READY do outro jogador já chegou.
    bool ready_sent = false;
    bool opponent_ready = false;

    sf::Clock clock;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        // ── Slideshow do lobby ────────────────────────────────
        if (g.fase == Fase::CONNECT) {
            bg_timer += dt;
            constexpr float FADE_DUR = 0.5f;   // duração do crossfade em segundos

            if (!bg_fading && bg_timer >= BG_INTERVAL - FADE_DUR) {
                bg_fading   = true;
                bg_next_idx = (bg_idx + 1) % num_img;
                bg_fade_alpha = 0.f;
            }
            if (bg_fading) {
                bg_fade_alpha += (255.f / FADE_DUR) * dt;
                if (bg_fade_alpha >= 255.f) {
                    bg_fade_alpha = 0.f;
                    bg_fading     = false;
                    bg_idx        = bg_next_idx;
                    bg_timer      = 0.f;
                }
            }
        }

        // ── Carregando ────────────────────────────────
        if (g.fase == Fase::WAITING) waiting_timer += dt;

        // ── Som de game over (toca uma vez) ───────────────────
        if (g.fase==Fase::GAMEOVER && !gameover_snd_played) {
            gameover_snd_played = true;
            if (g.winner==g.player) {
                if (winBuf.getDuration()>sf::Time::Zero) winSnd.play();
            } else {
                if (loseBuf.getDuration()>sf::Time::Zero) loseSnd.play();
            }
        }

        // ── Eventos ──────────────────────────────────────────
        sf::Event ev;
        while (window.pollEvent(ev)) {

            if (ev.type==sf::Event::Closed) window.close();
            if (fade.active) continue;
            
            #ifdef DEV_MODE
                // Debug listener to locate points directly on the map image
                if (ev.type == sf::Event::MouseButtonPressed &&
                    ev.mouseButton.button == sf::Mouse::Left) {

                    sf::Vector2i mousePixel(ev.mouseButton.x, ev.mouseButton.y);

                    // Converts mouse position from window pixels to world coordinates
                    sf::Vector2f mouseWorld = window.mapPixelToCoords(mousePixel);

                    // Converts world coordinates to local map image coordinates
                    sf::Vector2f mapPos =
                        mapa.getInverseTransform().transformPoint(mouseWorld);

                    std::cout << "[MAP PICKER] x = " << mapPos.x
                            << ", y = " << mapPos.y << "\n";

                    std::cout << "{ID, \"Nome\", \"SIGLA\", "
                            << mapPos.x << ".f, " << mapPos.y << ".f},\n";
                }
            #endif

            if (ev.type==sf::Event::KeyPressed) {
                auto k=ev.key.code;

                if (g.fase==Fase::CONNECT) {
                    
                    if (k==sf::Keyboard::H && char_sel!=1) {
                        playClick(); char_sel=1; hosting=true; g.player=1;
                        net.startServer();
                        status="Waiting for Player 2 on PORT "+std::to_string(PORT)+"...";
                    }
                    if (k==sf::Keyboard::C && char_sel!=2) {
                        playClick(); char_sel=2; hosting=false; g.player=2;
                        status="Digit the IP and press ENTER";
                    }
                    if (k==sf::Keyboard::Enter && char_sel==2 && !ip_input.empty()) {
                        connect_request = true;
                        status="Connecting...";
                    }
                    if (k==sf::Keyboard::BackSpace && !ip_input.empty())
                        ip_input.pop_back();
                }
                if (g.fase==Fase::BATTLE) {
                    if (k==sf::Keyboard::Num1) g.arma=0;
                    if (k==sf::Keyboard::Num2) g.arma=1;
                    if (k==sf::Keyboard::Num3) g.arma=2;
                }
                if (g.fase==Fase::GAMEOVER && k==sf::Keyboard::Escape)
                    window.close();
            }

            if (ev.type==sf::Event::TextEntered && g.fase==Fase::CONNECT && char_sel==2) {
                char c=(char)ev.text.unicode;
                if ((isdigit(c)||c=='.') && ip_input.size()<15) ip_input+=c;
            }

            if (ev.type==sf::Event::MouseButtonPressed &&
                ev.mouseButton.button==sf::Mouse::Left)
            {
                float mx=ev.mouseButton.x, my=ev.mouseButton.y;

                if (g.fase==Fase::CONNECT) {
                    if (CARD_GAB.contains({mx,my}) && char_sel!=1) {
                        playClick(); char_sel=1; hosting=true; g.player=1;
                        ip_input.clear(); net.startServer();
                        status="Waiting for Player 2 on PORT "+std::to_string(PORT)+"...";
                    }
                    if (CARD_LAR.contains({mx,my}) && char_sel!=2) {
                        playClick(); char_sel=2; hosting=false; g.player=2;
                        status="Digit the IP and press ENTER";
                    }
                }

                if (g.fase==Fase::PLACE && my<MAP_H) {
                    for (auto& p:pontos()) {
                        float dx=p.x-mx, dy=p.y-my;
                        if (dx*dx+dy*dy<16*16) {
                            if (g.placeOvo(p.id)) playClick();
                            break;
                        }
                    }
                    if (!fade.active && fade.alpha <= 0.f) {
                        if ((g.fase==Fase::PLACE || g.fase==Fase::WAITING) && ready_sent) {
                            if (opponent_ready) {
                                fade.trigger(Fase::BATTLE);
                            } else if (g.fase==Fase::PLACE && (int)g.ovos.size()==MAX_OVOS) {
                                fade.trigger(Fase::WAITING);
                            }
                        }
                    }
                    if ((int)g.ovos.size()==MAX_OVOS && !ready_sent) {
                        Packet rdy{}; 
                        rdy.type=PktType::READY; 
                        rdy.player=g.player;
                        rdy.ready={MAX_OVOS};
                        
                        net.send(rdy);

                        std::cout << "[SEND] sizeof(Packet)=" << sizeof(Packet) 
                        << " bytes enviados para peer=" << net.peer << "\n";
    
                        ready_sent = true;
                        std::cout << "[SEND] READY enviado. opponent_ready=" << opponent_ready << "\n";
                    }
                }

                if (g.fase==Fase::BATTLE && g.meu_turno && my<MAP_H) {
                    if (g.ammo[g.arma]>0) {
                        playClick();
                        g.ammo[g.arma]--;
                        auto pkt=g.buildAttack(mx, my);
                        net.send(pkt); g.meu_turno=false;
                    }
                }
            }
        }

        // ── Rede ─────────────────────────────────────────────
        if (g.fase==Fase::CONNECT) {
            if (hosting) net.accept();
            else if (char_sel==2 && !ip_input.empty() && connect_request) net.connect(ip_input);
            if (net.connected) {
                std::cout << "[CONNECTED] peer=" << net.peer << " player=" << g.player << "\n";
                g.meu_turno=(g.player==1);
                fade.trigger(Fase::PLACE);
                status.clear();
            }
            if (!net.error.empty()) {
                std::cout << "[ERROR] " << net.error << "\n";
                status = "Erro: " + net.error + " - corrija o IP e pressione ENTER novamente.";
                net.resetPeer();
                connect_request = false;
            }
        }

        if (g.fase==Fase::PLACE || g.fase==Fase::WAITING||g.fase==Fase::BATTLE) {
            while (auto pkt=net.recv()){
                if (pkt->type==PktType::READY) {
                    opponent_ready = true;
                    g.op_ovos = pkt->ready.num_torres;
                    std::cout << "[RECV] READY do oponente. ready_sent=" << ready_sent << "\n";
                    if (ready_sent) fade.trigger(Fase::BATTLE);
                }
                if (pkt->type==PktType::ATTACK && g.fase==Fase::BATTLE) {
                    auto res=g.resolveAttack(*pkt);
                    g.explosions.push_back(Explosion{
                        pkt->attack.x,
                        pkt->attack.y,
                        pkt->attack.raio,
                        res.result.res != Result::ERRO
                    });

                    net.send(res);

                    if (res.result.res==Result::VITORIA) { g.winner=pkt->player; fade.trigger(Fase::GAMEOVER); }
                    else g.meu_turno=true;
                }
                if (pkt->type==PktType::RESULT && g.fase==Fase::BATTLE) {
                    g.op_ovos=pkt->result.ovos_vivos;
                    g.explosions.push_back({0,0,50, pkt->result.res!=Result::ERRO});
                    if (pkt->result.res==Result::VITORIA) { g.winner=g.player; fade.trigger(Fase::GAMEOVER); }
                    else g.meu_turno=false;
                }
                if (pkt->type==PktType::GAMEOVER) { g.winner=pkt->gameover.winner; fade.trigger(Fase::GAMEOVER); }
            }
        }

        g.tick(dt);
        fade.tick(dt, g.fase);

        if (!fade.active && fade.alpha <= 0.f) {
            if ((g.fase==Fase::PLACE || g.fase==Fase::WAITING) && ready_sent) {
                std::cout << "[TRANSITION] opponent_ready=" << opponent_ready
                        << " ovos=" << g.ovos.size() << "/" << MAX_OVOS
                        << " fase=" << (int)g.fase << "\n";
                if (opponent_ready) {
                    fade.trigger(Fase::BATTLE);
                } else if (g.fase==Fase::PLACE && (int)g.ovos.size()==MAX_OVOS) {
                    fade.trigger(Fase::WAITING);
                }
            }
        }

        // ── Render ───────────────────────────────────────────
        window.clear(sf::Color(15,15,28));

        switch (g.fase) {
            case Fase::CONNECT:
                // Draw current bg
                window.draw(bgSprites[bg_idx]);
                if (bg_fading) {
                    bgSprites[bg_next_idx].setColor(sf::Color(255,255,255,(uint8_t)bg_fade_alpha));
                    // redesenha só o fundo do próximo por cima
                    window.draw(bgSprites[bg_next_idx]);
                    bgSprites[bg_next_idx].setColor(sf::Color::White);   // reseta para não afetar outros frames
                }
                drawConnect(window, transSpr, logo,
                            spGab, spLar, char_sel, ip_input, status);
                break;

            case Fase::PLACE:
                drawPlace(window, mapa, eggSpr, g);
                break;

            case Fase::WAITING: {
                window.draw(mapa);

                // Fundo escuro embaixo do mapa
                sf::RectangleShape bar({MAP_W, 90}); bar.setPosition(0, MAP_H);
                bar.setFillColor(sf::Color(15,15,35,240)); window.draw(bar);

                // Overlay transparente
                sf::RectangleShape overlay({800,690});
                overlay.setFillColor(sf::Color(0,0,0,150));
                window.draw(overlay);

                window.draw(txt("Aguardando o oponente esconder os ovos...",
                                25, 115, MAP_H + 25));

                // Spinner: 6 círculos pulsando em sequência
                int   dots      = 6;
                float cx        = 400.f, cy = 300, orbit = 28.f;

                for (int i = 0; i < dots; i++) {
                    float angle  = (i / (float)dots) * 2.f * 3.14159f;
                    float phase  = fmod(waiting_timer * 3.f - i * 0.3f, 2.f * 3.14159f);
                    float bright = 100.f + 155.f * std::max(0.f, std::cos(phase));
                    sf::CircleShape dot(5.f); dot.setOrigin(5,5);
                    dot.setPosition(cx + orbit * std::cos(angle), cy + orbit * std::sin(angle));
                    dot.setFillColor(sf::Color((uint8_t)bright,(uint8_t)bright,255));
                    window.draw(dot);
                }
                break;
            }

            case Fase::BATTLE:
                drawBattle(window, mapa, eggSpr, g);
                break;

            case Fase::GAMEOVER:
                drawGameOver(window, g);
                break;
        }

        fade.draw(window);
        window.display();
    }
}
