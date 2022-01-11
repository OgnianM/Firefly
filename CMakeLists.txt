cmake_minimum_required(VERSION 3.19)
project(Firefly)

set(CMAKE_CXX_STANDARD 20)

file(GLOB SENJO_SOURCES  "external/SenjoUCIAdapter/senjo/*.cpp" "external/SenjoUCIAdapter/senjo/*.h")
file(GLOB FATHOM_SOURCES "external/Fathom/src/*.cpp" "external/Fathom/src/*.h")
file(GLOB LC0_UTILS
        external/LeelaUtils/weights_adapter.cpp external/LeelaUtils/network_legacy.cpp
        external/LeelaUtils/network_legacy.h
        external/LeelaUtils/encoder.cpp external/LeelaUtils/encoder.h
        external/LeelaUtils/net.pb.cc external/LeelaUtils/weights_adapter.h
        # external/LeelaUtils/loader.cc external/LeelaUtils/extracted_utils.h
        )


if (WITH_NNUE MATCHES TRUE)
    file(GLOB NNUE_PROBE external/nnue-probe/src/nnue.cpp external/nnue-probe/src/nnue.h
        external/nnue-probe/src/misc.cpp external/nnue-probe/src/misc.h)
endif()

if (COMPILE_TESTING_UTILS MATCHES TRUE)
    file(GLOB OPTIONAL src/testing/*)
    message("Compiling with:  ${OPTIONAL}")
    set(OPTIONAL_LIBS stdc++fs sfml-graphics sfml-window sfml-system)
endif()

file(GLOB EXTERNALS ${SENJO_SOURCES} ${FATHOM_SOURCES} ${LC0_UTILS} ${NNUE_PROBE})

add_executable(Firefly
        src/main.cpp
        src/pch.h
        src/config.h

        ${EXTERNALS}


        src/chess/constants/rook_attacks.cpp src/chess/constants/bishop_attacks.cpp
        src/chess/constants/constants.cpp src/chess/constants/constants.h

        src/chess/board.cpp src/chess/board.h

        src/utils/utils.cpp src/utils/utils.h

        #src/testing/timer.cpp src/testing/timer.h
        #src/testing/nnue_tests.cpp src/utils/fsts_queue.h
        #src/testing/neural_net_tests.cpp
        #src/testing/chess_gui.cpp src/testing/chess_gui.h src/testing/board_test.cpp src/testing/board_test.h
        ${OPTIONAL}


        src/engine/neural/lc0_network.cpp src/engine/neural/lc0_network.h src/engine/neural/network_manager.h

        src/engine/engine_interface.cpp src/engine/engine_interface.h

        src/engine/mcts/search.cpp src/engine/mcts/search.h



        src/engine/neural/utils/policy_map_to_flattened.h src/engine/neural/utils/inverted_policy_map.h
        src/engine/neural/utils/compressed_policy_map.h src/engine/neural/utils/uncompressed_policy_map.h

        src/engine/mcts/node.h src/engine/mcts/node.cpp

        src/engine/mcts/memory.cpp src/engine/mcts/memory.h src/utils/logger.cpp src/utils/logger.h)

target_include_directories(Firefly PUBLIC src/ ./ external/cxxopts/include)

target_precompile_headers(Firefly PUBLIC src/pch.h)


if (CMAKE_BUILD_TYPE MATCHES Debug)
    message(" ")
    set(CMAKE_CXX_FLAGS "-ggdb -march=native")
    message("CMAKE IN DEBUG MODE")
    message(" ")
#elseif(CMAKE_BUILD_TYPE MATCHES Release)
else()
    message(" ")
    set(CMAKE_CXX_FLAGS "-Ofast -march=native")
    message("CMAKE IN RELEASE MODE")
    message(" ")
endif ()

find_package(Torch REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS} -Wcomment")

target_link_libraries(Firefly ${TORCH_LIBRARIES} ${OPTIONAL_LIBS} protobuf)
