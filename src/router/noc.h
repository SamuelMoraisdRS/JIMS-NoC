#ifndef NOC_H
#define NOC_H

#include <systemc.h>
#include "router.h"
#include "core.h"

SC_MODULE(NoC) {
    sc_in<bool> clk;
    sc_in<bool> rst;

    // Ponteiros para os roteadores
    Router* leaf_routers[4]; // Level 0 (Roteadores 0 a 3)
    Router* root_routers[4]; // Level 1 (Roteadores 4 a 7)

    // Ponteiros para os cores
    Core* cores[16]; // Cores 0 a 15

    // Sinais para conexoes Core <-> Roteador Folha
    sc_signal<Flit> core_to_leaf_data[16];
    sc_signal<bool> core_to_leaf_valid[16];
    sc_signal<bool> leaf_to_core_credit[16];

    sc_signal<Flit> leaf_to_core_data[16];
    sc_signal<bool> leaf_to_core_valid[16];
    sc_signal<bool> core_to_leaf_credit[16];

    // Sinais para conexões Roteador Folha <-> Roteador Raiz
    // Folha [i] -> Raiz [j]
    sc_signal<Flit> leaf_to_root_data[4][4];
    sc_signal<bool> leaf_to_root_valid[4][4];
    sc_signal<bool> root_to_leaf_credit[4][4];

    // Raiz [j] -> Folha [i]
    sc_signal<Flit> root_to_leaf_data[4][4];
    sc_signal<bool> root_to_leaf_valid[4][4];
    sc_signal<bool> leaf_to_root_credit[4][4];

    // Sinais Dummy para portas desconectadas
    sc_signal<Flit> dummy_flit_in;
    sc_signal<bool> dummy_bool_in;
    sc_signal<Flit> dummy_flit_out[20];
    sc_signal<bool> dummy_bool_out_sig[40];

    SC_CTOR(NoC);
    ~NoC();
};

#endif // NOC_H
