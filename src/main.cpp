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
