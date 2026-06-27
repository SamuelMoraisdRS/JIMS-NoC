#include "noc.h"

NoC::NoC(sc_module_name name) : sc_module(name) {
    // Inicialização dos sinais dummy para portas não utilizadas
    dummy_flit_in.write(Flit());
    dummy_bool_in.write(false);

    // 1. Instanciação e parametrização dos Roteadores Folha (Level 0)
    for (int i = 0; i < 4; i++) {
        char r_name[30];
        sprintf(r_name, "LeafRouter_%d", i);
        leaf_routers[i] = new Router(r_name);
        leaf_routers[i]->clk(clk);
        leaf_routers[i]->rst(rst);

        // Configurações da RoutingUnit correspondente
        leaf_routers[i]->routing_unit->router_id = i;
        leaf_routers[i]->routing_unit->level = 0;
        leaf_routers[i]->routing_unit->min_addr = i * 4;
        leaf_routers[i]->routing_unit->max_addr = i * 4 + 3;
    }

    // 2. Instanciação e parametrização dos Roteadores Raiz (Level 1)
    for (int j = 0; j < 4; j++) {
        char r_name[30];
        sprintf(r_name, "RootRouter_%d", j);
        root_routers[j] = new Router(r_name);
        root_routers[j]->clk(clk);
        root_routers[j]->rst(rst);

        // Configurações da RoutingUnit correspondente
        root_routers[j]->routing_unit->router_id = 4 + j;
        root_routers[j]->routing_unit->level = 1;
        root_routers[j]->routing_unit->min_addr = 0;
        root_routers[j]->routing_unit->max_addr = 15; // Cobre todo o espaço de endereços
    }

    // 3. Instanciação dos 16 Cores/Processadores
    for (int c = 0; c < 16; c++) {
        char c_name[30];
        sprintf(c_name, "Core_%d", c);
        cores[c] = new Core(c_name);
        cores[c]->clk(clk);
        cores[c]->rst(rst);
        cores[c]->core_id = c;

        // Amarrar o Core aos seus barramentos de sinais
        cores[c]->out_data(core_to_leaf_data[c]);
        cores[c]->out_valid(core_to_leaf_valid[c]);
        cores[c]->credit_in(leaf_to_core_credit[c]);

        cores[c]->in_data(leaf_to_core_data[c]);
        cores[c]->in_valid(leaf_to_core_valid[c]);
        cores[c]->credit_out(core_to_leaf_credit[c]);
    }

    // 4. Conexões Core <-> Roteadores Folha (Level 0)
    // Regra:
    // Core 'c' pertence ao roteador 'c / 4'.
    // A porta D associada no roteador correspondente é dada por '3 - (c % 4)'.
    for (int c = 0; c < 16; c++) {
        int leaf_id = c / 4;
        int port_d = 3 - (c % 4);

        leaf_routers[leaf_id]->in_data[port_d](core_to_leaf_data[c]);
        leaf_routers[leaf_id]->in_valid[port_d](core_to_leaf_valid[c]);
        leaf_routers[leaf_id]->credit_out[port_d](leaf_to_core_credit[c]);

        leaf_routers[leaf_id]->out_data[port_d](leaf_to_core_data[c]);
        leaf_routers[leaf_id]->out_valid[port_d](leaf_to_core_valid[c]);
        leaf_routers[leaf_id]->credit_in[port_d](core_to_leaf_credit[c]);
    }

    // 5. Conexões Roteadores Folha <-> Roteadores Raiz
    // Topologia Fat-Tree de 2 níveis bipartida completa:
    // A porta de subida 'j' (U0-U3 correspondendo aos índices de porta 4 a 7) do Roteador Folha 'i'
    // conecta à porta de descida do Roteador Raiz 'j' (Down Port 3-i correspondente aos índices 0 a 3).
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int leaf_port_u = 4 + j;
            int root_port_d = 3 - i;

            // Transmissão Folha 'i' -> Raiz 'j'
            leaf_routers[i]->out_data[leaf_port_u](leaf_to_root_data[i][j]);
            leaf_routers[i]->out_valid[leaf_port_u](leaf_to_root_valid[i][j]);
            leaf_routers[i]->credit_in[leaf_port_u](root_to_leaf_credit[i][j]);

            root_routers[j]->in_data[root_port_d](leaf_to_root_data[i][j]);
            root_routers[j]->in_valid[root_port_d](leaf_to_root_valid[i][j]);
            root_routers[j]->credit_out[root_port_d](root_to_leaf_credit[i][j]);

            // Transmissão Raiz 'j' -> Folha 'i'
            root_routers[j]->out_data[root_port_d](root_to_leaf_data[j][i]);
            root_routers[j]->out_valid[root_port_d](root_to_leaf_valid[j][i]);
            root_routers[j]->credit_in[root_port_d](leaf_to_root_credit[j][i]);

            leaf_routers[i]->in_data[leaf_port_u](root_to_leaf_data[j][i]);
            leaf_routers[i]->in_valid[leaf_port_u](root_to_leaf_valid[j][i]);
            leaf_routers[i]->credit_out[leaf_port_u](leaf_to_root_credit[j][i]);
        }
    }

    // 6. Terminação das portas de subida U (4 a 7) dos Roteadores Raiz
    // Por estarem no topo da árvore Fat-Tree, elas são ligadas aos barramentos dummy inativos.
    int dummy_idx_flit = 0;
    int dummy_idx_bool = 0;
    for (int j = 0; j < 4; j++) {
        for (int p = 4; p < 8; p++) {
            root_routers[j]->in_data[p](dummy_flit_in);
            root_routers[j]->in_valid[p](dummy_bool_in);
            root_routers[j]->credit_out[p](dummy_bool_out_sig[dummy_idx_bool++]);
            root_routers[j]->out_data[p](dummy_flit_out[dummy_idx_flit++]);
            root_routers[j]->out_valid[p](dummy_bool_out_sig[dummy_idx_bool++]);
            root_routers[j]->credit_in[p](dummy_bool_in);
        }
    }
}

NoC::~NoC() {
    for (int i = 0; i < 4; i++) {
        delete leaf_routers[i];
    }
    for (int j = 0; j < 4; j++) {
        delete root_routers[j];
    }
    for (int c = 0; c < 16; c++) {
        delete cores[c];
    }
}
