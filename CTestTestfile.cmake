# CMake generated Testfile for 
# Source directory: /home/lxw/文档/range scan v2注释
# Build directory: /home/lxw/文档/range scan v2注释
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(leveldb_tests "/home/lxw/文档/range scan v2注释/leveldb_tests")
set_tests_properties(leveldb_tests PROPERTIES  _BACKTRACE_TRIPLES "/home/lxw/文档/range scan v2注释/CMakeLists.txt;367;add_test;/home/lxw/文档/range scan v2注释/CMakeLists.txt;0;")
add_test(c_test "/home/lxw/文档/range scan v2注释/c_test")
set_tests_properties(c_test PROPERTIES  _BACKTRACE_TRIPLES "/home/lxw/文档/range scan v2注释/CMakeLists.txt;393;add_test;/home/lxw/文档/range scan v2注释/CMakeLists.txt;396;leveldb_test;/home/lxw/文档/range scan v2注释/CMakeLists.txt;0;")
add_test(env_posix_test "/home/lxw/文档/range scan v2注释/env_posix_test")
set_tests_properties(env_posix_test PROPERTIES  _BACKTRACE_TRIPLES "/home/lxw/文档/range scan v2注释/CMakeLists.txt;393;add_test;/home/lxw/文档/range scan v2注释/CMakeLists.txt;404;leveldb_test;/home/lxw/文档/range scan v2注释/CMakeLists.txt;0;")
subdirs("third_party/googletest")
subdirs("third_party/benchmark")
