project('slox', 'cpp',
        version : '0.0.1',
        default_options : ['cpp_std=c++20', 'warning_level=3'])

# Dependencies
fmt_dep = dependency('fmt')
readline_dep = dependency('readline')

# Libraries
slox_static_sources = ['src/interpreter.cpp', 'src/lexer.cpp', 'src/parser.cpp',
                       'src/ast.cpp', 'src/utils.cpp']
slox_static_lib = static_library('libslox', slox_static_sources,
                                 dependencies: [fmt_dep, readline_dep])

# Executables
interpreter_sources = ['main.cpp']
executable('sloxi', interpreter_sources,
           link_with: slox_static_lib,
           dependencies: fmt_dep)

# Tests + test dependencies.
gtest_dep = dependency('gtest')
unittest_sources = ['test/main.cpp', 'test/lexer.cpp']
tests = executable('unittest', unittest_sources,
                   d_unittest: true,
                   install: false,
                   link_with: slox_static_lib,
                   dependencies: gtest_dep)
test('unittests', tests)