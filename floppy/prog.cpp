extern "C" void prog()
{
    const short color = 0x0F00;
    const char* hello = "Hello, this is prog!";
    unsigned short* vga = (unsigned short*)0xb8000;
 
    for (int i = 0; i<20;++i)
        vga[i+80] = color | hello[i];
}