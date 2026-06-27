#ifndef FIFO_H
#define FIFO_H

#include <systemc.h>
#include "flit.h"

template <typename T, int DEPTH>
SC_MODULE(FifoBuffer) {
    sc_in<bool> clk;
    sc_in<bool> rst;

    // Sinal de escrita
    sc_in<bool> write_en;
    // Dado da escrita
    sc_in<T> data_in;

    // Sinal da leitura
    sc_in<bool> read_en;
    // Dado da leitura
    sc_out<T> data_out;

    // Flags de status
    sc_out<bool> full;
    sc_out<bool> empty;
    sc_out<sc_uint<DEPTH>> count; // Quantidade atual de itens

private:
    // NOTE : Usamos os ponteiros wr_ptr e rd_ptr pra manter logicamente quem esta na fila
    // ao inves de fazer escritas em memoria
    T buffer[DEPTH];
    int wr_ptr;         // Ponteiro de escrita
    int rd_ptr;         // Ponteiro de leitura
    int items_count;    // Contagem de Flits

    void fifo_process() {
        if (rst.read()) { 
            wr_ptr = 0;
            rd_ptr = 0;
            items_count = 0;
            full.write(false);
            empty.write(true);
            count.write(0);
            data_out.write(T());
            return;
        } 

        // Escrita
        if (write_en.read() && items_count < DEPTH) {
            // Escreve na posicao wr_ptr e avanca fazendo ciclo
            buffer[wr_ptr] = data_in.read();
            // NOTE : O mecanismo de controle por creditos teoricamente fara com que 
            // ao completar o ciclo, sobrescreva flits que ja sairam da fila
            wr_ptr = (wr_ptr + 1) % DEPTH;
            items_count++;
        }

        //  Leitura
        if (read_en.read() && items_count > 0) {
            rd_ptr = (rd_ptr + 1) % DEPTH;
            items_count--;
        }

        full.write(items_count == DEPTH);
        empty.write(items_count == 0);
        count.write(items_count);

        // Saida de dados
        if (items_count > 0) {
            data_out.write(buffer[rd_ptr]); // cabeca da fila
        } else {
            data_out.write(T());
        }
    }

public:
    SC_CTOR(FifoBuffer) {
        SC_METHOD(fifo_process);
        sensitive << clk.pos() << rst;
    }
};

#endif // FIFO_H
