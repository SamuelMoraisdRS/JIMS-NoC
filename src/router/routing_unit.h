#ifndef ROUTING_UNIT_H
#define ROUTING_UNIT_H


#include <systemc.h>

// NOTE : Vamos dar ids numericos sequenciais para os roteadores/nucleos da arvore
// NOTE : Estamos enumeradno internamente as portas de 0 a 7.
// NOTE : 0,1,2,3 = DN. 4,5,6,7 = UP.

SC_MODULE(RoutingUnit) {
    // parametros do roteador (configurados no construtor ou externamente)
    int router_id;
    int level;      // 0 = Roteador Folha, 1 = Roteador Raiz

    // NOTE : Acho que isso so e util nos roteadores folha
    int min_addr;   // Menor endereco de folha na subarvore
    int max_addr;   // Maior endereco de folha na subarvore

    // == Entradas

    // NOTE : Usando o indice da array para referenciar cada porta

    // Flag indicando que a porta pediu rota
    sc_in<bool> req_route[8];

    // endereco de destino recebido no flit header
    sc_in<sc_uint<4>> dest_addr[8];

    // # de creditos livres em cada canal de saida, para parte adaptativa do roteamento 
    sc_in<sc_uint<8>> output_credits[8];

    // == Saidas
    
    // saidas contendo a porta destino calculada pra cada entrada (8 = QDN, 9 = QUP)
    sc_out<sc_uint<4>> req_port[8];

    // NOTE : Este sinal eh necessario por essa parte ser combinacional. Eh analogo ao in_valid em @input_channel.h, mas sera usado pelo arbitro
    // Sinal de controle pra arbitragem informando que a porta esta mandando flit
    sc_out<bool> req_valid[8];

    // Funcao de roteamento (combinacional)
    void routing_process();

public:
    SC_CTOR(RoutingUnit) {
        SC_METHOD(routing_process);
        // 
        for (int i = 0; i < 8; i++) {
            sensitive << req_route[i] << dest_addr[i] << output_credits[i];
        }
    }
};

#endif // ROUTING_UNIT_H
