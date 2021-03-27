add_executable(x86Emu
    Bios.cpp
    Cpu.cpp
    Disasm.cpp
    Dos.cpp
    Memory.cpp
    SDLInterface.cpp
    Vga.cpp
    main.cpp
)

target_link_libraries(x86Emu SDL2 pthread)

add_executable(vgaTest
    Memory.cpp
    Vga.cpp
    vgaTest.cpp
)

target_link_libraries(vgaTest SDL2)
