add_library(x86Emu_Common OBJECT
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
)

add_executable(x86Emu main.cpp)
add_executable(x86Emu_FreeDos main_freedos.cpp)
add_executable(vgaTest vgaTest.cpp)

if(WIN32)
    target_link_libraries(x86Emu x86Emu_Common ${SDL2_LIBRARY})
    target_link_libraries(x86Emu_FreeDos x86Emu_Common ${SDL2_LIBRARY})
    target_link_libraries(vgaTest x86Emu_Common ${SDL2_LIBRARY})
else()
    target_link_libraries(x86Emu x86Emu_Common SDL2 pthread)
    target_link_libraries(x86Emu_FreeDos x86Emu_Common SDL2 pthread)
    target_link_libraries(vgaTest x86Emu_Common SDL2)
endif()
