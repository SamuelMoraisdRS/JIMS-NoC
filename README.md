# JIMS-NoC

Simulação em **SystemC** de uma Rede em Chip (NoC) baseada em roteadores **RSPIN** em uma topologia **Fat-Tree** (árvore gorda) de dois níveis.

---

## 🚀 Estado Atual do Projeto (100% Implementado)

A infraestrutura completa da **JIMS-NoC** foi totalmente desenvolvida, integrada e verificada por meio de testes funcionais. O sistema final consiste em **8 roteadores RSPIN** interconectados em uma topologia **Fat-Tree de 2 níveis** e **16 núcleos (Cores)** emissores e receptores de tráfego.

### 📁 Estrutura de Arquivos

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

## 💻 Como Rodar os Testes Unitários e Simulação

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
*(Nota: O executável pré-compilado `./test_runner` na raiz do projeto também pode ser executado diretamente).*
