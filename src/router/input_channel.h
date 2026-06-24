#ifndef INPUT_CHANNEL_H
#define INPUT_CHANNEL_H

#include <systemc.h>
#include "flit.h"
#include "fifo.h"

SC_MODULE(InputChannel) {
    // Portas globais
    sc_in<bool> clk;
    sc_in<bool> rst;

    // Interface com o emissor
    sc_in<Flit> in_data; // Flit de entrada
    sc_in<bool> in_valid; // Sinal que indica se o emissor esta realmente mandando flit
    sc_out<bool> credit_out; // sinal para o emissor incrementar contagem de credito

    // Interface com arbitragem
    sc_in<bool> read_en;             // Sinal recebido do arbitro para ler um flit do buffer
    sc_in<bool> route_grant;         // sinal recebido do arbirto concedendo rota
    sc_out<bool> release_route;      // sinaliza ao arbirtro para liberar a rota (fim do pacote)

    // Interface com unidade de roteamento
    sc_out<bool> req_route;          // solicita roteamento baseado no header atual (esta na frente fila)
    sc_out<sc_uint<4>> dest_addr;    // endereco de destino extraido do header

    // Interface com a Crossbar / Saida
    sc_out<Flit> data_out;           // Flit saindo para a crossbar
    sc_out<bool> empty;              // Buffer vazio

    // Estados da FSM de Roteamento do Canal
    enum State {
        STATE_IDLE,         // esperando header na cabeca do buffer
        STATE_REQ_ROUTE,    // solicitando uma rota
        STATE_ROUTED        // pacote sendo transmitido 
    };

    sc_signal<State> current_state;

private:
    // buffer de entrada
    FifoBuffer<4>* fifo;
    
    // sinais internos da FIFO
    sc_signal<bool> fifo_write_en;
    sc_signal<bool> fifo_read_en;
    sc_signal<bool> fifo_full;
    sc_signal<bool> fifo_empty;
    sc_signal<Flit> fifo_data_out;
    sc_signal<sc_uint<8>> fifo_count;

    // estado interno para controle de credito
    sc_signal<bool> credit_pending;

    void process_logic();
    void fsm_process();

public:
    SC_CTOR(InputChannel) {
        fifo = new FifoBuffer<4>("fifo_inst");
        fifo->clk(clk);
        fifo->rst(rst);
        fifo->write_en(fifo_write_en);
        fifo->data_in(in_data);
        fifo->read_en(fifo_read_en);
        fifo->data_out(fifo_data_out);
        fifo->full(fifo_full);
        fifo->empty(fifo_empty);
        fifo->count(fifo_count);

        SC_METHOD(process_logic);
        sensitive << in_valid << read_en << fifo_empty << fifo_data_out << current_state;

        SC_METHOD(fsm_process);
        sensitive << clk.pos() << rst;
    }

    ~InputChannel() {
        delete fifo;
    }
};

#endif // INPUT_CHANNEL_H
