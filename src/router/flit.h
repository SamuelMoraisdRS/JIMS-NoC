#ifndef FLIT_H
#define FLIT_H

#include <systemc.h>
#include <iostream>

struct Flit {
    sc_uint<2> framing;   // 00 = Header, 01 = Body, 10 = Tail
    sc_uint<16> payload;  // 16 bits de dados

    // Construtor padrão
    Flit() : framing(0), payload(0) {}

    // Construtor parametrizado
    Flit(sc_uint<2> f, sc_uint<16> p) : framing(f), payload(p) {}

    // Sobrecarga de operador de igualdade (necessário para sc_signal)
    bool operator==(const Flit& other) const {
        return (framing == other.framing) && (payload == other.payload);
    }

    // Sobrecarga de operador de atribuição
    Flit& operator=(const Flit& other) {
        if (this != &other) {
            framing = other.framing;
            payload = other.payload;
        }
        return *this;
    }

    // Sobrecarga do operador de impressão (cout)
    friend std::ostream& operator<<(std::ostream& os, const Flit& f) {
        os << "[F:" << f.framing.to_string(SC_BIN) 
           << ", P:0x" << std::hex << f.payload.to_uint() << std::dec << "]";
        return os;
    }

    // Função de rastreamento (trace) para gerar arquivos VCD
    friend void sc_trace(sc_trace_file* tf, const Flit& f, const std::string& name) {
        sc_trace(tf, f.framing, name + ".framing");
        sc_trace(tf, f.payload, name + ".payload");
    }
};

#endif // FLIT_H
