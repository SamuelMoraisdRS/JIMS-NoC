#include "arbiter.h"

// =========================================================================
// 1. LÓGICA COMBINACIONAL (Recalcula conexões e controle instantaneamente)
// =========================================================================
void Arbiter::combinational_logic() {
    // 1. Inicializa todos os sinais de controle com valores padrão (desativados)
    for (int i = 0; i < 8; i++) {
        route_grant[i].write(false);
        read_en[i].write(false);
    }
    for (int i = 0; i < 10; i++) {
        connection_valid[i].write(false);
        sel_input[i].write(0);
        write_en[i].write(false);
    }

    // Criamos cópias locais do estado dos registradores para manipulação combinacional interna
    bool local_locked[10];
    int local_owner[10];
    for (int i = 0; i < 10; i++) {
        local_locked[i] = output_locked[i];
        local_owner[i] = output_owner[i];
    }

    // 2. Mantém e ativa as conexões Wormhole estáveis (O flit passa antes de limpar a rota)
    for (int out = 0; out < 10; out++) {
        if (local_locked[out]) {
            int owner = local_owner[out];

            // Canais físicos de entrada vão de 0 a 7. Canais 8 (QDN) e 9 (QUP) 
            // controlam seus próprios buffers internos.
            if (owner < 8) {
                route_grant[owner].write(true);
                read_en[owner].write(true);
            }
            
            write_en[out].write(true);
            sel_input[out].write(owner);
            connection_valid[out].write(true);
        }
    }

    // 3. Liberação de rota combinacional (se o release_route foi ativado, limpa locais pós-uso)
    for (int in = 0; in < 8; in++) {
        if (release_route[in].read()) {
            for (int out = 0; out < 10; out++) {
                if (local_owner[out] == in) {
                    local_locked[out] = false;
                    local_owner[out] = -1;
                }
            }
        }
    }

    // 4. Montagem da lista de prioridade de ENTRADA: QUP (9) > QDN (8) > U (4-7) > D (0-3)
    int input_priority_list[10];
    int idx = 0;

    // Alta Prioridade: Buffers Compartilhados
    input_priority_list[idx++] = 9; // QUP primeiro
    input_priority_list[idx++] = 8; // QDN segundo

    // Média Prioridade: Portas U (4 a 7) com Round-Robin escalonado entre as 4
    for (int count = 0; count < 4; count++) {
        input_priority_list[idx++] = 4 + ((rr_up + count) % 4);
    }

    // Baixa Prioridade: Portas D (0 a 3) com Round-Robin escalonado entre as 4
    for (int count = 0; count < 4; count++) {
        input_priority_list[idx++] = (rr_dn + count) % 4;
    }

    // 5. Varredura e Atendimento seguindo a Hierarquia Estrita de Entrada
    for (int i = 0; i < 10; i++) {
        int in = input_priority_list[i];

        // Buffers compartilhados (8 e 9) usam lógica de controle diferente dos canais físicos (0-7)
        // Se for canal físico, checa a requisição da Routing Unit.
        bool has_request = false;
        int target_port = -1;

        if (in < 8) {
            has_request = req_valid[in].read();
            target_port = req_port[in].read().to_int();
        } else {
            // TODO: deve-se mapear os sinais dos buffers compartilhados aqui para ativar o has_request e target_port.
            has_request = false; 
            target_port = -1;
        }

        if (has_request && target_port >= 0 && target_port < 10) {
            // Só aloca se a porta de saída desejada estiver livre neste ciclo
            if (!local_locked[target_port]) {
                
                // Restrição de hardware: Canais D (0-3) não podem acessar o QUP (Saída 9)
                if (target_port == 9 && in < 4) {
                    continue;
                }

                // Ativa os sinais de controle combinacionais
                if (in < 8) {
                    route_grant[in].write(true);
                    read_en[in].write(true);
                }
                write_en[target_port].write(true);
                sel_input[target_port].write(in);
                connection_valid[target_port].write(true);

                // Trava a porta localmente para evitar que entradas de menor prioridade a roubem
                local_locked[target_port] = true;
                local_owner[target_port] = in;
            }
        }
    }
}

// =========================================================================
// 2. LÓGICA SEQUENCIAL (Atualiza registradores de estado na borda do Clock)
// =========================================================================
void Arbiter::sequential_logic() {
    if (rst.read()) {
        rr_up = 0;
        rr_dn = 0;
        for (int i = 0; i < 10; i++) {
            output_locked[i] = false;
            output_owner[i] = -1;
        }
        return;
    }

    // 1. Atualiza as liberações de rota físicas nos registradores estáveis
    for (int in = 0; in < 8; in++) {
        if (release_route[in].read()) {
            for (int out = 0; out < 10; out++) {
                if (output_owner[out] == in) {
                    output_locked[out] = false;
                    output_owner[out] = -1;
                }
            }
        }
    }

    // 2. Monta a mesma árvore de prioridades para sincronizar o estado dos registradores
    int input_priority_list[10];
    int idx = 0;
    input_priority_list[idx++] = 9; // QUP
    input_priority_list[idx++] = 8; // QDN
    for (int count = 0; count < 4; count++) input_priority_list[idx++] = 4 + ((rr_up + count) % 4);
    for (int count = 0; count < 4; count++) input_priority_list[idx++] = (rr_dn + count) % 4;

    // 3. Salva os novos donos de rotas estabelecidas e avança os ponteiros Round-Robin
    for (int i = 0; i < 10; i++) {
        int in = input_priority_list[i];

        bool has_request = false;
        int target_port = -1;

        if (in < 8) {
            has_request = req_valid[in].read();
            target_port = req_port[in].read().to_int();
        }

        if (has_request && target_port >= 0 && target_port < 10) {
            if (!output_locked[target_port]) {
                if (target_port == 9 && in < 4) continue;

                // Grava permanentemente na memória do Árbitro
                output_locked[target_port] = true;
                output_owner[target_port] = in;

                // Atualiza o avanço do ponteiro Round-Robin da classe correspondente
                if (in >= 4 && in <= 7) {
                    // Avança a prioridade para a próxima porta da classe U
                    rr_up = (in - 4 + 1) % 4;
                } 
                else if (in >= 0 && in <= 3) {
                    // Avança a prioridade para a próxima porta da classe D
                    rr_dn = (in + 1) % 4;
                }
            }
        }
    }
}