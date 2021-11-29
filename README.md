# slox
An implementation of Lox (from crafting interpreters) using sum types. 
Some (most?) implementation details diverge from the book. The main role
of this project is to experiment with implementation techniques.

# Dependencies

```
pip3 install --user meson
sudo apt-get install libgtest-dev libfmt-dev libreadline-dev
```

# Build
```
meson setup build
cd build
meson compile
```

# Test
```
meson test
```