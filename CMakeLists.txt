cmake_minimum_required(VERSION 3.12)
project(SPIN_Router_Simulation CXX)

# 1. Definir o padrão do C++ (SystemC moderno geralmente exige C++11 ou superior)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Configurar o caminho do SystemC 
# Substitua pelo caminho onde o SystemC está instalado na sua máquina ou na VPN da UFRN
set(SystemC_HOME "/usr/local/systemc" CACHE PATH "Path to SystemC installation")

# 3. Adicionar os cabeçalhos do SystemC e do seu próprio projeto
include_directories(
    ${SystemC_HOME}/include
    src/router
)

# 4. Adicionar o caminho para encontrar a biblioteca estática/dinâmica do SystemC
link_directories(${SystemC_HOME}/lib-linux64) # Mude para lib-linux se for 32-bit

# 5. Listar os arquivos fontes do seu projeto
set(SOURCES
    src/router/input_channel.cpp
    src/router/output_channel.cpp
    src/router/routing_unit.cpp
    src/router/crossbar.cpp
    src/router/arbiter.cpp
    src/router/router.cpp
    src/router/noc.cpp
    src/router/core.cpp
    src/main.cpp
)

# 6. Criar o executável da simulação
add_executable(router_sim ${SOURCES})

# 7. Linkar a biblioteca do SystemC e threads (exigido pelo SystemC no Linux)
find_package(Threads REQUIRED)
target_link_libraries(router_sim PRIVATE systemc Threads::Threads)