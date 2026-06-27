#include "output_channel.h"

// Lógica Combinacional
void OutputChannel::combinational_logic() {
    // Atualiza continuamente a unidade de roteamento sobre quantos créditos restam neste canal
    credit_to_routing.write(credit_count.read());

    // Se o árbitro der ordens para transmitir e houver créditos, envia para fora do roteador
    if (write_en.read() && credit_count.read() > 0) {
        out_data.write(data_in.read());
        out_valid.write(true);
    } else {
        out_data.write(Flit()); // Retorna flit vazio (zerado)
        out_valid.write(false);
    }
}

// Lógica Sequencial (Sincronizada no Clock)
void OutputChannel::sequential_logic() {
    if (rst.read()) {
        // Todo buffer de entrada da NoC SPIN possui profundidade 4, logo começamos com 4 créditos
        credit_count.write(4);
    } else {
        sc_uint<8> current_credits = credit_count.read();
        
        // Condição de envio: o árbitro requisitou e de fato tínhamos créditos para gastar
        bool flit_sent = write_en.read() && (current_credits > 0);
        bool credit_received = credit_in.read();

        // Log de envio de flit
        if (flit_sent) {
            std::cout << "[" << this->name() << " at " << sc_time_stamp()
                      << "] Flit Saindo: " << data_in.read() << std::endl;
        }

        // Gerenciamento do contador ponto a ponto (Controle por Créditos)
        if (flit_sent && !credit_received) {
            credit_count.write(current_credits - 1);
        } 
        else if (!flit_sent && credit_received) {
            // Trava de segurança para não ultrapassar a capacidade máxima do buffer vizinho (4)
            if (current_credits < 4) {
                credit_count.write(current_credits + 1);
            }
        }
        // Se enviou um flit e recebeu um crédito no mesmo ciclo, o saldo é nulo (não altera)
    }
}
