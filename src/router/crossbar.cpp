#include "crossbar.h"

// Essa implementação é de uma crossbar completa 10x10, porém a routing unit e o árbitro impedirão as conexões inválidas
void Crossbar::switch_logic() {
    // Varre cada uma das 10 portas de saída possíveis da matriz
    for (int out_idx = 0; out_idx < 10; out_idx++) {
        // Se o árbitro indicar que há uma conexão válida estabelecida para esta saída
        if (connection_valid[out_idx].read()) {
            // Lê qual canal de entrada foi mapeado para esta saída
            sc_uint<4> in_idx = sel_input[out_idx].read();
            // Faz a conexão direta fio-a-fio
            out_data[out_idx].write(in_data[in_idx].read());
        } else {
            // Se não houver nenhuma entrada conectada a essa saída, ela fica ociosa (Zera o barramento)
            out_data[out_idx].write(Flit());
        }
    }
}