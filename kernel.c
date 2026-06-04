
int startstringpoint = 0;

unsigned char keyboard_map[128] =
    {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', /* Backspace */
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',  /* Enter key */
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,       /* Left shift */
        '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,               /* Right shift */
        '*', 0, ' '                                                              /* Space bar */
};

void print(char *message, char attribute, char *video_memory, void *value)
{
    int i = 0;
    int vga_index = 0;

    if (startstringpoint == 4000)
    {
        for (int i = 160; i < 80 * 25 * 2; i++)
        {
            video_memory[i - 160] = video_memory[i];
        }
        for (int i = 3840; i < 80 * 25 * 2; i += 2)
        {
            video_memory[i] = ' ';           // Set character to blank space
            video_memory[i + 1] = attribute; // Set it to your background color
        }
        startstringpoint = 3840;
    }

    while (message[i] != '\0')
    {
        

     

        if (message[i] == '%' && message[i + 1] == 'd')
        {
            int arr[10];

            for (int k = 0; k < 10; k++)
            {
                arr[k] = 0;
            }

            int a = *(int *)value;
            if (a == 0)
            {
                video_memory[startstringpoint] = '0';
                video_memory[startstringpoint + 1] = attribute;
                startstringpoint += 2;
            }

            int index = 9;

            while (a != 0)
            {
                int temp = 0;
                temp = a % 10;
                arr[index] = temp + '0';
                index = index - 1;
                a = a / 10;
            }
            int start = index + 1;
            while (start != 10)
            {

                video_memory[startstringpoint] = arr[start];
                video_memory[startstringpoint + 1] = attribute;
                startstringpoint += 2;
                start++;
            }
            // [0,0,0,0,1,2,3]

            i += 2;
        }

        if (message[i] == '\n')
        {
            int current_row_bytes = startstringpoint % 160;
            startstringpoint += (160 - current_row_bytes);
            break;
        }

        video_memory[startstringpoint] = message[i];
        video_memory[startstringpoint + 1] = attribute;
        startstringpoint += 2;
        i++;
    }
}

void cleanscreen(char *video_memory, char attribute)
{
    for (int i = 0; i < 80 * 25 * 2; i += 2)
    {
        video_memory[i] = ' ';
        video_memory[i + 1] = attribute;
    }
}

unsigned char inputbyte(unsigned short port)
{
    unsigned char result;

    __asm__("inb %%dx, %%al" : "=a"(result) : "d"(port));

    return result;
}

char getKeyPress()
{
    while ((inputbyte(0x64) & 1) == 0)
    {
        // wait unit the key is pressed
    }
    unsigned char scancode = inputbyte(0x60);

    if (scancode & 0x80)
    {
        return 0;
    }

    return keyboard_map[scancode];
}

void kernel_main()
{

    char *video_memory = (char *)0xB8000;
    cleanscreen(video_memory, 0x00);
    print("WELCOME TO SKOS \n ", 0x0F, video_memory, 0);

    while (1)
    {
        char c = getKeyPress();
        if (c != 0)
        {
            // Create a small string to pass to your print function
            char str[2] = {c, '\0'};
            print(str, 0x0F, video_memory, 0);
        }
    }
}
