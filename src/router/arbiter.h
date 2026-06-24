#include "arbiter.h"

void Arbiter::arbitration_process()
{
    if (rst.read())
    {
        for (int i = 0; i < 10; i++)
        {
            output_locked[i] = false;
            output_owner[i] = -1;

            connection_valid[i].write(false);
            sel_input[i].write(0);
        }

        for (int i = 0; i < 8; i++)
        {
            route_grant[i].write(false);
            read_en[i].write(false);
        }

        return;
    }

    // Limpa sinais
    for (int i = 0; i < 8; i++)
    {
        route_grant[i].write(false);
        read_en[i].write(false);
    }

    for (int i = 0; i < 10; i++)
    {
        connection_valid[i].write(false);
    }

    // Libera rotas terminadas
    for (int in = 0; in < 8; in++)
    {
        if (release_route[in].read())
        {
            for (int out = 0; out < 10; out++)
            {
                if (output_owner[out] == in)
                {
                    output_locked[out] = false;
                    output_owner[out] = -1;
                }
            }
        }
    }

    // Mantém conexões wormhole já estabelecidas
    for (int out = 0; out < 10; out++)
    {
        if (output_locked[out])
        {
            int owner = output_owner[out];

            route_grant[owner].write(true);
            read_en[owner].write(true);

            sel_input[out].write(owner);
            connection_valid[out].write(true);
        }
    }

    // Arbitração simples
    for (int out = 0; out < 10; out++)
    {
        if (output_locked[out])
            continue;

        int winner = -1;

        for (int in = 0; in < 8; in++)
        {
            if (req_valid[in].read() &&
                req_port[in].read() == out)
            {
                winner = in;
                break;
            }
        }

        if (winner != -1)
        {
            output_locked[out] = true;
            output_owner[out] = winner;

            route_grant[winner].write(true);
            read_en[winner].write(true);

            sel_input[out].write(winner);
            connection_valid[out].write(true);
        }
    }
}


// Falta implementar prioridades
// Integrar os buffers
// Falta o round-robin