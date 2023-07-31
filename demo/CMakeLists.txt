find_package(OpenCV REQUIRED)
if(OpenCV_FOUND)
message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")
include_directories(${OpenCV_INCLUDE_DIRS})
link_libraries(${OpenCV_LIBS})
endif()

add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# set(CodePorting.Native.Cs2Cpp_DIR /home/vincent_xie/workspace/gitlab.intsig.net/aspose_package/third_party/Aspose.Words.Cpp_21.10/CodePorting.Native.Cs2Cpp_api_21.10)
# find_package(CodePorting.Native.Cs2Cpp)
# if(CodePorting.Native.Cs2Cpp_FOUND)
# message(STATUS "${CodePorting.Native.Cs2Cpp_INCLUDE_DIRS}")
# message(STATUS "${CodePorting.Native.Cs2Cpp_LIBS}")
# endif()


# set(Aspose.Words.Cpp_DIR /home/vincent_xie/workspace/gitlab.intsig.net/aspose_package/third_party/Aspose.Words.Cpp_21.10/Aspose.Words.Cpp)
# find_package(Aspose.Words.Cpp)
# if(Aspose.Words.Cpp_FOUND)
# message(STATUS "${Aspose.Words.Cpp_INCLUDE_DIRS}")
# message(STATUS "${Aspose.Words.Cpp_LIBS}")
# endif()

# 	# # find Aspose.Words for C++ package and it's dependencies
# 	# find_package(CodePorting.Native.Cs2Cpp REQUIRED CONFIG PATHS ${CMAKE_CURRENT_SOURCE_DIR} NO_DEFAULT_PATH)
# 	# find_package(Aspose.Words.Cpp REQUIRED CONFIG PATHS ${CMAKE_CURRENT_SOURCE_DIR} NO_DEFAULT_PATH)
# 	# find_package(Threads REQUIRED)

# 	# # Link target application with Aspose.Words for C++
# 	# target_link_libraries(app PRIVATE Aspose::Words Threads::Threads)
	



# include_directories(/home/vincent_xie/workspace/gitlab.intsig.net/aspose_package/third_party/Aspose.Words.Cpp_21.10/Aspose.Words.Cpp/include)
# include_directories(/home/vincent_xie/workspace/gitlab.intsig.net/aspose_package/third_party/Aspose.Words.Cpp_21.10/CodePorting.Native.Cs2Cpp_api_21.10/include)

# link_directories(/home/vincent/workspace/aspose_package/lib)
# link_directories(${CMAKE_SOURCE_DIR}/third_party/spdlog/lib)



link_directories(${CMAKE_BINARY_DIR})

#add_executable(demo main.cpp pdf2img.cpp md5.cpp base64.cpp word2img.cpp xie_filesystem.cpp)
#add_executable(demo main.cpp pdf2img.cpp md5.cpp base64.cpp xie_filesystem.cpp)
add_executable(demo main.cpp md5.cpp base64.cpp xie_filesystem.cpp)

target_link_libraries(demo contract_compare_static pthread)
#target_link_libraries(demo poppler-cpp)
target_link_libraries(demo yaml-cpp)
target_link_libraries(demo stdc++fs)
target_link_libraries(demo ssl crypto)
# target_link_libraries(demo spdlog)
#target_link_libraries(demo Aspose::Words)




