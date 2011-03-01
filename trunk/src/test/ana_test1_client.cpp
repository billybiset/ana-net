/* $Id$ */

/**
 * @file
 * @brief Client side test application. Example for the ana project.
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

const char* const DEFAULT_ADDRESS = "127.0.0.1";


void show_help(char* const command)
{
    std::cout << "The following options should be given in order and are all optional:\n"
        "\tAddress:         [optional]    Set address. Default=" << DEFAULT_ADDRESS << std::endl <<
        "\tPort:            [optional]    Set port. Default=" << DEFAULT_PORT << std::endl <<
        "\tProxy address:   [optional]    Set proxy address." << std::endl <<
        "\tProxy port:      [optional]    Set proxy port." << std::endl <<
        "\tProxy user:      [optional]    Set proxy authentication user name." << std::endl <<
        "\tProxy password:  [optional]    Set proxy authentication password." << std::endl <<
        "Examples:\n" <<
        "\t" << command << " localhost 12345\n" <<
        "\t\tConnect to a local server on port 12345.\n" <<
        "\t" << command << " X.K.C.D 12345 localhost 3128 foo bar\n" <<
        "\t\tConnect to a remote server at X.K.C.D:12345 with proxy credentials foo:bar.\n\n";
        ;
}

struct connection_info
{
    connection_info() :
        pt(DEFAULT_PORT),
        address(DEFAULT_ADDRESS),
        proxyaddr(),
        proxyport(),
        user(),
        password()
    {
    }

    bool use_proxy() const
    {
        return proxyaddr != "" && proxyport != "";
    }

    port        pt;
    std::string address;
    std::string proxyaddr;
    std::string proxyport;
    std::string user;
    std::string password;
};

class ChatClient : public ana::listener_handler,
                   public ana::send_handler,
                   public ana::connection_handler
{
    public:
        ChatClient(connection_info ci) :
            conn_info_( ci ),
            client_( ana::client::create(ci.address, ci.pt) )
        {
        }

        void run()
        {
            try
            {
                if ( ! conn_info_.use_proxy() )
                    client_->connect( this );
                else
                    client_->connect_through_proxy(conn_info_.proxyaddr,
                                                   conn_info_.proxyport,
                                                   this,
                                                   conn_info_.user,
                                                   conn_info_.password);

                client_->set_listener_handler( this );
                client_->run();

                std::stringstream ss;
                for ( size_t i = 0; i < 10000; ++i)
                {
                    char c = '0' + (i % 10);
                    ss << c;
                }

                for ( size_t i = 0; i < 10000; ++i)
                {
                    client_->send( ana::buffer( ss.str() ), this);
                }
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << std::endl;
            }

            delete client_;
        }

    private:

        virtual void handle_connect( ana::error_code error, net_id server_id )
        {
            if ( error )
                std::cerr << "\nError connecting." << std::endl;
        }

        virtual void handle_disconnect( ana::error_code error, net_id server_id)
        {
            std::cerr << "\nServer disconnected. Enter to exit." << std::endl;
        }

        virtual void handle_receive( ana::error_code error, net_id, ana::read_buffer msg)
        {
        }

        virtual void handle_send( ana::error_code error, net_id client, ana::operation_id op_id)
        {
            if ( error )
                std::cout << "Error. Timeout?" << std::endl;
        }

        connection_info     conn_info_;
        ana::client*        client_;
};

int main(int argc, char **argv)
{
    show_help(argv[0]);

    connection_info ci;

    if ( argc > 1 )
        ci.address   = argv[1];
    if ( argc > 2 )
        ci.pt        = argv[2];
    if ( argc > 3 )
        ci.proxyaddr = argv[3];
    if ( argc > 4 )
        ci.proxyport = argv[4];
    if ( argc > 5 )
        ci.user      = argv[5];
    if ( argc > 6 )
        ci.password  = argv[6];

    std::cout << "Main client app.: Starting client" << std::endl;

    ChatClient client(ci);
    client.run();
}
