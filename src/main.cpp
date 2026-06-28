#include <systemc.h>
#include <string>
#include <fstream>
#include <sstream>
#include "noc.h"

// Buffer auxiliar para duplicar a saída do cout (console + buffer de memória para arquivo txt)
class TeeBuf : public std::streambuf {
    std::streambuf* buf1;
    std::streambuf* buf2;
public:
    TeeBuf(std::streambuf* b1, std::streambuf* b2) : buf1(b1), buf2(b2) {}
protected:
    virtual int overflow(int c) override {
        if (c == EOF) return !EOF;
        int r1 = buf1->sputc(c);
        int r2 = buf2->sputc(c);
        return (r1 == EOF || r2 == EOF) ? EOF : c;
    }
    virtual int sync() override {
        int r1 = buf1->pubsync();
        int r2 = buf2->pubsync();
        return (r1 == 0 && r2 == 0) ? 0 : -1;
    }
};

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
    // Configura captura de logs em memória para exportação ao final da simulação
    std::stringstream log_buffer;
    std::streambuf* old_cout_buf = std::cout.rdbuf();
    TeeBuf tee_buf(old_cout_buf, log_buffer.rdbuf());
    std::cout.rdbuf(&tee_buf);

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

    // Rastrear conexões físicas de TODOS os 16 Cores
    for (int i = 0; i < 16; ++i) {
        std::string prefix = "Core_" + std::to_string(i);
        sc_trace(tf, noc->core_to_leaf_data[i], prefix + "_tx_data");
        sc_trace(tf, noc->core_to_leaf_valid[i], prefix + "_tx_valid");
        sc_trace(tf, noc->leaf_to_core_credit[i], prefix + "_tx_credit_in");

        sc_trace(tf, noc->leaf_to_core_data[i], prefix + "_rx_data");
        sc_trace(tf, noc->leaf_to_core_valid[i], prefix + "_rx_valid");
        sc_trace(tf, noc->core_to_leaf_credit[i], prefix + "_rx_credit_out");
    }

    // Rastrear conexões físicas entre Roteadores Folha (Level 0) e Roteadores Raiz (Level 1)
    for (int leaf = 0; leaf < 4; ++leaf) {
        for (int root = 0; root < 4; ++root) {
            std::string leaf_name = "Leaf_" + std::to_string(leaf);
            std::string root_name = "Root_" + std::to_string(root + 4);

            // Subida: Folha -> Raiz
            sc_trace(tf, noc->leaf_to_root_data[leaf][root], leaf_name + "_to_" + root_name + "_data");
            sc_trace(tf, noc->leaf_to_root_valid[leaf][root], leaf_name + "_to_" + root_name + "_valid");
            sc_trace(tf, noc->root_to_leaf_credit[leaf][root], root_name + "_to_" + leaf_name + "_credit_in");

            // Descida: Raiz -> Folha
            sc_trace(tf, noc->root_to_leaf_data[leaf][root], root_name + "_to_" + leaf_name + "_data");
            sc_trace(tf, noc->root_to_leaf_valid[leaf][root], root_name + "_to_" + leaf_name + "_valid");
            sc_trace(tf, noc->leaf_to_root_credit[leaf][root], leaf_name + "_to_" + root_name + "_credit_in");
        }
    }

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
    std::cout << "\n==========================================================================" << std::endl;
    std::cout << "  CENÁRIO 1: COMPARATIVO DE LATÊNCIA (LOCAL VS. DISTANTE - MESMO TAMANHO)  " << std::endl;
    std::cout << "==========================================================================\n" << std::endl;

    // Pacote A (Comunicação Local - Mesmo Roteador Folha): Core 0 -> Core 1
    // Roteado diretamente internamente no LeafRouter 0 (D3 -> D2), sem subir para roteadores raiz.
    std::cout << "[SIM] Agendando Pacote A (LOCAL): Core 0 -> Core 1 (Mesmo LeafRouter 0 | Tamanho: 4 flits payload, msg_id: 1)" << std::endl;
    noc->cores[0]->send_packet(/*dest=*/1, /*size=*/4, /*msg_id=*/1);

    // Pacote B (Comunicação Distante - Roteadores Folha Diferentes): Core 12 -> Core 5
    // Roteado subindo do LeafRouter 3 para o RootRouter e descendo para o LeafRouter 1.
    std::cout << "[SIM] Agendando Pacote B (DISTANTE): Core 12 -> Core 5 (LeafRouter 3 -> LeafRouter 1 | Tamanho: 4 flits payload, msg_id: 2)" << std::endl;
    noc->cores[12]->send_packet(/*dest=*/5, /*size=*/4, /*msg_id=*/2);

    // 7. Execução do Cenário 1 (Tráfego Padrão)
    sc_start(200, SC_NS);

    std::cout << "\n==========================================================================" << std::endl;
    std::cout << "  CENÁRIO 2: CONCORRÊNCIA E TRÁFEGO SIMULTÂNEO (MÚLTIPLOS FLUXOS EM PARALELO)  " << std::endl;
    std::cout << "==========================================================================\n" << std::endl;
    std::cout << "[SIM] Injetando 5 pacotes simultâneos cruzando a rede em direções opostas..." << std::endl;

    // Fluxo 1: Core 2 -> Core 7 (Leaf 0 -> Leaf 1)
    std::cout << "[SIM] Agendando Fluxo 1: Core 2 -> Core 7 (Tamanho: 3 flits payload, msg_id: 10)" << std::endl;
    noc->cores[2]->send_packet(/*dest=*/7, /*size=*/3, /*msg_id=*/10);

    // Fluxo 2: Core 6 -> Core 1 (Leaf 1 -> Leaf 0)
    std::cout << "[SIM] Agendando Fluxo 2: Core 6 -> Core 1 (Tamanho: 3 flits payload, msg_id: 11)" << std::endl;
    noc->cores[6]->send_packet(/*dest=*/1, /*size=*/3, /*msg_id=*/11);

    // Fluxo 3: Core 10 -> Core 14 (Leaf 2 -> Leaf 3)
    std::cout << "[SIM] Agendando Fluxo 3: Core 10 -> Core 14 (Tamanho: 4 flits payload, msg_id: 12)" << std::endl;
    noc->cores[10]->send_packet(/*dest=*/14, /*size=*/4, /*msg_id=*/12);

    // Fluxo 4: Core 15 -> Core 11 (Leaf 3 -> Leaf 2)
    std::cout << "[SIM] Agendando Fluxo 4: Core 15 -> Core 11 (Tamanho: 3 flits payload, msg_id: 13)" << std::endl;
    noc->cores[15]->send_packet(/*dest=*/11, /*size=*/3, /*msg_id=*/13);

    // Fluxo 5: Core 4 -> Core 5 (Local no Leaf 1)
    std::cout << "[SIM] Agendando Fluxo 5: Core 4 -> Core 5 (Tamanho: 2 flits payload, msg_id: 14)" << std::endl;
    noc->cores[4]->send_packet(/*dest=*/5, /*size=*/2, /*msg_id=*/14);

    // Executa por 250 ns para garantir a entrega simultânea satisfatória de todos os fluxos
    sc_start(250, SC_NS);

    std::cout << "\n==========================================================" << std::endl;
    std::cout << "  CENÁRIO 3: DEMONSTRAÇÃO DOS BUFFERS COMPARTILHADOS QUP E QDN " << std::endl;
    std::cout << "==========================================================\n" << std::endl;
    std::cout << "[SIM] Simulando estouro de buffer no nó destino (Core 1 fica BUSY)..." << std::endl;
    noc->cores[1]->rx_busy = true;

    std::cout << "[SIM] Injetando rajada concorrente para o mesmo destino (Core 1)..." << std::endl;

    // Transmissão A: Core 2 -> Core 1 (Local, porta D1 -> D2)
    noc->cores[2]->send_packet(/*dest=*/1, /*size=*/4, /*msg_id=*/20);

    // Transmissão B: Core 3 -> Core 1 (Local, porta D0 -> D2. Tamanho ajustado para 2 flits payload = 4 flits no total)
    noc->cores[3]->send_packet(/*dest=*/1, /*size=*/2, /*msg_id=*/21);

    // Transmissão C: Core 8 -> Core 1 (Distante, vem por porta U. Ocupará QUP quando fechar o crédito de D2)
    noc->cores[8]->send_packet(/*dest=*/1, /*size=*/3, /*msg_id=*/22);

    // Executa por 120 ns com o Core 1 retendo créditos para estourar o crédito e forçar uso do QDN/QUP
    sc_start(120, SC_NS);

    std::cout << "\n[SIM] Liberando processamento do Core 1 (Core 1 volta a processar e devolver créditos)..." << std::endl;
    noc->cores[1]->rx_busy = false;

    // Executa por mais 500 ns para observar todos os flits saindo dos buffers e canais
    sc_start(500, SC_NS);

    std::cout << "\n==============================================" << std::endl;
    std::cout << "           FIM DA SIMULAÇÃO DA NOC            " << std::endl;
    std::cout << "==============================================\n" << std::endl;

    // 8. Limpar recursos e fechar arquivo VCD
    sc_close_vcd_trace_file(tf);
    delete noc;

    std::cout << "[SIM] Simulação finalizada com sucesso. Arquivo 'noc_simulation.vcd' gerado." << std::endl;

    // Restaurar std::cout e escrever os logs no arquivo .txt
    std::cout.rdbuf(old_cout_buf);
    std::ofstream log_file("simulation_logs.txt");
    if (log_file.is_open()) {
        log_file << log_buffer.str();
        log_file.close();
        std::cout << "[SIM] Logs da simulação salvos com sucesso no arquivo 'simulation_logs.txt'." << std::endl;
    }

    return 0;
}
