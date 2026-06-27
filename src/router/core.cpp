#include "core.h"

void Core::send_packet(int dest, int size, int msg_id) {
    PacketRequest req;
    req.dest = dest;
    req.size = size;
    req.msg_id = msg_id;
    tx_queue.push(req);
}

void Core::process_rx() {
    if (rst.read()) {
        credit_out.write(false);
    } else {
        if (in_valid.read()) {
            Flit f = in_data.read();
            std::cout << "[Core " << core_id << " at " << sc_time_stamp() 
                      << "] Received Flit: " << f << std::endl;
            credit_out.write(true); // Devolve o crédito para o roteador vizinho
        } else {
            credit_out.write(false);
        }
    }
}

void Core::process_tx() {
    if (rst.read()) {
        send_credits.write(4);
        out_valid.write(false);
        out_data.write(Flit());
        current_state.write(STATE_SEND_IDLE);
        body_count = 0;
        while(!tx_queue.empty()) tx_queue.pop();
    } else {
        // 1. Controle de Créditos de Envio
        int credits = send_credits.read();
        bool flit_sent = out_valid.read() && (credits > 0);
        bool credit_received = credit_in.read();

        // Mandou flit e nao recebeu credito -> perde um creito
        if (flit_sent && !credit_received) {
            credits = credits - 1;
        } else if (!flit_sent && credit_received) { // nao mandou flit e recebeu credito -> ganha um creito
            if (credits < 4) { 
                credits = credits + 1;
            }
        }
        send_credits.write(credits);

        // 2. FSM de Transmissão
        switch (current_state.read()) {
            case STATE_SEND_IDLE:
                out_valid.write(false);
                out_data.write(Flit());
                if (!tx_queue.empty()) {
                    current_tx = tx_queue.front();
                    tx_queue.pop();
                    current_state.write(STATE_SEND_HEADER);
                }
                break;

            case STATE_SEND_HEADER:
                if (credits > 0) {
                    Flit header;
                    header.framing = 0; // 00 = Header
                    
                    // TODO : Definir essa estrutura de mensagem como um tipo de dados
                    // Formato: bits 15-12 = dest, bits 11-8 = msg_id, bits 7-0 = size
                    sc_uint<16> payload = 0;
                    payload.range(15, 12) = current_tx.dest;
                    payload.range(11, 8) = current_tx.msg_id;
                    payload.range(7, 0) = current_tx.size;
                    header.payload = payload;
                    header.update_fields();

                    out_data.write(header);
                    out_valid.write(true);
                    body_count = 0;

                    std::cout << "[Core " << core_id << " at " << sc_time_stamp()
                              << "] Sending Header: " << header 
                              << " with target destination: " << current_tx.dest << std::endl;

                    // Usa o tamnanho restante do pacote pra dizer se manda o tail ou o body
                    if (current_tx.size > 0) {
                        current_state.write(STATE_SEND_BODY);
                    } else {
                        current_state.write(STATE_SEND_TAIL);
                    }
                } else {
                    out_valid.write(false);
                }
                break;

            case STATE_SEND_BODY:
                if (credits > 0) {
                    Flit body;
                    body.framing = 1; // 01 = Body
                    body.payload = 0x1000 + body_count; // Payload incremental, pra ajudar a identificar

                    out_data.write(body);
                    out_valid.write(true);
                    body_count++;

                    std::cout << "[Core " << core_id << " at " << sc_time_stamp()
                              << "] Sending Body (" << body_count << "/" << current_tx.size 
                              << "): " << body << std::endl;

                    if (body_count >= current_tx.size) {
                        current_state.write(STATE_SEND_TAIL);
                    }
                } else {
                    out_valid.write(false);
                }
                break;

            case STATE_SEND_TAIL:
                if (credits > 0) {
                    Flit tail;
                    tail.framing = 2; // 10 = Tail
                    tail.payload = 0xDEAD; // DONT CARE

                    out_data.write(tail);
                    out_valid.write(true);

                    std::cout << "[Core " << core_id << " at " << sc_time_stamp()
                              << "] Sending Tail: " << tail << std::endl;

                    current_state.write(STATE_SEND_IDLE);
                } else {
                    out_valid.write(false);
                }
                break;
        }
    }
}
