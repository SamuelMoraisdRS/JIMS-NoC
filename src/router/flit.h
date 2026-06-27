#ifndef FLIT_H
#define FLIT_H

#include <systemc.h>
#include <iostream>

struct Flit {
    sc_uint<2> framing;   // 00 = Header, 01 = Body, 10 = Tail
    sc_uint<16> payload;  // 16 bits de dados

    // Helper fields for visualization in GTKWave
    sc_uint<4> dest_addr; // Target destination address (valid when framing == 0)
    sc_uint<4> msg_id;    // Message ID (valid when framing == 0)
    sc_uint<8> size;      // Payload size (valid when framing == 0)

    void update_fields() {
        if (framing == 0) { // Header
            dest_addr = payload.range(15, 12);
            msg_id = payload.range(11, 8);
            size = payload.range(7, 0);
        } else {
            dest_addr = 0;
            msg_id = 0;
            size = 0;
        }
    }

    // Construtor padrão
    Flit() : framing(0), payload(0), dest_addr(0), msg_id(0), size(0) {}

    // Construtor parametrizado
    Flit(sc_uint<2> f, sc_uint<16> p) : framing(f), payload(p) {
        update_fields();
    }

    // Construtor de cópia
    Flit(const Flit& other) {
        framing = other.framing;
        payload = other.payload;
        dest_addr = other.dest_addr;
        msg_id = other.msg_id;
        size = other.size;
        update_fields();
    }

    // Sobrecarga de operador de igualdade (necessário para sc_signal)
    bool operator==(const Flit& other) const {
        return (framing == other.framing) && (payload == other.payload) &&
               (dest_addr == other.dest_addr) && (msg_id == other.msg_id) && (size == other.size);
    }

    // Sobrecarga de operador de atribuição
    Flit& operator=(const Flit& other) {
        if (this != &other) {
            framing = other.framing;
            payload = other.payload;
            dest_addr = other.dest_addr;
            msg_id = other.msg_id;
            size = other.size;
            update_fields();
        }
        return *this;
    }

    // Sobrecarga do operador de impressão (cout)
    friend std::ostream& operator<<(std::ostream& os, const Flit& f) {
        os << "[F:" << f.framing.to_string(SC_BIN) 
           << ", P:0x" << std::hex << f.payload.to_uint() << std::dec;
        if (f.framing == 0) {
            os << " (Hdr dest:" << f.dest_addr.to_uint() 
               << " msg:" << f.msg_id.to_uint() 
               << " size:" << f.size.to_uint() << ")";
        }
        os << "]";
        return os;
    }

    // Função de rastreamento (trace) para gerar arquivos VCD
    friend void sc_trace(sc_trace_file* tf, const Flit& f, const std::string& name) {
        sc_trace(tf, f.framing, name + ".framing");
        sc_trace(tf, f.payload, name + ".payload");
        sc_trace(tf, f.dest_addr, name + ".dest_addr");
        sc_trace(tf, f.msg_id, name + ".msg_id");
        sc_trace(tf, f.size, name + ".size");
    }
};

#endif // FLIT_H
