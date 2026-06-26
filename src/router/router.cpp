#include "router.h"

void Router::bus_wiring_matrix() {
  // Portas de dados estão conectadas diretamente por binding no construtor
}

Router::Router(sc_module_name name) : sc_module(name) {
  // 1. INSTANCIAÇÃO E CONEXÃO DA UNIDADE DE ROTEAMENTO (RoutingUnit)
  // A RU calcula os caminhos com base nos endereços de destino dos cabeçalhos.
  routing_unit = new RoutingUnit("RU_Inst");
  
  // Conexões globais e de controle com os buffers compartilhados
  routing_unit->qup_empty(sig_qup_empty);
  routing_unit->qup_full(sig_qup_full);
  routing_unit->qup_head_dest(sig_qup_head_dest);
  routing_unit->qdn_empty(sig_qdn_empty);
  routing_unit->qdn_full(sig_qdn_full);
  routing_unit->qdn_head_dest(sig_qdn_head_dest);
  
  // Conexões de requisição de saída dos buffers compartilhados que a RU manda em direção ao Árbitro para arbitragem
  routing_unit->qup_req_port(sig_qup_req_port);
  routing_unit->qdn_req_port(sig_qdn_req_port);
  routing_unit->qup_req_valid(sig_qup_req_valid);
  routing_unit->qdn_req_valid(sig_qdn_req_valid);

  // Conexões em vetor para as 8 portas físicas
  for(int i = 0; i < 8; i++) {
      routing_unit->req_route[i](sig_req_route[i]);       // Entrada: Canal físico 'i' pede rota
      routing_unit->dest_addr[i](sig_dest_addr[i]);       // Entrada: Destino que o canal extraiu do flit
      routing_unit->output_credits[i](sig_output_credits[i]); // Entrada: Créditos disponíveis na saída 'i'
      routing_unit->req_port[i](sig_req_port[i]);         // Saída: Porta calculada pela RU para o canal 'i'
      routing_unit->req_valid[i](sig_req_valid[i]);       // Saída: Sinaliza ao Árbitro que a requisição de 'i' está pronta
  }


  // 2. INSTANCIAÇÃO E CONEXÃO DO ÁRBITRO (Arbiter)
  // O Árbitro dita quem ganha o direito de usar a Crossbar e concede as leituras.
  arbiter = new Arbiter("Arbiter_Inst");
  arbiter->clk(clk);
  arbiter->rst(rst);
  
  // Escuta as requisições virtuais que a RU calculou para o esvaziamento dos buffers
  arbiter->qup_req_valid(sig_qup_req_valid);
  arbiter->qup_req_port(sig_qup_req_port);
  arbiter->qdn_req_valid(sig_qdn_req_valid);
  arbiter->qdn_req_port(sig_qdn_req_port);
  
  // Manda os comandos de POP (leitura) para liberar espaço nos buffers compartilhados
  arbiter->qup_read_en(sig_qup_read_en);
  arbiter->qdn_read_en(sig_qdn_read_en);
  
  // Envia o carimbo do destino original para salvar na FIFO dos buffers (Abordagem Centralizada)
  arbiter->qup_target_out(sig_qup_target_out);
  arbiter->qdn_target_out(sig_qdn_target_out);

  // Amarra as conexões em vetor de controle das 8 portas físicas
  for(int i = 0; i < 8; i++) {
      arbiter->req_valid[i](sig_req_valid[i]);         // Entrada: Requisição da RU
      arbiter->req_port[i](sig_req_port[i]);           // Entrada: Porta alvo calculada pela RU
      arbiter->release_route[i](sig_release_route[i]); // Entrada: Canal físico avisa que o pacote (Tail) acabou
      arbiter->route_grant[i](sig_route_grant[i]);     // Saída: Árbitro concede a rota para o canal físico
      arbiter->read_en[i](sig_read_en[i]);             // Saída: Árbitro manda o canal físico dar POP (ler flit)
  }

  // Amarra as saídas de controle que governam os 10 canais da Crossbar (0-7 Físicos, 8 QDN, 9 QUP)
  for(int i = 0; i < 10; i++) {
      arbiter->write_en[i](sig_write_en[i]);               // Habilita escrita no canal de destino 'i'
      arbiter->sel_input[i](sig_sel_input[i]);             // Escolhe qual entrada (0-9) vai se conectar na saída 'i'
      arbiter->connection_valid[i](sig_connection_valid[i]); // Valida que a conexão chaveada está ativa
  }


  // 3. INSTANCIAÇÃO DOS BUFFERS COMPARTILHADOS (QDN e QUP)
  // Estruturas de memórias centrais compartilhadas para evitar Deadlock na árvore.
  
  // Buffer de Descida (Fica na posição lógica 8 do Roteador)
  q_dn = new SharedBuffer("QDN_Buffer");
  q_dn->clk(clk); q_dn->rst(rst);
  q_dn->write_en(sig_write_en[8]);       // Árbitro ativa a entrada dele se alguém desviar para o QDN
  q_dn->target_in(sig_qdn_target_out);    // Recebe o carimbo de destino que o árbitro capturou
  q_dn->read_en(sig_qdn_read_en);        // Recebe ordem de POP do árbitro quando vai esvaziar
  q_dn->empty(sig_qdn_empty);            // Avisa a RU se está vazio
  q_dn->full(sig_qdn_full);              // Avisa a RU se está lotado (para gerar Stall preventivo nas portas)
  q_dn->head_dest(sig_qdn_head_dest);    // Expõe para a RU o destino do flit que está no topo da fila
  q_dn->data_out(sig_crossbar_inputs[8]); // Conecta a saída de dados do buffer à Crossbar

  // Buffer de Subida (Fica na posição lógica 9 do Roteador)
  q_up = new SharedBuffer("QUP_Buffer");
  q_up->clk(clk); q_up->rst(rst);
  q_up->write_en(sig_write_en[9]);       // Árbitro ativa a entrada dele se alguém desviar para o QUP
  q_up->target_in(sig_qup_target_out);    
  q_up->read_en(sig_qup_read_en);        
  q_up->empty(sig_qup_empty);
  q_up->full(sig_qup_full);
  q_up->head_dest(sig_qup_head_dest);
  q_up->data_out(sig_crossbar_inputs[9]); // Conecta a saída de dados do buffer à Crossbar


  // 4. MALHA DE PORTAS FÍSICAS EXTERNAS (InputChannels e OutputChannels)
  // Mapeamento e amarração dos blocos que conversam com o mundo de fora do chip.
  for (int i = 0; i < 8; i++) {
      char name_in[20], name_out[20];
      sprintf(name_in, "InChannel_%d", i);
      sprintf(name_out, "OutChannel_%d", i);

      // Instancia o Receptor Físico 'i'
      input_channels[i] = new InputChannel(name_in);
      input_channels[i]->clk(clk); input_channels[i]->rst(rst);
      
      // CONEXÕES COM O MUNDO EXTERNO (Vêm do encapsulamento do Roteador)
      input_channels[i]->in_data(in_data[i]);       // Recebe os bits físicos do roteador vizinho
      input_channels[i]->in_valid(in_valid[i]);     // Fio que avisa se o vizinho está mandando algo válido
      input_channels[i]->credit_out(credit_out[i]);  // Envia créditos de volta para o vizinho saber que temos espaço
      
      // CONEXÕES DE SINALIZAÇÃO INTERNA (Conversa com RU e Árbitro)
      input_channels[i]->req_route(sig_req_route[i]);
      input_channels[i]->dest_addr(sig_dest_addr[i]);
      input_channels[i]->release_route(sig_release_route[i]);
      input_channels[i]->route_grant(sig_route_grant[i]);
      input_channels[i]->read_en(sig_read_en[i]);
      input_channels[i]->data_out(sig_crossbar_inputs[i]); // Conecta a saída do canal à Crossbar
      input_channels[i]->empty(sig_input_empty[i]);        // Conecta a sinalização de vazio ao sinal correspondente

      // Instancia o Transmissor Físico 'i'
      output_channels[i] = new OutputChannel(name_out);
      output_channels[i]->clk(clk); output_channels[i]->rst(rst);
      
      // Alimenta a RU com a quantidade estável de créditos que essa saída tem no momento
      output_channels[i]->credit_to_routing(sig_output_credits[i]);
      
      // CONEXÕES COM O MUNDO EXTERNO (Transmite para fora do chip)
      output_channels[i]->out_data(out_data[i]);     // Cospe o flit em direção ao roteador vizinho
      output_channels[i]->out_valid(out_valid[i]);   // Avisa o vizinho que o dado na linha é estável e válido
      output_channels[i]->credit_in(credit_in[i]);   // Escuta o vizinho devolvendo créditos para nós
      
      // CONTROLE DE ESCRITA (Vindo do Árbitro)
      output_channels[i]->write_en(sig_write_en[i]);

      // BARRAMENTO DE ENTRADA DE DADOS: O canal de saída 'i' fica escutando a porta de saída 'i' da Crossbar
      output_channels[i]->data_in(sig_crossbar_outputs[i]); 
  }


  // 5. FECHAMENTO DOS FIOS DE DADOS DOS BUFFERS (Entrada e Saída na Crossbar)
  // Agora precisamos conectar as SAÍDAS 8 e 9 da Crossbar para que elas entrem jogando dados DENTRO dos buffers!
  q_dn->data_in(sig_crossbar_outputs[8]); // O que sai da Crossbar na porta 8 entra para ser armazenado no QDN
  q_up->data_in(sig_crossbar_outputs[9]); // O que sai da Crossbar na porta 9 entra para ser armazenado no QUP


  // 6. INSTANCIAÇÃO E CONEXÃO DA MATRIZ DE CHAVEAMENTO (Crossbar 10x10)
  // O grande entroncamento do circuito que conecta fisicamente qualquer entrada em qualquer saída.
  crossbar = new Crossbar("Crossbar_Inst");
  for(int i = 0; i < 10; i++) {
      crossbar->in_data[i](sig_crossbar_inputs[i]);   // Alimentada pelo barramento gerenciado no bus_wiring_matrix
      crossbar->out_data[i](sig_crossbar_outputs[i]); // Espalha os flits roteados para as saídas físicas/buffers
      crossbar->sel_input[i](sig_sel_input[i]);             // Configurada pelo Árbitro (Diz quem passa para onde)
      crossbar->connection_valid[i](sig_connection_valid[i]);    // Habilitada pelo Árbitro
  }
}

// =========================================================================
// DESTRUTOR (Limpeza cirúrgica da memória)
// =========================================================================
Router::~Router() {
  delete routing_unit;
  delete arbiter;
  delete q_up;
  delete q_dn;
  delete crossbar;
  for(int i = 0; i < 8; i++) {
      delete input_channels[i];
      delete output_channels[i];
  }
}
