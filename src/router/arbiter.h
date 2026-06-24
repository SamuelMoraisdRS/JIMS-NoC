#ifndef ARBITER_H
#define ARBITER_H

#include <systemc.h>

SC_MODULE(Arbiter)
{
    sc_in<bool> clk;
    sc_in<bool> rst;

    // Routing Unit
    sc_in<bool> req_valid[8];
    sc_in<sc_uint<4>> req_port[8];

    // Input Channels
    sc_in<bool> release_route[8];
    sc_out<bool> route_grant[8];
    sc_out<bool> read_en[8];

    // Output Channels e buffers compartilhados
    sc_out<bool> write_en[10];

    // Crossbar
    sc_out<sc_uint<4>> sel_input[10];
    sc_out<bool> connection_valid[10];

private:

    bool output_locked[10];
    int output_owner[10];

    int rr_up;
    int rr_dn;

    void combinational_logic();
    void sequential_logic();

public:

    SC_CTOR(Arbiter)
    {
        SC_METHOD(combinational_logic);
        for(int i = 0; i < 8; i++) {
            sensitive << req_valid[i] << req_port[i] << release_route[i];
        }

        SC_METHOD(sequential_logic);
        sensitive << clk.pos() << rst;
        dont_initialize();
    }
};

#endif