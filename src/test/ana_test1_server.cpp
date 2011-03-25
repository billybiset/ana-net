/* $Id$ */

/**
 * @file
 * @brief Server side simple test application. Example for the ana project.
 *
 * ana: Asynchronous Network API.
 * Copyright (C) 2011 Guillermo Biset.
 *
 * This file is part of the ana project.
 *
 * System:         ana
 * Language:       C++
 *
 * Author:         Guillermo Biset
 * E-Mail:         billybiset AT gmail DOT com
 *
 * ana is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * ana is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ana.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <iostream>

#include "ana.hpp"

using namespace ana;

const port DEFAULT_PORT = "30303";

const size_t EXPECTED_SEND_OPERATIONS_PER_CLIENT = 2;

class ChatServer : public listener_handler,
                   public send_handler,
                   public connection_handler
{
    public:
        ChatServer() :
            server_( ana::server::create() ),
            completed_(100)
        {
        }

        void run(port pt)
        {
            server_->set_connection_handler( this );

            server_->set_listener_handler( this );

            server_->run(pt);

            std::cout << "Server running, Enter to quit." << std::endl;

            std::string s;
            std::getline(std::cin, s); //yes, i've seen better code :)
        }

        ~ChatServer()
        {
            delete server_;
        }

    private:

        virtual void handle_connect(ana::error_code error, net_id client)
        {
            if (! error)
            {
                std::cout << "Server: User " << client << " has joined from "
                   << server_->ip_address( client ) << "." << std::endl;

               completed_[client] = 0;
            }
        }

        virtual void handle_disconnect(ana::error_code error, net_id client)
        {
            std::cout << "Client " << client << " disconnected." << std::endl;
        }

        virtual void handle_send(ana::error_code error, net_id client, ana::operation_id id)
        {
            if ( error )
                std::cerr << "Error sending to client " << client << ". Timeout?" << std::endl;
        }

        virtual void handle_receive( ana::error_code          error,
                                     net_id                   client,
                                     ana::read_buffer         buffer)
        {
            if (! error)
            {
                bool errored = false;

                std::string msg = buffer->string();

                if ( msg.size() != 10000 )
                {
                    std::cout << "Client " << client << " ERRORED. Received buffer"
                                 " with wrong size: " << buffer->size() << "\n";
                }
                else
                {
                    for ( size_t i = 0; i < msg.size(); ++i )
                    {
                        char c = '0' + (i % 10);
                        if ( msg[i] != c )
                        {
                            std::cout << "ERROR: wrong message. Expecting: "
                            << c << " and got " << msg[i] << std::endl;

                            errored = true;
                            break;
                        }
                    }
                }

                if ( ! errored )
                {
                    ++completed_[client];

                    std::cout << "\nClient " << client << " PASSED operation number: " << completed_[client] << std::endl;
                }
            }
            else
                handle_disconnect(error, client);
        }

        server* server_;
        std::vector<size_t> completed_;
};

int main(int argc, char **argv)
{
    port pt(DEFAULT_PORT);

    if ( argc > 1 )
        pt = argv[1];

    std::cout << "Running on port " << pt << ".\n";

    ChatServer server;
    server.run(pt);
}
