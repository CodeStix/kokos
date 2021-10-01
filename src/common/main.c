

struct VideoChar
{
    char charater;
    unsigned char color;
};

volatile struct VideoChar *videoMemory = (struct VideoChar *)0xb8000;

int kernel_main()
{
    videoMemory[0].charater = 'A';
    videoMemory[0].color = 0x0f;
    videoMemory[1].charater = 'B';
    videoMemory[1].color = 0xf0;
}