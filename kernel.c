#include <stdint.h>

// vga starting point

int startstringpoint = 0;

// user data which is typed in shell

char inputdata[50];
char filedata[500];
struct FileEntry
{
    char name[12];
    uint32_t start_lba;
    uint32_t is_used;
};

#define MAX_FILES 256
#define DIRECTORY_SECTORS 10
#define START_DIR_LBA 5

struct FileEntry directory_table[MAX_FILES];

// Load all 10 sectors into memory

unsigned char keyboard_map[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', /* Backspace */
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',  /* Enter key */
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,       /* Left shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,               /* Right shift */
    '*', 0, ' '                                                              /* Space bar */
};

int strcompare(char *str1, char *str2)
{
    int i = 0;
    while (str1[i] != '\0' || str2[i] != '\0')
    {
        if (str1[i] != str2[i])
        {
            return 1;
        }
        i++;
    }
    return 0;
}

void cleanscreen(char *video_memory, char attribute)
{
    for (int i = 0; i < 80 * 25 * 2; i += 2)
    {
        video_memory[i] = ' ';
        video_memory[i + 1] = attribute;
    }
}

void print(char *message, char attribute, char *video_memory, void *value)
{
    int i = 0;

    if (startstringpoint >= 4000)
    {
        for (int idx = 160; idx < 80 * 25 * 2; idx++)
        {
            video_memory[idx - 160] = video_memory[idx];
        }
        for (int idx = 3840; idx < 80 * 25 * 2; idx += 2)
        {
            video_memory[idx] = ' ';
            video_memory[idx + 1] = attribute;
        }
        startstringpoint = 3840;
    }

    while (message[i] != '\0')
    {
        if (message[i] == '\b')
        {
            if (startstringpoint > 0)
            {
                startstringpoint -= 2;
                video_memory[startstringpoint] = ' ';
                video_memory[startstringpoint + 1] = attribute;
            }
            i++;
            continue;
        }

        if (message[i] == '%' && message[i + 1] == 'd')
        {
            int arr[10] = {0};
            int a = *(int *)value;

            if (a == 0)
            {
                video_memory[startstringpoint] = '0';
                video_memory[startstringpoint + 1] = attribute;
                startstringpoint += 2;
            }
            else
            {
                int index = 9;
                while (a != 0)
                {
                    arr[index] = (a % 10) + '0';
                    index--;
                    a /= 10;
                }
                for (int start = index + 1; start < 10; start++)
                {
                    video_memory[startstringpoint] = arr[start];
                    video_memory[startstringpoint + 1] = attribute;
                    startstringpoint += 2;
                }
            }
            i += 2;
            continue;
        }

        if (message[i] == '\n')
        {
            int current_row_bytes = startstringpoint % 160;
            startstringpoint += (160 - current_row_bytes);
            i++;
            continue;
        }

        video_memory[startstringpoint] = message[i];
        video_memory[startstringpoint + 1] = attribute;
        startstringpoint += 2;
        i++;
    }
}

unsigned char inputbyte(unsigned short port)
{

    // basically this is a blolier plate it get to the specific port and ge the required informatoion from it and we
    // return it as a result
    unsigned char result;
    __asm__("inb %%dx, %%al" : "=a"(result) : "d"(port));
    return result;
}

char getKeyPress()
{
    while ((inputbyte(0x64) & 1) == 0)
    {
    }
    unsigned char scancode = inputbyte(0x60);

    if (scancode & 0x80)
    {
        return 0;
    }
    return keyboard_map[scancode];
}

void outb(unsigned short port, unsigned char data)
{
    __asm__ volatile("outb %0,%1" : : "a"(data), "Nd"(port));
}

void outw(unsigned short port, unsigned short data)
{
    __asm__ volatile("outw %0, %1" : : "a"(data), "Nd"(port));
}

