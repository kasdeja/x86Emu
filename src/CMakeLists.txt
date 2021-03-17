add_executable(x86Emu
    Bios.cpp
    Cpu.cpp
    Disasm.cpp
    Dos.cpp
    Memory.cpp
    main.cpp
)

add_executable(vgaTest vgaTest.cpp)

target_link_libraries(vgaTest SDL2)
