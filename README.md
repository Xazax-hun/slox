# slox
An implementation of Lox (from crafting interpreters) using sum types

# Dependencies

```
pip3 install --user meson
sudo apt-get install libgtest-dev libfmt-dev
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