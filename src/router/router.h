#ifndef ROUTER_H
#define ROUTER_H

#include <systemc.h>
#include "flit.h"
#include "input_channel.h"
#include "output_channel.h"
#include "routing_unit.h"
#include "arbiter.h"
#include "shared_buffer.h"
#include "crossbar.h"

SC_MODULE(Router) {
  // 1. Portas Globais
  sc_in<bool> clk;
  sc_in<bool> rst;

  // 2. Interfaces Físicas Externas
  sc_in<Flit> in_data[8];
  sc_in<bool> in_valid[8];
  sc_out<bool> credit_out[8];

  sc_out<Flit> out_data[8];
  sc_out<bool> out_valid[8];
  sc_in<bool> credit_in[8];

  // 3. Sinais Internos
  // Sinais ligando Input Channels -> Árbitro
  sc_signal<bool> sig_release_route[8];
  sc_signal<bool> sig_route_grant[8];
  sc_signal<bool> sig_read_en[8];
  
  // Sinais ligando Input Channels -> Routing Unit 
  sc_signal<bool> sig_req_route[8];
  sc_signal<sc_uint<4>> sig_dest_addr[8];

  // Sinais ligando RoutingUnit -> Árbitro
  sc_signal<bool> sig_req_valid[8];
  sc_signal<sc_uint<4>> sig_req_port[8];

  // Sinais de Crédito ligando OutputChannels -> RoutingUnit
  sc_signal<sc_uint<8>> sig_output_credits[8];

  // Sinais dedicados para os Buffers Compartilhados (QDN = 8, QUP = 9)
  // Controle: RU <-> Árbitro
  sc_signal<bool> sig_qup_req_valid;
  sc_signal<sc_uint<4>> sig_qup_req_port;
  sc_signal<bool> sig_qdn_req_valid;
  sc_signal<sc_uint<4>> sig_qdn_req_port;

  // Estado dos Buffers -> RU 
  sc_signal<bool> sig_qup_empty;
  sc_signal<bool> sig_qup_full;
  sc_signal<sc_uint<4>> sig_qup_head_dest;
  sc_signal<bool> sig_qdn_empty;
  sc_signal<bool> sig_qdn_full;
  sc_signal<sc_uint<4>> sig_qdn_head_dest;

  // Comandos de POP (Leitura) do Árbitro -> Buffers
  sc_signal<bool> sig_qup_read_en;
  sc_signal<sc_uint<4>> sig_qup_target_out;
  sc_signal<bool> sig_qdn_read_en;
  sc_signal<sc_uint<4>> sig_qdn_target_out;

  // Sinais de barramento que alimentam a Crossbar (Entradas 0-9)
  sc_signal<Flit> sig_crossbar_inputs[10];

  // Sinais de barramento que saem da Crossbar (Saídas 0-9)
  sc_signal<Flit> sig_crossbar_outputs[10];

  // Linhas de Controle do Árbitro -> Crossbar e OutputChannels/Buffers
  sc_signal<bool> sig_write_en[10];
  sc_signal<sc_uint<4>> sig_sel_input[10];
  sc_signal<bool> sig_connection_valid[10];

  // 4. Ponteiros para os submódulos
  InputChannel* input_channels[8];
  OutputChannel* output_channels[8];
  RoutingUnit* routing_unit;
  Arbiter* arbiter;
  SharedBuffer* q_up;
  SharedBuffer* q_dn;
  Crossbar* crossbar;  

  void bus_wiring_matrix();

  SC_CTOR(Router);
  ~Router();
};

#endif