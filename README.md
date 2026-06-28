# JIMS-NoC

Simulação em **SystemC** de uma Rede em Chip (NoC) baseada em roteadores **RSPIN** em uma topologia **Fat-Tree** (árvore gorda) de dois níveis.

---

## O que foi implementado

A infraestrutura completa da **JIMS-NoC** foi totalmente desenvolvida, integrada e verificada por meio de testes funcionais. O sistema final consiste em **8 roteadores RSPIN** interconectados em uma topologia **Fat-Tree de 2 níveis** e **16 núcleos (Cores)** emissores e receptores de tráfego.

### Estrutura de Arquivos

```text
.
├── CMakeLists.txt              # Configuração do build C++/SystemC
├── LICENSE                     # Licença do projeto
├── README.md                   # Documentação principal
├── topologia.png               # Imagem da topologia da rede
└── src/
    ├── main.cpp                # Ponto de entrada da simulação de tráfego de rede
    └── router/                 # Código-fonte do Roteador RSPIN e Rede
        ├── flit.h              # Estrutura do Flit (18 bits)
        ├── fifo.h              # Fila FIFO parametrizável (Circular wr/rd)
        ├── shared_buffer.h     # Buffers centrais compartilhados (QUP e QDN)
        ├── input_channel.h/cpp # Gerenciamento do canal de entrada e FSM
        ├── output_channel.h/cpp# Gerenciamento do canal de saída e créditos
        ├── routing_unit.h/cpp  # Lógica de roteamento combinacional (adaptativo/determinístico)
        ├── arbiter.h/cpp       # Árbitro com suporte a comutação Wormhole
        ├── crossbar.h/cpp      # Matriz combinacional completa 10x10
        ├── router.h/cpp        # Instanciação e ligação do roteador RSPIN
        ├── core.h/cpp          # Módulo de núcleo gerador/receptor de pacotes
        └── noc.h/cpp           # Topologia completa da rede (8 roteadores, 16 cores)
```
---

## Como Rodar os Testes Unitários e Simulação

Para compilar e executar a suíte de testes unitários do roteador:

```bash
# Crie e acesse o diretório de compilação
mkdir -p build && cd build

# Configure com o CMake (certifique-se de que o caminho do SystemC em CMakeLists.txt está correto)
cmake ..

# Compile o projeto
make

# Execute os testes
./test_runner

# Execute a simulação
./router_sim
```

---

## Formato dos Logs de Simulação (`simulation_logs.txt`)

Durante a execução da simulação, todos os eventos relevantes de tráfego, roteamento e controle de fluxo são capturados e salvos no arquivo `simulation_logs.txt`. Os logs seguem padrões bem definidos para facilitar a depuração e análise:

### 1. Transmissão e Recepção nos Cores
- **Envio de Header:** `[Core X at TIME ns] Sending Header: [F:0b000, P:0xDEST_MSG_SIZE (Hdr dest:D msg:M size:S)] with target destination: D`
- **Envio de Body:** `[Core X at TIME ns] Sending Body (N/TOTAL): [F:0b001, P:0x1000]`
- **Envio de Tail:** `[Core X at TIME ns] Sending Tail: [F:0b010, P:0xdead]`
- **Recepção pelo Destino:** `[Core X at TIME ns] Received Flit: [F:0bFRAMING, P:0xPAYLOAD]`

### 2. Trânsito de Flits nos Roteadores (Canais de Entrada e Saída)
- **Entrada no Roteador:** `[SPIN_NoC.RouterName.InChannel_PORT at TIME ns] Flit Chegando: [F:0bFRAMING, P:0xPAYLOAD]`
- **Saída do Roteador:** `[SPIN_NoC.RouterName.OutChannel_PORT at TIME ns] Flit Saindo: [F:0bFRAMING, P:0xPAYLOAD]`
  *(Onde `PORT` indica as portas locais de descida `D0-D3` ou as portas de subida na árvore `U4-U7`).*

### 3. Gerenciamento de Congestionamento e Créditos
- **Retenção de Crédito:** `[Core X at TIME ns] (BUFFER CHEIO / BUSY) Retendo crédito para o roteador!`
- **Devolução de Crédito (Desgestionamento):** `[Core X at TIME ns] (DESGESTIONANDO) Devolvendo crédito pendente para o roteador! (Restantes: N)`

### 4. Operações nos Buffers Compartilhados (QUP e QDN)
- **Desvio / Armazenamento:** `[SPIN_NoC.RouterName.QDN_Buffer at TIME ns] ARMAZENANDO Flit no Buffer Compartilhado: [F:... ]`
- **Esvaziamento / Liberação:** `[SPIN_NoC.RouterName.QUP_Buffer at TIME ns] LIBERANDO Flit do Buffer Compartilhado: [F:... ]`

---

## Geração de Formas de Onda VCD (`noc_simulation.vcd`)

Além dos logs textuais, a simulação gera automaticamente o arquivo de formas de onda `noc_simulation.vcd`. Ele permite a inspeção gráfica do comportamento temporal de todos os sinais da rede (clocks, resets, dados e créditos dos 16 cores e 8 roteadores).

Para visualizar as formas de onda graficamente, utilize o **GTKWave**, ou outro visualizador compatível com VCD:

```bash
gtkwave noc_simulation.vcd
```

