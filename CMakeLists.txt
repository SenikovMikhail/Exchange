CMAKE_MINIMUM_REQUIRED(VERSION 3.21)
PROJECT(ClienServerEcn)

#set(libpqxx_ROOT /home/kasper/programming/libpqxx)
#set(PostgreSQL_ADDITIONAL_VERSIONS "14.5")
#find_package(libpqxx REQUIRED)


FIND_PACKAGE(Boost 1.40 COMPONENTS system REQUIRED)
FIND_PACKAGE(Threads REQUIRED)
find_package(PostgreSQL REQUIRED)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})




set(PQXX /home/kasper/programming/libpqxx/include/pqxx)
find_library(PQXX_LIB pqxx)
find_library(PQ_LIB pq)

include_directories (${PostgreSQL_INCLUDE_DIR} ${libpqxx_ROOT}/include)
link_directories(${PostgreSQL_LIBRARY_DIR} ${libpqxx_ROOT}/lib)

SET(CMAKE_CXX_STANDARD 17)

ADD_EXECUTABLE(Server Server.cpp Common.hpp User.hpp json.hpp)
target_include_directories(Server PRIVATE ${PostgreSQL_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(Server PRIVATE Threads::Threads ${Boost_LIBRARIES} ${PQXX_LIB} ${PQ_LIB})

ADD_EXECUTABLE(Client Client.cpp User.hpp Common.hpp json.hpp)
TARGET_LINK_LIBRARIES(Client PRIVATE Threads::Threads ${Boost_LIBRARIES} ) 