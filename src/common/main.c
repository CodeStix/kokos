struct VideoChar
{
    char charater;
    unsigned char color;
};

volatile struct VideoChar *videoMemory = (struct VideoChar *)0xb8000;

unsigned char printColor = 0x0f;
unsigned int x = 0, y = 0, w = 80, h = 25;

// #define u8 unsigned char
// #define i8 char
// #define i16 short
// #define u16 unsigned short
// #define i32 int
// #define u32 unsigned int
// #define i64 long long
// #define u64 unsigned long long

#define COLOR unsigned char
#define COLOR_BLACK (COLOR)0x0
#define COLOR_BLUE (COLOR)0x1
#define COLOR_GREEN (COLOR)0x2
#define COLOR_CYAN (COLOR)0x3
#define COLOR_RED (COLOR)0x4
#define COLOR_PURPLE (COLOR)0x5
#define COLOR_BROWN (COLOR)0x6
#define COLOR_GRAY (COLOR)0x7
#define COLOR_DARK_GRAY (COLOR)0x8
#define COLOR_LIGHT_BLUE (COLOR)0x9
#define COLOR_LIGHT_GREEN (COLOR)0xA
#define COLOR_LIGHT_CYAN (COLOR)0xB
#define COLOR_LIGHT_RED (COLOR)0xC
#define COLOR_LIGHT_PURPLE (COLOR)0xD
#define COLOR_YELLOW (COLOR)0xE
#define COLOR_WHITE (COLOR)0xF

void set_console_color(COLOR foreground, COLOR background)
{
    printColor = foreground | (background << 4);
}

void print_char(char c)
{
    if (c == '\n')
    {
        y++;
        x = 0;
    }
    else
    {
        videoMemory[x + y * w].charater = c;
        videoMemory[x + y * w].color = printColor;
        if (++x >= w)
        {
            y++;
            x = 0;
        }
    }
}

void set_cursor(unsigned int newX, unsigned int newY)
{
    x = newX;
    y = newY;
}

void clear()
{
    for (int ix = 0; ix < w; ix++)
    {
        for (int iy = 0; iy < h; iy++)
        {
            videoMemory[ix + iy * w].charater = 0x0;
            videoMemory[ix + iy * w].color = 0x0;
        }
    }
}

void print(char *str)
{
    while (*str)
    {
        print_char(*str);
        str++;
    }
}

int kernel_main()
{
    clear();
    set_console_color(COLOR_WHITE, COLOR_BLACK);
    print("booting...");
}