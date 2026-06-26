# JIMS-NoC

Simulação em **SystemC** de uma Rede em Chip (NoC) baseada em roteadores **RSPIN** em uma topologia **Fat-Tree** (árvore gorda) de dois níveis.

---

## 🚀 Estado Atual do Projeto (100% Implementado)

A infraestrutura completa da **JIMS-NoC** foi totalmente desenvolvida, integrada e verificada por meio de testes funcionais. O sistema final consiste em **8 roteadores RSPIN** interconectados em uma topologia **Fat-Tree de 2 níveis** e **14 núcleos (Cores)** emissores e receptores de tráfego.

### 📁 Estrutura de Arquivos

```text
.
├── CMakeLists.txt              # Configuração do build C++/SystemC
├── LICENSE                     # Licença do projeto
├── README.md                   # Documentação principal
├── Org. Comp - Projeto 2.md    # Planejamento original do projeto
├── analise_estado_projeto.md   # Relatório detalhado do projeto e análise das correções
├── test_runner                 # Executável contendo a suíte de testes unitários
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
        └── noc.h/cpp           # Topologia completa da rede (8 roteadores, 14 cores)
```

### ✅ O que está implementado
- **Módulo de Rede (NoC)**: Conecta 8 roteadores (4 folhas no Level 0 e 4 raízes no Level 1) e 14 cores (Cores 0 a 13) por meio de uma topologia Fat-Tree bipartida completa.
- **Módulo de Core (PE)**: Emissor e receptor completo de tráfego com FSM para gerar pacotes (Header, Body, Tail) respeitando créditos locais, e devolução de créditos na recepção.
- **Roteamento Híbrido e buffers compartilhados**: Roteamento adaptativo upward (baseado em créditos) e determinístico downward. Buffers centralizados compartilhados `QUP`/`QDN` de profundidade 18 utilizando `SharedFlit` para evitar deadlocks de descida.
- **Correções Estruturais e de Protocolo**: Resolvidos problemas de sensibilidade de construtor no SystemC, binding incompleto de portas, bugs de limites na inicialização da RoutingUnit e o ciclo extra de transmissão fantasma no Árbitro.

---

## 📖 Relatório de Análise Completo

Para uma descrição técnica aprofundada da arquitetura do roteador, mapeamento matemático de portas da Fat-Tree e detalhes da implementação, leia a [Análise do Estado Atual do Projeto](file:///home/samuelmorais/ufrn/7_semestre/organizacao_computadores/JIMS-NoC/analise_estado_projeto.md).

---

## 💻 Como Rodar os Testes Unitários

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
```
*(Nota: O executável pré-compilado `./test_runner` na raiz do projeto também pode ser executado diretamente).*