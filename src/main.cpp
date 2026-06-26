#include <systemc.h>
#include "noc.h"

// Processo auxiliar para gerar o sinal de clock
void generate_clock(sc_signal<bool>& clk, double period_ns) {
    while (true) {
        clk.write(true);
        wait(period_ns / 2, SC_NS);
        clk.write(false);
        wait(period_ns / 2, SC_NS);
    }
}
int sc_main(int argc, char* argv[]) {
    // 1. Sinais globais de clock e reset
    sc_signal<bool> clk;
    sc_signal<bool> rst;

    // 2. Instanciação do módulo NoC
    NoC* noc = new NoC("SPIN_NoC");
    noc->clk(clk);
    noc->rst(rst);

    // 3. Inicialização do Trace File VCD para GTKWave
    sc_trace_file* tf = sc_create_vcd_trace_file("noc_simulation");
    tf->set_time_unit(1, SC_NS);

    // Rastrear sinais globais
    sc_trace(tf, clk, "clk");
    sc_trace(tf, rst, "rst");

    // Rastrear conexões físicas selecionadas para depuração visual
    // Exemplo: Dados enviados pelo Core 0 e recebidos pelo Core 5
    sc_trace(tf, noc->core_to_leaf_data[0], "C0_to_R0_data");
    sc_trace(tf, noc->core_to_leaf_valid[0], "C0_to_R0_valid");
    sc_trace(tf, noc->leaf_to_core_credit[0], "R0_to_C0_credit");

    sc_trace(tf, noc->leaf_to_core_data[5], "R1_to_C5_data");
    sc_trace(tf, noc->leaf_to_core_valid[5], "R1_to_C5_valid");
    sc_trace(tf, noc->core_to_leaf_credit[5], "C5_to_R1_credit");

    sc_trace(tf, noc->core_to_leaf_data[14], "C14_to_R3_data");
    sc_trace(tf, noc->core_to_leaf_valid[14], "C14_to_R3_valid");
    sc_trace(tf, noc->leaf_to_core_credit[14], "R3_to_C14_credit");

    sc_trace(tf, noc->leaf_to_core_data[15], "R3_to_C15_data");
    sc_trace(tf, noc->leaf_to_core_valid[15], "R3_to_C15_valid");
    sc_trace(tf, noc->core_to_leaf_credit[15], "C15_to_R3_credit");

    // Rastrear barramentos entre roteadores
    sc_trace(tf, noc->leaf_to_root_data[0][0], "R0_to_R4_data");
    sc_trace(tf, noc->leaf_to_root_valid[0][0], "R0_to_R4_valid");

    // 4. Iniciar thread de Clock (período de 10 ns, i.e., 100 MHz)
    sc_spawn(sc_bind(&generate_clock, sc_ref(clk), 10.0));

    // 5. Sequência de Reset inicial
    std::cout << "[SIM] Aplicando sinal de Reset..." << std::endl;
    rst.write(true);
    sc_start(25, SC_NS); // Aguarda alguns ciclos em reset
    rst.write(false);
    std::cout << "[SIM] Reset desativado. Iniciando tráfego..." << std::endl;
    sc_start(10, SC_NS);

    // 6. Configuração e injeção de cenários de teste de mensagens
    std::cout << "\n==============================================" << std::endl;
    std::cout << "  INICIANDO TRANSMISSÃO DE PACOTES DE TESTE   " << std::endl;
    std::cout << "==============================================\n" << std::endl;

    // Cenário A: Comunicação Distante (Core 0 para Core 5)
    // Sobe via Roteador Folha 0, escolhe adaptativamente uma Raiz, e desce pelo Roteador Folha 1
    std::cout << "[SIM] Agendando pacote: Core 0 -> Core 5 (Tamanho: 4 flits de payload)" << std::endl;
    noc->cores[0]->send_packet(/*dest=*/5, /*size=*/4, /*msg_id=*/1);

    // Cenário B: Comunicação Local (Core 12 para Core 13)
    // Roteado diretamente na descida/local dentro do Roteador Folha 3, sem subir para as raízes
    std::cout << "[SIM] Agendando pacote: Core 12 -> Core 13 (Tamanho: 2 flits de payload)" << std::endl;
    noc->cores[12]->send_packet(/*dest=*/13, /*size=*/2, /*msg_id=*/2);

    // Cenário C: Comunicação Distante Concorrente (Core 7 para Core 0)
    std::cout << "[SIM] Agendando pacote: Core 7 -> Core 0 (Tamanho: 3 flits de payload)" << std::endl;
    noc->cores[7]->send_packet(/*dest=*/0, /*size=*/3, /*msg_id=*/3);

    // Cenário D: Comunicação Local usando novos núcleos (Core 14 para Core 15)
    std::cout << "[SIM] Agendando pacote: Core 14 -> Core 15 (Tamanho: 3 flits de payload)" << std::endl;
    noc->cores[14]->send_packet(/*dest=*/15, /*size=*/3, /*msg_id=*/4);

    // 7. Execução da Simulação
    // Executa por mais 300 ns para dar tempo dos pacotes transitarem totalmente pela rede
    sc_start(300, SC_NS);

    std::cout << "\n==============================================" << std::endl;
    std::cout << "           FIM DA SIMULAÇÃO DA NOC            " << std::endl;
    std::cout << "==============================================\n" << std::endl;

    // 8. Limpar recursos e fechar arquivo VCD
    sc_close_vcd_trace_file(tf);
    delete noc;

    std::cout << "[SIM] Simulação finalizada com sucesso. Arquivo 'noc_simulation.vcd' gerado." << std::endl;
    return 0;
}