void read_disk(uint32_t lba, uint8_t *buffer)
{
    // Wait until the drive is not Busy (BSY bit 7 must be 0) before sending ports
    while (inputbyte(0x1F7) & 0x80)
        ;

    outb(0x1F2, 1);                    // Sector count
    outb(0x1F3, (uint8_t)lba);         // LBA Low
    outb(0x1F4, (uint8_t)(lba >> 8));  // LBA Mid
    outb(0x1F5, (uint8_t)(lba >> 16)); // LBA High
                                       // outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); // Drive select
    outb(0x1F6, 0xA0);
    outb(0x1F7, 0x20); // Send READ command

    uint32_t timeout2 = 0;
    while ((inputbyte(0x1F7) & 0x08) == 0)
    {
        timeout2++;
        if (timeout2 >= 500000)
        {
            print("can't read file\n", 0x0F, (char *)0xB8000, 0);
            return;
        }
    }

    for (int i = 0; i < 256; i++)
    {
        uint16_t data = 0;
        __asm__ volatile("inw %%dx, %%ax" : "=a"(data) : "d"(0x1F0));
        buffer[i * 2] = (uint8_t)data;
        buffer[i * 2 + 1] = (uint8_t)(data >> 8);
    }
}

void write_disk_sector(uint32_t lba, uint8_t *buffer)
{
    // Wait until the drive is not Busy (BSY bit 7 must be 0) before sending ports
    while (inputbyte(0x1F7) & 0x80)
        ;

    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    // outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F6, 0xA0);
    outb(0x1F7, 0x30); // Send WRITE command

    uint32_t timeout = 0;
    while ((inputbyte(0x1F7) & 0x08) == 0)
    {
        timeout++;
        if (timeout >= 500000)
        {
            print("can't make file\n", 0x0F, (char *)0xB8000, 0);
            return;
        }
    }

    for (int i = 0; i < 256; i++)
    {
        uint16_t data = buffer[i * 2] | (buffer[i * 2 + 1] << 8);
        __asm__ volatile("outw %%ax, %%dx" : : "a"(data), "d"(0x1F0));
    }

    // Explicitly wait for the drive to flush its cache write changes
    while (inputbyte(0x1F7) & 0x80)
        ;
}

void load_directory()
{
    uint8_t sector_buffer[512];
    uint8_t *table_ptr = (uint8_t *)directory_table;

    for (int i = 0; i < DIRECTORY_SECTORS; i++)
    {
        read_disk(START_DIR_LBA + i, sector_buffer);

        // Safely copy sector by sector into RAM storage
        for (int j = 0; j < 512; j++)
        {
            table_ptr[(i * 512) + j] = sector_buffer[j];
        }
    }
}

void save_directory()
{
    uint8_t sector_buffer[512];
    uint8_t *table_ptr = (uint8_t *)directory_table;

    for (int i = 0; i < DIRECTORY_SECTORS; i++)
    {
        // Build isolated temporary frame buffer
        for (int j = 0; j < 512; j++)
        {
            sector_buffer[j] = table_ptr[(i * 512) + j];
        }

        write_disk_sector(START_DIR_LBA + i, sector_buffer);
    }
}

void format_disk_directory()
{
    // 1. Wipe out our array in RAM completely
    for (int i = 0; i < MAX_FILES; i++)
    {
        directory_table[i].is_used = 0;
        directory_table[i].start_lba = 0;
        for (int j = 0; j < 12; j++)
        {
            directory_table[i].name[j] = '\0';
        }
    }
    save_directory();
    // 2. Blast these clean zeros into disk sectors 5 through 14
}

void creatediskfile(char *name, char *content)
{

    load_directory();

    for (int i = 0; i < MAX_FILES; i++)
    {
        if (directory_table[i].is_used == 0)
        {

            directory_table[i].start_lba = 15 + i;
            directory_table[i].is_used = 1;

            int j = 0;
            while (name[j] != '\0' && j < 11)
            {
                directory_table[i].name[j] = name[j];
                j++;
            }
            directory_table[i].name[j] = '\0';

            uint8_t buffer[512] = {0};
            j = 0;
            while (content[j] != '\0' && j < 511)
            {
                buffer[j] = content[j];
                j++;
            }

            write_disk_sector(directory_table[i].start_lba, buffer);

            save_directory();
            return;
        }
    }
}

