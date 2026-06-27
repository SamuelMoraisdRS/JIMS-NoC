#include "input_channel.h"

// Logica Combinacional
void InputChannel::process_logic() {
    // Escreve na FIFO se vier um dado valido e houver espaço
    fifo_write_en.write(in_valid.read() && !fifo_full.read());

    // Le da FIFO se o arbitro/crossbar solicitar E houver dados
    fifo_read_en.write(read_en.read() && !fifo_empty.read());

    // conecta saidas da FIFO
    data_out.write(fifo_data_out.read());
    empty.write(fifo_empty.read());

    // NOTE : Por que esta parte eh combinacional
    // se estiver solicitando rota, disponibiliza o endereço de destino
    if (current_state.read() == STATE_REQ_ROUTE) {
        req_route.write(true);
        // o endereço de destino está nos 4 bits mais significativos do payload do Header
        sc_uint<16> payload = fifo_data_out.read().payload;
        dest_addr.write(payload.range(15, 12));
    } else {
        req_route.write(false);
        dest_addr.write(0);
    }
}

// Logica Sequencial. atualiza o estado e os sinais de controle do componente
void InputChannel::fsm_process() {
    if (rst.read()) {
        current_state.write(STATE_IDLE);
        credit_out.write(false);
        release_route.write(false);
    } else {
        // Log de recebimento de flit
        if (in_valid.read() && !fifo_full.read()) {
            std::cout << "[" << this->name() << " at " << sc_time_stamp()
                      << "] Flit Chegando: " << in_data.read() << std::endl;
        }

        // Se houver uma leitura válida no ciclo atual, geramos um pulso de retorno de crédito
        if (read_en.read() && !fifo_empty.read()) {
            credit_out.write(true);
        } else {
            credit_out.write(false);
        }

        // transicao do estado local
        switch (current_state.read()) {
            case STATE_IDLE: // Se estiver aguardando flit header
                release_route.write(false);
                // se for header
                if (!fifo_empty.read() && fifo_data_out.read().framing == 0) {
                    current_state.write(STATE_REQ_ROUTE);
                }
                break;

            case STATE_REQ_ROUTE:
                // espera o roteador/arbitro alocar uma rota para o pacote
                if (route_grant.read()) {
                    current_state.write(STATE_ROUTED);
                }
                break;

            case STATE_ROUTED:
                // se ler o flit tail, manda sinal de release para o arbitro
                if (read_en.read() && !fifo_empty.read() && fifo_data_out.read().framing == 2) {
                    release_route.write(true);
                    current_state.write(STATE_IDLE);
                }
                break;
        }
    }
}
