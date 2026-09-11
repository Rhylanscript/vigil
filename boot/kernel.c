static unsigned short* const VGA_MEMORY = (unsigned short*) 0xb8000;

void kernel_main(void) {
    const char* message = "VIGIL kernal online";
    unsigned char colour = 0x0f;

    for (int i = 0; message[i] != '\0'; i++) {
        VGA_MEMORY[i] = (unsigned short) message[i] | ((unsigned short) colour << 8);
    }

    for (;;) {
        
    }
}