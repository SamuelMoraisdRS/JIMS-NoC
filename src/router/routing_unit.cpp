#include "routing_unit.h"

void RoutingUnit::routing_process() {
    // loop para avaliar cada uma das 8 portas
    for (int i = 0; i < 8; i++) {
        if (!req_route[i].read()) {
            req_valid[i].write(false);
            req_port[i].write(0);
            continue;
        } 
        sc_uint<4> dest = dest_addr[i].read(); // Ve o destino mandado pela porta
        // sinal de controle
        req_valid[i].write(true);

        // Descida determinista ou trafego local
        if (dest >= min_addr && dest <= max_addr) {
            int target_d = -1;
            if (level == 0) { // Se for um folha
                // NOTE : Seguindo o desenho do roteador no material do prof
                // mapeia a porta D correspondente ao destino (D3=0, D2=1, D1=2, D0=3)
                target_d = 3 - (dest.to_int() - min_addr);
            } else { // roteador raiz

                // divisao inteira pra saber qual o roteador folha escolhido (D3=Folha0, D2=Folha1, D1=Folha2, D0=Folha3)
                target_d = 3 - (dest.to_int() / 4);
            }

            // TODO: Ver o comportamento dos buffers centrais
            // checando se a porta tem creditos livres
            if (output_credits[target_d].read() > 0) {
                req_port[i].write(target_d); // Salva o destino para a porta
            } else {
                // SE O CRÉDITO ACABOU: Depende de onde o flit veio!
                if (i < 4) {
                    // Regra 1: Veio de uma porta D -> vai para o QDN (8)
                    req_port[i].write(8); 
                } else {
                    // Regra 3: Veio de uma porta U -> vai para o QUP (9)
                    req_port[i].write(9); 
                }
            }
        } else { // subida adaptativa
            // seleciona a porta U (4 a 7) com mais creditos
            int best_u = -1;
            int max_credits = -1;

            for (int p = 4; p < 8; p++) {
                int credits = output_credits[p].read().to_int();
                if (credits > max_credits) {
                    max_credits = credits;
                    best_u = p;
                }
            }
            // TODO : Ver o comportamento dos buffers centrais
            // se teve alguma porta com creditos livres, manda por ela
            if (max_credits > 0 && best_u != -1) {
                req_port[i].write(best_u);   // manda pra melhor porta U
            } else {
                // TODAS AS PORTAS U CHEIAS: Não vai para o QUP de forma alguma!
                // Força o STALL (congelamento) no canal de entrada atual.
                req_port[i].write(0);        
                req_valid[i].write(false);   // Avisa o Árbitro que esta requisição NÃO é válida neste ciclo
            }
        }
    }
}
