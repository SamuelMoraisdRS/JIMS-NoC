#ifndef CORE_H
#define CORE_H

#include <systemc.h>
#include "flit.h"
#include <queue>

// Informacoes contidas no pacote   
struct PacketRequest {
    int dest; // Nucleo destino
    int size; // Tamanho da mensagem (metadado para o nucleo alvo, nao usado no roteamento)
    int msg_id; // ID unico da mensagem
};

SC_MODULE(Core) {
    sc_in<bool> clk;
    sc_in<bool> rst;

    // Interface de Envio (Para o Roteador)
    sc_out<Flit> out_data;
    sc_out<bool> out_valid; // NOTE : Necessario pela natureza combinacional do cirtcutyio
    // NOTE: Nao sei se faz sentido receber pulso de credito do roteador. Mantendo p/ compatibildade de interface
    sc_in<bool> credit_in;

    // Interface de Recebimento (Vem do Roteador)
    sc_in<Flit> in_data;
    sc_in<bool> in_valid;

    sc_out<bool> credit_out;

    int core_id;
    bool rx_busy; // Flag para simular contenção/estouro de buffer no nó receptor
    int pending_credits; // Créditos retidos durante o período ocupado

    // Metodos publicos para injetar pacotes
    void send_packet(int dest, int size, int msg_id = 0);

private:
    enum SendState {
        STATE_SEND_IDLE,
        STATE_SEND_HEADER,
        STATE_SEND_BODY,
        STATE_SEND_TAIL
    };

    // Sinais internos
    sc_signal<SendState> current_state;
    sc_signal<int> send_credits;

    // Fila representando os pacotes pra envio do nucleo. Semelhanto ao buffer, mas simplificado (queue simples)
    std::queue<PacketRequest> tx_queue;
    // Pacote na frente da fila
    PacketRequest current_tx;
    // Contado de bodies enviados
    int body_count;

    void process_tx();
    void process_rx();

public:
    SC_CTOR(Core) : rx_busy(false), pending_credits(0) {
        SC_METHOD(process_rx);
        sensitive << clk.pos() << rst;

        SC_METHOD(process_tx);
        sensitive << clk.pos() << rst;
    }
};

#endif // CORE_H
