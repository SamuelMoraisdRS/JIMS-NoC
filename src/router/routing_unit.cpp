#include "routing_unit.h"

void RoutingUnit::routing_process() {
    qup_req_valid.write(false);
    qup_req_port.write(0);
    qdn_req_valid.write(false);
    qdn_req_port.write(0);

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

            // NOTE : Colocando isso por causa dos segfault
            // checando se a porta tem creditos livres com validacao de limites
            if (target_d >= 0 && target_d < 8 && output_credits[target_d].read() > 0) {
                req_port[i].write(target_d); // Salva o destino para a porta
            } else {
                // SE O CRÉDITO ACABOU: Depende de onde o flit veio!
                if (i < 4) {
                    // Veio de uma porta D -> vai para o QDN (8) caso não esteja lotado
                    if (!qdn_full.read()) {
                        req_port[i].write(8); 
                        qdn_target_out.write(dest);
                    } else {
                        // Se o buffer compartilhado também encheu, força STALL
                        req_port[i].write(0);
                        req_valid[i].write(false);
                    }
                } else {
                    // Veio de uma porta U -> vai para o QUP (9) caso não esteja lotado
                    if (!qup_full.read()) {
                        req_port[i].write(9); 
                        qup_target_out.write(dest);
                    } else {
                        // Se o buffer compartilhado também encheu, força STALL
                        req_port[i].write(0);
                        req_valid[i].write(false);
                    }
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

    // --- Monitoramento do QUP ---
    if (!qup_empty.read()) {
        sc_uint<4> qup_dest_addr = qup_head_dest.read();
        int target_port_qup = -1;

        //  TODO : Encapsular essa logica redundante em funcao
        // Mapeia o endereço de destino na porta de descida correspondente
        if (level == 0) {
            target_port_qup = 3 - (qup_dest_addr.to_int() - min_addr);
        } else {
            target_port_qup = 3 - (qup_dest_addr.to_int() / 4);
        }

        // Se a porta física de destino finalmente liberou espaço (crédito > 0) com validacao de limites
        if (target_port_qup >= 0 && target_port_qup < 8 && output_credits[target_port_qup].read() > 0) {
            qup_req_valid.write(true);             // Ativa requisição do QUP para o Árbitro
            qup_req_port.write(target_port_qup);   // Informa qual a porta física de saída desejada
        }
    }

    // --- Monitoramento do QDN ---
    if (!qdn_empty.read()) {
        sc_uint<4> qdn_dest_addr = qdn_head_dest.read();
        int target_port_qdn = -1;

        if (level == 0) {
            target_port_qdn = 3 - (qdn_dest_addr.to_int() - min_addr);
        } else {
            target_port_qdn = 3 - (qdn_dest_addr.to_int() / 4);
        }

        if (target_port_qdn >= 0 && target_port_qdn < 8 && output_credits[target_port_qdn].read() > 0) {
            qdn_req_valid.write(true);             // Ativa requisição do QDN para o Árbitro
            qdn_req_port.write(target_port_qdn);   // Informa qual a porta física de saída desejada
        }
    }
}
