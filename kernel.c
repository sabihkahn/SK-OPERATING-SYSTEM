int startstringpoint = 0;

void print(char *message, char attribute, char *video_memory)
{
    int i = 0;
    int vga_index = 0;

    while (message[i] != '\0')
    {
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

void kernel_main()
{

    char *video_memory = (char *)0xB8000;
    cleanscreen(video_memory, 0x00);
    int i = 0;
    while (i < 100)
    {
        print("hello bro ", 0x0F, video_memory);
        i++;
    }
    
    

  
}
