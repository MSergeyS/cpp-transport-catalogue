cmake_minimum_required(VERSION 3.10)

project(TransportCatalogue CXX)
set(CMAKE_CXX_STANDARD 17)

# Эта команда найдёт собранный нами пакет Protobuf.
# REQUIRED означает, что библиотека обязательна.
# Путь для поиска укажем в параметрах команды cmake.
find_package(Protobuf REQUIRED)
# Помимо Protobuf, понадобится библиотека Threads
find_package(Threads REQUIRED)


FILE (GLOB ALL_PROTO "*.proto" )
# Команда вызова protoc. 
# Ей переданы названия переменных, в которые будут сохранены 
# списки сгенерированных файлов, а также сам proto-файл.
# protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${ALL_PROTO})
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${ALL_PROTO})

#FILE (GLOB ALL_SOURCES "*.cpp")
#FILE (GLOB ALL_INCLUDES "*.h")

#SET (ALL_SRCS 
#	${ALL_SOURCES}
#	${ALL_INCLUDES}
#	${ALL_PROTO}
#)

set(TRANSPORT_CATALOGUE_FILES domain.cpp domain.h geo.cpp geo.h graph.h json_builder.cpp
    json_builder.h json.cpp json.h json_reader.cpp json_reader.h main.cpp map_renderer.cpp
    map_renderer.h ranges.h request_handler.cpp request_handler.h router.h svg.cpp svg.h
    transport_catalogue.cpp transport_catalogue.h transport_router.cpp transport_router.h
    serialization.cpp serialization.h
    transport_catalogue.proto)

add_executable(transport_catalogue ${PROTO_SRCS} ${PROTO_HDRS} ${TRANSPORT_CATALOGUE_FILES})

# добавляем цель - transport_catalogue
#add_executable(transport_catalogue ${PROTO_SRCS} ${PROTO_HDRS} ${ALL_SRCS})

# find_package определила переменную Protobuf_INCLUDE_DIRS,
# которую нужно использовать как include-путь.
# Также нужно добавить как include-путь директорию, куда
# protoc положит сгенерированные файлы.
target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(transport_catalogue PUBLIC ${ALL_SRCS})

# Также find_package определила Protobuf_LIBRARY.
# Protobuf зависит от библиотеки Threads. Добавим и её при компоновке.
target_link_libraries(transport_catalogue ${Protobuf_LIBRARY} Threads::Threads)

