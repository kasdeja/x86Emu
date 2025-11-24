add_executable(x86Emu
    Bios.cpp
    Cpu.cpp
    Disasm.cpp
    Dos.cpp
    Keyboard.cpp
    Memory.cpp
    MemoryView.cpp
    Pic.cpp
    Pit.cpp
    SDLInterface.cpp
    Vga.cpp
    main.cpp
)

add_executable(vgaTest
    Memory.cpp
    Vga.cpp
    vgaTest.cpp
)

if(WIN32)
    target_link_libraries(x86Emu ${SDL2_LIBRARY})
    target_link_libraries(vgaTest ${SDL2_LIBRARY})
else()
    target_link_libraries(x86Emu SDL2 pthread)
    target_link_libraries(vgaTest SDL2)
endif()
