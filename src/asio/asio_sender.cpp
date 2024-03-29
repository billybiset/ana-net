/* $Id$ */

/**
 * @file
 * @brief Implementation file providing send capability to ana.
 *
 * ana: Asynchronous Network API.
 * Copyright (C) 2010 Guillermo Biset.
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

#include <boost/bind.hpp>

#include "asio_sender.hpp"


asio_sender::asio_sender() :
    _sending( false ),
    _sender_mutex(),
    _send_queue()
{
}

asio_sender::~asio_sender()
{
    std::list<_send_operation*>::iterator it = _send_queue.begin();

    while ( it != _send_queue.end() )
    {
        delete *it;
    }
}

asio_sender::_send_operation::_send_operation( ana::detail::shared_buffer buffer ,
                                               tcp::socket&               socket ,
                                               ana::send_handler*         handler,
                                               ana::detail::sender*       sender ,
                                               ana::operation_id          op_id  ) :
   buffer( buffer ),
   socket( socket ),
   handler( handler ),
   sender( sender ),
   op_id( op_id )
{
}


void asio_sender::send(ana::detail::shared_buffer buffer ,
                       tcp::socket&               socket ,
                       ana::send_handler*         handler,
                       ana::detail::sender*       sender ,
                       ana::operation_id          op_id  )
{
    ana::timer* running_timer( NULL );
    try
    {
        boost::mutex::scoped_lock glock(_sender_mutex);

        if ( ! _sending )
        {
            _sending = true;

            if ( sender->timeouts_enabled() )
            {
                running_timer = sender->create_timer();

                sender->start_timer( running_timer, buffer,
                                    boost::bind(&asio_sender::handle_send, this,
                                                boost::asio::placeholders::error, handler,
                                                running_timer, op_id, true ) );
            }

            stats_collector().start_send_packet(  buffer->size()
                                                + ( raw_mode() ? 0 : ana::HEADER_LENGTH ) );

            if ( raw_mode() )
            {
                socket.async_write_some( boost::asio::buffer(buffer->base(), buffer->size() ),
                                         boost::bind(&asio_sender::handle_partial_send,this,
                                                     buffer, boost::asio::placeholders::error,
                                                     &socket, handler, running_timer, 0, _2, op_id ));
            }
            else
            {
                ana::ana_uint32 size( buffer->size() );
                ana::host_to_network_long( size );

                ana::serializer::bostream* output_stream = new ana::serializer::bostream();
                (*output_stream) << size;

                socket.async_write_some( boost::asio::buffer( output_stream->str() ),
                                        boost::bind(&asio_sender::handle_sent_header,this,
                                                    boost::asio::placeholders::error, output_stream,
                                                    &socket, buffer,
                                                    handler, running_timer, _2, op_id ));
            }
        }
        else
        {
            _send_operation* oper = new _send_operation(buffer,socket,handler,sender,op_id);

            _send_queue.push_back( oper );
        }
    }
    catch(std::exception& e)
    {
        disconnect();
        delete running_timer;
    }
}

void asio_sender::handle_sent_header(const ana::error_code&      ec,
                                     ana::serializer::bostream*  bos,
                                     tcp::socket*                socket,
                                     ana::detail::shared_buffer  buffer,
                                     ana::send_handler*          handler,
                                     ana::timer*                 running_timer,
                                     size_t                      bytes_sent,
                                     ana::operation_id           op_id)
{
    delete bos;

    if (bytes_sent != sizeof( ana::ana_uint32 ) )
        throw std::runtime_error("Couldn't send header.");

    if ( ec )
        handle_send(ec, handler, running_timer, op_id);
    else
    {
        socket->async_write_some( boost::asio::buffer(buffer->base(), buffer->size() ),
                                  boost::bind(&asio_sender::handle_partial_send,this,
                                              buffer, boost::asio::placeholders::error,
                                              socket, handler, running_timer, 0, _2, op_id ));
    }
}

void asio_sender::handle_partial_send( ana::detail::shared_buffer  buffer,
                                       const ana::error_code&      ec,
                                       tcp::socket*                socket,
                                       ana::send_handler*          handler,
                                       ana::timer*                 timer,
                                       size_t                      accumulated,
                                       size_t                      last_msg_size,
                                       ana::operation_id           op_id)
{
    try
    {
        if (ec)
            handle_send(ec, handler, timer, op_id);
        else
        {
            accumulated += last_msg_size;

            stats_collector().log_send( last_msg_size, accumulated == buffer->size() );

            if ( accumulated > buffer->size() )
                throw std::runtime_error("The send operation was too large.");

            if ( accumulated == buffer->size() )
                handle_send( ec, handler, timer, op_id );
            else
                socket->async_write_some(boost::asio::buffer(buffer->base_char() + accumulated,
                                                             buffer->size()      - accumulated),
                                         boost::bind(&asio_sender::handle_partial_send, this,
                                                     buffer, boost::asio::placeholders::error,
                                                     socket, handler, timer,
                                                     accumulated, _2, op_id ));
        }
    }
    catch(const std::exception& e)
    {
        disconnect( );
    }
}

void asio_sender::handle_send(const ana::error_code& ec,
                              ana::send_handler*     handler,
                              ana::timer*            running_timer,
                              ana::operation_id      op_id,
                              bool                   from_timeout)
{
    if ( ec != boost::asio::error::operation_aborted ) // equals only after cancellation
    {
        delete running_timer;

        if ( ec && from_timeout )
            handler->handle_send( ana::timeout_error , id(), op_id );
        else
            handler->handle_send( ec, id(), op_id );

        if ( ec )
            disconnect();
    }

    bool should_send_from_queue = false;

    {
        boost::mutex::scoped_lock glock( _sender_mutex );
        _sending = false;

        should_send_from_queue = ! ec && ! _send_queue.empty();
    }

    if ( should_send_from_queue )
    {
        _send_operation* oper = _send_queue.front();

        {
            boost::mutex::scoped_lock glock( _sender_mutex );
            _send_queue.pop_front();
        }

        send( oper->buffer, oper->socket, oper->handler, oper->sender, oper->op_id);
        delete oper;
    }
}

