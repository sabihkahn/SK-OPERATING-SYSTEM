#include <stdint.h>

// vga starting point
int startstringpoint = 0;
// user data which is typed in shell
char inputdata[50];

#define MAX_FILES 256
#define directory_sectors 10
#define start_directory_lba 5

struct File_entry
{
    char name[12];
    uint32_t start_lba;
    uint32_t is_used;
};

struct File_entry DIRECTORY_TABLES[MAX_FILES];

// // before cotinue read this again it will hlp you understand
// This is the address of the 41st slot (Index 40).When you write arr + 40,
// the computer calculates: Start + (40 * size of element).If you want to put 324 at index 40,
// you would have to write: *(arr + 40) = 324;





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

    outb(0x1F2, 1); // in this port we are just sending one byte to the port 0xF2 and asking for only read one sector

    // now we will send the lba adress in 8bits because the size maximum bit we can send is 8 or 1 byte so we will
    // sift the 32 bit lba into 3 bit the total size two get one request to the port is 28 bit so (8+8+8) = 24
    // the remaning 4 will be send on 0x1F6 port this all will be added up and then we can successfully send our
    // 28bit lba adreess

    // examples ===>   suppose we send this lba adress 0x02345f2
    // first we will send f2 on (0x1F3)---> which we will say low
    // second we will send 45 on (0x1F4) ----> which will say medium
    // third we will send  23 on (0x1F5) ----> which will say high
    // last the 0 on (0x1F6) which is neutral and will be sended on 0x1f6 port easy

    // also >>, << these are right and left shift which we will use to manupulate value and send in low,medium,high bit

    outb(0x1F3, (uint8_t)lba);         // low 8 bit eg: f2
    outb(0x1F4, (uint8_t)(lba >> 8));  // medium 8 bit eg:45
    outb(0x1F5, (uint8_t)(lba >> 16)); // high 8 bit eg: 23
    outb(0x1F6, 0xE0 | ((lba >> 24) && 0x0F));

    outb(0x1F7, 0x20); // 0x20 is read command which will be sended to disk controller

    // wait for the drivers or we can say disk conroller to done work

    while ((inputbyte(0x1F7) && 0x08) == 0)
    {
        // when ever not equal to zero loop stops and now we can perform other actions
    }

    for (int i = 0; i < 256; i++)
    {
        uint16_t data = 0;

        // this assembly instruction is also a boiler template can be get from stack overflow
        // it takes value from port 0x1F0 store it into data varaible which is like this
        // in intel assembly
        // inw dx,ax   where ax is port and dx is variable or register

        __asm__ volatile("inw %%dx, %%ax" : "=a"(data) : "d"(0x1F0));

        // so in first iteration we will 0*2 = 0 so buffer[0] = value from last of orignal value
        // and then when we do ==>  0*2+1 we get 1 so buffer[1] = a 8 bit data which is in
        // first but moved to the last by me because uint8_t choose from last bro!!

        buffer[i * 2] = (uint8_t)data;
        buffer[i * 2 + 1] = (uint8_t)(data >> 8);
    }
}

void write_disk_sector(uint32_t lba, uint8_t *buffer)
{

    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));

    // Send the WRITE command (0x30) instead of read
    outb(0x1F7, 0x30);

    // Wait for the disk to be ready to accept data
    while ((inputbyte(0x1F7) & 0x08) == 0)
    {
    }

    // Push our 512 bytes from RAM into the disk

    for (int i = 0; i < 256; i++)
    {

        uint16_t data = buffer[i * 2] | (buffer[i * 2 + 1] << 8);
        // check readfuction you will notice that we are reading from ax and putting in dx

        __asm__ volatile("outw %%ax, %%dx" : : "a"(data), "d"(0x1F0));
    }
}

void kernel_main()
{
    char *video_memory = (char *)0xB8000;

    cleanscreen(video_memory, 0x00);
    print("WELCOME TO SKOS \n", 0x0F, video_memory, 0);
    print("> ", 0x0F, video_memory, 0);

    initInputData(video_memory);
}