project('slox', 'cpp',
        version : '0.0.1',
        default_options : ['cpp_std=c++20', 'warning_level=3'])

# Executables
fmt_dep = dependency('fmt')
sources = ['main.cpp', 'src/interpreter.cpp', 'src/lexer.cpp', 'src/utils.cpp']
executable('sloxi', sources, dependencies: fmt_dep)

# Tests
gtest_dep = dependency('gtest')
tests = executable('unittest', 'test/main.cpp',
                   dependencies: gtest_dep)
test('unittests', tests)