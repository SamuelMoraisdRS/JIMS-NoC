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

    // Input Channels
    sc_out<bool> route_grant[8];
    sc_out<bool> read_en[8];

    // Crossbar
    sc_out<sc_uint<4>> sel_input[10];
    sc_out<bool> connection_valid[10];

private:

    bool output_locked[10];
    int output_owner[10];

    int rr_up;
    int rr_dn;

    void arbitration_process();

public:

    SC_CTOR(Arbiter)
    {
        rr_up = 0;
        rr_dn = 0;

        for(int i = 0; i < 10; i++)
        {
            output_locked[i] = false;
            output_owner[i] = -1;
        }

        SC_METHOD(arbitration_process);
        sensitive << clk.pos();
        dont_initialize();
    }
};

#endif