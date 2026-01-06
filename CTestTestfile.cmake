# CMake generated Testfile for 
# Source directory: /Users/linxinwei/Downloads/range scan/RIOKV
# Build directory: /Users/linxinwei/Downloads/range scan/RIOKV
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(leveldb_tests "/Users/linxinwei/Downloads/range scan/RIOKV/leveldb_tests")
set_tests_properties(leveldb_tests PROPERTIES  _BACKTRACE_TRIPLES "/Users/linxinwei/Downloads/range scan/RIOKV/CMakeLists.txt;367;add_test;/Users/linxinwei/Downloads/range scan/RIOKV/CMakeLists.txt;0;")
add_test(c_test "/Users/linxinwei/Downloads/range scan/RIOKV/c_test")
set_tests_properties(c_test PROPERTIES  _BACKTRACE_TRIPLES "/Users/linxinwei/Downloads/range scan/RIOKV/CMakeLists.txt;393;add_test;/Users/linxinwei/Downloads/range scan/RIOKV/CMakeLists.txt;396;leveldb_test;/Users/linxinwei/Downloads/range scan/RIOKV/CMakeLists.txt;0;")
add_test(env_posix_test "/Users/linxinwei/Downloads/range scan/RIOKV/env_posix_test")
set_tests_properties(env_posix_test PROPERTIES  _BACKTRACE_TRIPLES "/Users/linxinwei/Downloads/range scan/RIOKV/CMakeLists.txt;393;add_test;/Users/linxinwei/Downloads/range scan/RIOKV/CMakeLists.txt;404;leveldb_test;/Users/linxinwei/Downloads/range scan/RIOKV/CMakeLists.txt;0;")
subdirs("third_party/googletest")
subdirs("third_party/benchmark")
