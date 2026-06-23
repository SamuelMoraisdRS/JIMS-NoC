#ifndef OUTPUT_CHANNEL_H
#define OUTPUT_CHANNEL_H

#include <systemc.h>
#include "flit.h"

SC_MODULE(OutputChannel) {
    // Portas globais
    sc_in<bool> clk;
    sc_in<bool> rst;

    // Interface interna com a Crossbar / Árbitro do Roteador
    sc_in<Flit> data_in;            // Flit vindo da crossbar
    sc_in<bool> write_en;           // Árbitro indica que este canal foi escolhido para transmitir

    // Comunicação com a Unidade de Roteamento (RoutingUnit)
    // Envia a contagem atualizada de créditos para a tomada de decisão da RoutingUnit
    sc_out<sc_uint<8>> credit_to_routing; 

    // Interface física externa (Link com o próximo roteador ou terminal)
    sc_out<Flit> out_data;          // Dados saindo do roteador
    sc_out<bool> out_valid;         // Sinaliza que o dado externo é válido
    sc_in<bool> credit_in;          // Pulso de crédito recebido do vizinho (indica leitura lá)

    // Sinal interno para o registrador de créditos
    // Inicializado em 4 (tamanho padrão do buffer de entrada do SPIN)
    sc_signal<sc_uint<8>> credit_count;

    void combinational_logic();
    void sequential_logic();

    SC_CTOR(OutputChannel) {
        SC_METHOD(combinational_logic);
        sensitive << data_in << write_en << credit_count;

        SC_METHOD(sequential_logic);
        sensitive << clk.pos() << rst;
    }
};

#endif // OUTPUT_CHANNEL_H