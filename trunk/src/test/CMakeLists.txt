# $Id$

find_package(SDL_net)

include_directories( ../api )

if ( SDL_net )
    add_executable( sdl_server sdl_server.cpp )

    target_link_libraries( sdl_server ${SDLNET_LIBRARY} )

    add_executable( ana_client ana_client.cpp )

    target_link_libraries( ana_client ana )
endif ( SDL_net )

add_executable( test1_server ana_test1_server.cpp )

target_link_libraries( test1_server ana )

add_executable( test1_client ana_test1_client.cpp )

target_link_libraries( test1_client ana )
