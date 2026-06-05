cmake_minimum_required(VERSION 3.14)
project(HexChess)

list(APPEND CMAKE_PREFIX_PATH "C:\\libtorch")

find_package(Torch REQUIRED)

set(CMAKE_CXX_STANDARD 20)

set(HEXCHESS_CORE_SOURCE_FILES
    src/game/GameStateConstants.h
    src/game/GameState.h src/game/GameState.cpp
    src/game/BoardAnalytics.h src/game/BoardAnalytics.cpp
    src/game/BitburnerBoardGen.h src/game/BitburnerBoardGen.cpp
    src/ai/GameStateData.h src/ai/GameStateData.cpp
    src/ai/Example.h src/ai/Example.cpp
    src/utils.h src/utils.cpp
)
add_library(hexchess_core STATIC ${HEXCHESS_CORE_SOURCE_FILES})

target_include_directories(hexchess_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(HEXCHESS_AI_SOURCE_FILES
    src/ai/Net.h
    src/ai/NeuralNetwork.h src/ai/NeuralNetwork.cpp
    src/ai/MCTSConstants.h
    src/ai/MCTS.h src/ai/MCTS.cpp
    src/ai/BasicMCTS.h src/ai/BasicMCTS.cpp
    src/ai/AdvancedMCTS.h src/ai/AdvancedMCTS.cpp
)
add_library(hexchess_ai STATIC ${HEXCHESS_AI_SOURCE_FILES})
target_link_libraries(hexchess_ai PUBLIC hexchess_core "${TORCH_LIBRARIES}")

set(CONSOLE_SOURCE_FILES
    src/console.cpp
)
add_executable(console ${CONSOLE_SOURCE_FILES})
target_sources(console PRIVATE ${HEXCHESS_CORE_SOURCE_FILES} ${HEXCHESS_AI_SOURCE_FILES})
target_link_libraries(console PRIVATE hexchess_core hexchess_ai)

set(READER_SOURCE_FILES
    src/reader.cpp
)
add_executable(reader ${READER_SOURCE_FILES})
target_sources(console PRIVATE ${HEXCHESS_CORE_SOURCE_FILES})
target_link_libraries(reader PRIVATE hexchess_core)
find_package(Boost REQUIRED)

set(MICROSERVICE_SOURCE_FILES
    src/boostUtils.h src/boostUtils.cpp
    src/Session.h src/Session.cpp
    src/Listener.h src/Listener.cpp
    src/microservice.cpp
)
add_executable(microservice ${MICROSERVICE_SOURCE_FILES})
target_include_directories(microservice PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(microservice ${Boost_LIBRARIES})
target_sources(microservice PRIVATE ${HEXCHESS_CORE_SOURCE_FILES} ${HEXCHESS_AI_SOURCE_FILES})
target_link_libraries(microservice PRIVATE hexchess_core hexchess_ai)

set(TRAINER_SOURCE_FILES
    src/trainer.cpp
)
add_executable(trainer ${TRAINER_SOURCE_FILES})
target_sources(trainer PRIVATE ${HEXCHESS_CORE_SOURCE_FILES} ${HEXCHESS_AI_SOURCE_FILES})
target_link_libraries(trainer PRIVATE hexchess_core hexchess_ai ${Boost_LIBRARIES})
target_include_directories(trainer PRIVATE ${Boost_INCLUDE_DIRS})

file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
add_custom_command(TARGET console POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TORCH_DLLS} $<TARGET_FILE_DIR:console>)
add_custom_command(TARGET microservice POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TORCH_DLLS} $<TARGET_FILE_DIR:microservice>)
add_custom_command(TARGET trainer POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TORCH_DLLS} $<TARGET_FILE_DIR:trainer>)

file(GLOB OTHER_DLLS "dlls/*.dll")
add_custom_command(TARGET console POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OTHER_DLLS} $<TARGET_FILE_DIR:console>)
add_custom_command(TARGET microservice POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OTHER_DLLS} $<TARGET_FILE_DIR:microservice>)
add_custom_command(TARGET trainer POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OTHER_DLLS} $<TARGET_FILE_DIR:trainer>)