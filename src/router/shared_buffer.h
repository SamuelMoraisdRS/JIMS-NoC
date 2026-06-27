#ifndef SHARED_BUFFER_H
#define SHARED_BUFFER_H

#include <systemc.h>
#include "flit.h"
#include "fifo.h"

// Estrutura que une o flit com o metadado de destino
struct SharedFlit {
    Flit flit_data;
    sc_uint<4> original_dest;

    SharedFlit() : flit_data(Flit()), original_dest(0) {}
    SharedFlit(Flit f, sc_uint<4> dest) : flit_data(f), original_dest(dest) {}

    // Sobrecarga do operador de igualdade (necessário para sc_signal)
    bool operator==(const SharedFlit& other) const {
        return (flit_data == other.flit_data) && (original_dest == other.original_dest);
    }

    // Sobrecarga de operador de atribuição
    SharedFlit& operator=(const SharedFlit& other) {
        if (this != &other) {
            flit_data = other.flit_data;
            original_dest = other.original_dest;
        }
        return *this;
    }

    // Sobrecarga do operador de impressão (cout)
    friend std::ostream& operator<<(std::ostream& os, const SharedFlit& sf) {
        os << "[Shared - Flit: " << sf.flit_data << ", OrigDest: " << sf.original_dest.to_uint() << "]";
        return os;
    }

    // Função de rastreamento (trace) para gerar arquivos VCD
    friend void sc_trace(sc_trace_file* tf, const SharedFlit& sf, const std::string& name) {
        sc_trace(tf, sf.flit_data, name + ".flit_data");
        sc_trace(tf, sf.original_dest, name + ".original_dest");
    }
};

SC_MODULE(SharedBuffer) {
  // Portas globais
  sc_in<bool> clk;
  sc_in<bool> rst;

  // Crossbar
  sc_in<Flit> data_in;            // Flit de dados vindo da Crossbar
  sc_out<Flit> data_out;          // Joga o flit de volta na Crossbar para ir para a porta física
  
  // Árbitro
  sc_in<bool> write_en;           // Vem da linha write_en[8] ou write_en[9] do Árbitro
  sc_in<bool> read_en;            // Comando de POP enviado pelo Árbitro
  
  // RoutingUnit
  sc_in<sc_uint<4>> target_in;    // ID da porta D de destino original (0 a 3) onde colidiu
  sc_out<sc_uint<4>> head_dest;   // Expõe o destino original do flit na cabeça da fila
  sc_out<bool> empty;             // Avisa se o buffer está vazio
  sc_out<bool> full;              // Avisa se o buffer lotou (para gerar STALL nas portas de entrada)
  
private:
  // Instanciação da sua FIFO genérica passando o novo tipo estruturado
  // Podemos definir uma profundidade maior, ex: 8 ou 16 posições para tráfego compartilhado
  FifoBuffer<SharedFlit, 18>* fifo;

  // Sinais internos para empaquetar os dados antes de jogar na FIFO
  sc_signal<SharedFlit> fifo_in_pack;
  sc_signal<SharedFlit> fifo_out_pack;
  sc_signal<sc_uint<18>> fifo_count;

  void pack_logic() {
      // Bloco combinacional para montar a estrutura de entrada
      SharedFlit pack;
      pack.flit_data = data_in.read();
      pack.original_dest = target_in.read();
      fifo_in_pack.write(pack);

      // Bloco combinacional para desempacotar a saída da FIFO para as portas do módulo
      SharedFlit out_pack = fifo_out_pack.read();
      data_out.write(out_pack.flit_data);
      head_dest.write(out_pack.original_dest);
  }

  void log_process() {
      if (!rst.read()) {
          if (write_en.read()) {
              std::cout << "[" << this->name() << " at " << sc_time_stamp()
                        << "] ARMAZENANDO Flit no Buffer Compartilhado: " << data_in.read() << std::endl;
          }
          if (read_en.read()) {
              std::cout << "[" << this->name() << " at " << sc_time_stamp()
                        << "] LIBERANDO Flit do Buffer Compartilhado: " << data_out.read() << std::endl;
          }
      }
  }

public:
  SC_CTOR(SharedBuffer) {
      fifo = new FifoBuffer<SharedFlit, 18>("internal_shared_fifo");
      
      // Conexões da FIFO interna
      fifo->clk(clk);
      fifo->rst(rst);
      fifo->write_en(write_en);
      fifo->data_in(fifo_in_pack);
      fifo->read_en(read_en);
      fifo->data_out(fifo_out_pack);
      fifo->full(full);   
      fifo->empty(empty); 
      fifo->count(fifo_count);

      SC_METHOD(pack_logic);
      sensitive << data_in << target_in << fifo_out_pack;

      SC_METHOD(log_process);
      sensitive << clk.pos();
  }

  ~SharedBuffer() {
      delete fifo;
  }
};

#endif 
