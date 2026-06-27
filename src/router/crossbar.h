#ifndef CROSSBAR_H
#define CROSSBAR_H

#include <systemc.h>
#include "flit.h"

SC_MODULE(Crossbar) {
    // --- Interfaces de Entrada (Dados vindo dos buffers) ---
    // 0 a 7: Canais de entrada físicos. 8: Saída do QDN. 9: Saída do QUP.
    sc_in<Flit> in_data[10];

    // --- Interfaces de Saída (Dados indo para os canais de saída) ---
    // 0 a 7: Canais de saída físicos. 8: Entrada do QDN. 9: Entrada do QUP.
    sc_out<Flit> out_data[10];

    // --- Sinais de Controle (Vindo do Árbitro) ---
    // Cada saída precisa saber qual entrada (0 a 9) deve se conectar a ela.
    sc_in<sc_uint<4>> sel_input[10]; 
    
    // Indica se a conexão para aquela saída específica está ativa neste ciclo
    sc_in<bool> connection_valid[10];

    void switch_logic();

    SC_CTOR(Crossbar) {
        SC_METHOD(switch_logic);
        // A crossbar reage instantaneamente a qualquer mudança nos dados ou no controle
        for (int i = 0; i < 10; i++) {
            sensitive << in_data[i] << sel_input[i] << connection_valid[i];
        }
    }
};

#endif // CROSSBAR_H