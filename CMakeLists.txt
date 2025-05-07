if(WIN32)
cmake_minimum_required (VERSION 3.30.0)
cmake_policy(SET CMP0167 OLD)  # FindBoost 관련 경고 억제
elseif(UNIX)
cmake_minimum_required (VERSION 3.28)
endif()

project ("rtmp_server")

set(CMAKE_CXX_STANDARD 20)

if(WIN32)
#  add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7 타겟

  # Boost
  set(Boost_USE_STATIC_LIBS ON) # 정적 라이브러리 사용 설정
  find_package(Boost 1.85.0 REQUIRED COMPONENTS system random)

  if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS} )
    link_directories(${BOOST_LIBRARYDIR})
  else()
    message(FATAL_ERROR "Could not find Boost")
  endif()
 
   # Hiredis
   set(HIREDIS_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/external/redis/include)
   set(HIREDIS_LIB_DIR ${CMAKE_SOURCE_DIR}/external/redis/lib)
 
   include_directories(${HIREDIS_INCLUDE_DIR})
   link_directories(${HIREDIS_LIB_DIR})
   set(HIREDIS_LIBRARIES ${HIREDIS_LIB_DIR}/hiredis.lib)
   
elseif(UNIX)
  # Hiredis 
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(HIREDIS REQUIRED hiredis)
  if(HIREDIS_FOUND)
    include_directories(${HIREDIS_INCLUDE_DIRS})
    link_directories(${HIREDIS_LIBRARY_DIRS})
    set(HIREDIS_LIBRARIES ${HIREDIS_LIBRARIES})
  else()
    message(FATAL_ERROR "Could not find hiredis")
  endif()
   
endif()

# Include sub-projects.
add_subdirectory ("src/common")
add_subdirectory ("src/media")
add_subdirectory ("src/network")
add_subdirectory ("src/rtmp_server")