void readfiles(char *name, char *video_memory)
{

    load_directory();
    for (int i = 0; i < MAX_FILES; i++)
    {

        if (directory_table[i].is_used == 1 && (strcompare(directory_table[i].name, name) == 0))
        {

            uint8_t buffer[513] = {0};
            buffer[512] = '\0';
            read_disk(directory_table[i].start_lba, buffer);
            print("starting to manage file ...\n", 0x0F, video_memory, 0);
            print((char *)buffer, 0x0F, video_memory, 0);
            print("\n", 0x0F, video_memory, 0);
            return;
        }
    }
    print("File not found on mega table!\n", 0x0F, video_memory, 0);
}

void initInputData(char *video_memory)
{
    int i = 0;

    while (1)
    {
        char c = getKeyPress();

        if (c != 0)
        {
            if (c == '\b')
            {
                if (i > 0)
                {
                    i--;
                    inputdata[i] = '\0';
                    char str[2] = {'\b', '\0'};
                    print(str, 0x0F, video_memory, 0);
                }
                continue;
            }

            if (c == '\n')
            {
                inputdata[i] = '\0';

                char newline_str[2] = {'\n', '\0'};
                print(newline_str, 0x0F, video_memory, 0);

                if (i > 0)
                {
                    if (strcompare("clear", inputdata) == 0)
                    {
                        cleanscreen(video_memory, 0x00);
                        startstringpoint = 0;
                    }
                    else if (inputdata[0] == 'd' && inputdata[1] == 'i' && inputdata[2] == 's' &&
                             inputdata[3] == 'p' && inputdata[4] == 'l' && inputdata[5] == 'a' &&
                             inputdata[6] == 'y')
                    {
                        int idx = 7;
                        int inside_quotes = 0;

                        while (inputdata[idx] != '\0')
                        {

                            if (inputdata[idx] == '\'')
                            {
                                if (inside_quotes == 1)
                                {
                                    break;
                                }
                                else
                                {
                                    inside_quotes = 1;
                                }
                            }
                            else if (inside_quotes == 1)
                            {
                                char single_char_str[2] = {inputdata[idx], '\0'};
                                print(single_char_str, 0x0F, video_memory, 0);
                            }
                            idx++;
                        }

                        char clear_line[2] = {'\n', '\0'};
                        print(clear_line, 0x0F, video_memory, 0);
                    }

                    else if (inputdata[0] == 'm' && inputdata[1] == 'a' && inputdata[2] == 'k' &&
                             inputdata[3] == 'e' && inputdata[4] == 'f' && inputdata[5] == 'i' &&
                             inputdata[6] == 'l' && inputdata[7] == 'e')
                    {

                        if (inputdata[8] == '\0')
                        {
                            print("Error: Please provide a filename! Example: makefile hi.txt\n", 0x0F, video_memory, 0);
                        }
                        else
                        {
                            char mybuffer[500];
                            char *filename = &inputdata[9];

                            print("write what you want max 200 character\n", 0x0F, video_memory, 0);
                            int idx = 0;
                            while (1)
                            {
                                char a = getKeyPress();

                                if (a != 0)
                                {
                                    // 1. Handle Backspace inside file writer
                                    if (a == '\b')
                                    {
                                        if (idx > 0)
                                        {
                                            idx--;
                                            mybuffer[idx] = '\0';
                                            char back_str[2] = {'\b', '\0'};
                                            print(back_str, 0x0F, video_memory, 0);
                                        }
                                        continue;
                                    }

                                    // 2. Handle Enter (Stop typing, save, and exit)
                                    if (a == '\n')
                                    {
                                        mybuffer[idx] = '\0'; // Properly null-terminate the file string!
                                        char newline_str[2] = {'\n', '\0'};
                                        print(newline_str, 0x0F, video_memory, 0);

                                        // Save to disk and print confirmation back to the screen
                                        creatediskfile(filename, mybuffer);
                                        // readfiles(filename, video_memory);
                                        break; // Crucial: break out of the loop to return to main shell!
                                    }

                                    // 3. Handle Regular Characters
                                    if (idx < 499)
                                    {
                                        mybuffer[idx] = a;
                                        char echo_str[2] = {a, '\0'};
                                        print(echo_str, 0x0F, video_memory, 0);
                                        idx++;
                                    }
                                }
                            }
                        }
                    }

                    else if (inputdata[0] == 'r' && inputdata[1] == 'e' && inputdata[2] == 'a' &&
                             inputdata[3] == 'd')
                    {

                        if (inputdata[4] == '\0')
                        {
                            print("Error: Please provide a filename! Example: makefile hi.txt\n", 0x0F, video_memory, 0);
                        }
                        char *filename = &inputdata[5];
                        readfiles(filename, video_memory);
                    }
                    else if (inputdata[0] == 'l' && inputdata[1] == 'i' && inputdata[2] == 's' &&
                             inputdata[3] == 't')
                    {

                        for (int i = 0; i < MAX_FILES; i++)
                        {
                            print(directory_table[i].name, 0x0F, video_memory, 0);
                            print(" ", 0x0F, video_memory, 0);
                        }
                        print("\n", 0x0F, video_memory, 0);
                    }
                    else if (inputdata[0] == 'd' && inputdata[1] == 'e' && inputdata[2] == 'l')
                    {
                        format_disk_directory();
                    }
                    else if (inputdata[0] == 'h' && inputdata[1] == 'e' && inputdata[2] == 'l' && inputdata[3] == 'p')
                    {

                        print(" command 1: clear \n command 2: display [string value] \n command 3: makefile [filename] \n command 4: read [filename] \n command 5: del---> remove all files \n command 6: list--> all files \n command 7: loop [value] string", 0x0F, video_memory, 0);
                        print("\n", 0x0F, video_memory, 0);
                    }
                    else if (inputdata[0] == 'l' && inputdata[1] == 'o' && inputdata[2] == 'o' && inputdata[3] == 'p')
                    {

                        int idx = 4;

                        while (inputdata[idx] == ' ')
                        {
                            idx++;
                        }

                        int cycles = 0;

                        while (inputdata[idx] >= '0' && inputdata[idx] <= '9')
                        {
                            cycles = cycles * 10 + (inputdata[idx] - '0');
                            idx++;
                        }

                        while (inputdata[idx] == ' ')
                        {
                            idx++;
                        }

                        char *data = &inputdata[idx];

                        if (cycles > 0 && cycles <= 1999 && *data != '\0')
                        {
                            for (int i = 0; i < cycles; i++)
                            {
                                print("%d ", 0x0F, video_memory, &i);

                                print(data, 0x0F, video_memory, 0);
                                print("\n", 0x0F, video_memory, 0);
                            }
                        }
                        else
                        {
                            print("Error: Loop count must be 1-1999 and contain text.\n",
                                  0x0F,
                                  video_memory,
                                  0);
                        }
                    }

                    else
                    {
                        print("Invalid command !\n", 0x0F, video_memory, 0);
                    }
                }

                for (int j = 0; j < 50; j++)
                {
                    inputdata[j] = 0;
                }
                i = 0;

                print("> ", 0x0F, video_memory, 0);
                continue;
            }

            if (i < 49)
            {
                inputdata[i] = c;
                char str[2] = {c, '\0'};
                print(str, 0x0F, video_memory, 0);
                i++;
            }
        }
    }
}

void kernel_main()
{
    char *video_memory = (char *)0xB8000;

    cleanscreen(video_memory, 0x00);
    print("WELCOME TO SKOS \n", 0x0F, video_memory, 0);

    // 1. Load directory from disk right away on bootup
    load_directory();

    // 2. If the disk is uninitialized (garbage values), format it once cleanly.
    if (directory_table[0].is_used != 0 && directory_table[0].is_used != 1)
    {
        print("First boot or corrupted disk! Formatting file allocation table...\n", 0x0F, video_memory, 0);
        format_disk_directory();
    }
    else
    {
        print("Disk file system structure verified successfully.\n", 0x0F, video_memory, 0);
    }

    print("> ", 0x0F, video_memory, 0);
    initInputData(video_memory);
}