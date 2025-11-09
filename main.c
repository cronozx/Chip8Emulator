#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <time.h>
#include <string.h>

//Starting addresses
const unsigned int startingAddress = 0x200;
const unsigned int fontSetStartAddress = 0x50;

int SCREEN_HEIGHT = 32;
int SCREEN_WIDTH = 64;

uint8_t registers[16];
uint8_t memory[4096];
uint16_t idx;
uint16_t pc = 0;
uint16_t stack[16];
uint8_t stackPointer;
uint8_t delayTimer;
uint8_t soundTimer;
uint8_t keys[16];
uint32_t display[64 * 32];
uint16_t opcode;

typedef struct SDL_VARS {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
} SDL_VARS;

SDL_VARS sdlVars;

const unsigned int fontSetSize = 80;
uint8_t fontSet[80] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void initSDL(char const* title, int windowWidth, int windowHeight, int textureWidth, int textureHeight);
void loadROM(char const* fileName);
bool proccessInput();
void fdeLoop();
void updateDisplay(void const* buffer, int pitch);
void initTables();

//For the tables the way it works is for example table 0 you need to reserve 0xE + 1 so that the last memory indice EE is valid

//You write it like void (table[])(void) to specify that the type of the pointer is void and that its a array of pointers to functions and then you put (void) to specify that theres no params
void (*table[0xF + 1])(void);
void (*table0[0xE + 1])(void);
void (*table8[0xE + 1])(void);
void (*tableE[0xE + 1])(void);
void (*tableF[0x65 + 1])(void);

//Pointer functions (i hope thats what theyre actually called)
void Table0() {
    table0[opcode & 0x000F]();
}

void Table8() {
    table8[opcode & 0x000F]();
}

void TableE() {
    tableE[opcode & 0x000F]();
}

void TableF() {
    printf("PC: 0x%03X | Opcode: 0x%04X\n", pc, opcode);
    tableF[opcode & 0x00FF]();
}

void OP_00E0();
void OP_00EE();
void OP_1NNN();
void OP_2NNN();
void OP_3XKK();
void OP_4XKK();
void OP_5XY0();
void OP_6XKK();
void OP_7XKK();
void OP_8XY0();
void OP_8XY1();
void OP_8XY2();
void OP_8XY3();
void OP_8XY4();
void OP_8XY5();
void OP_8XY6();
void OP_8XY7();
void OP_8XYE();
void OP_9XY0();
void OP_ANNN();
void OP_BNNN();
void OP_CXKK();
void OP_DXYN();
void OP_EX9E();
void OP_EXA1();
void OP_FX07();
void OP_FX0A();
void OP_FX15();
void OP_FX18();
void OP_FX1E();
void OP_FX29();
void OP_FX33();
void OP_FX55();
void OP_FX65();
void OP_NULL();
void Table0();
void Table8();
void TableE();
void TableF();

int main(int argc, char** argv) {
    if (argc != 4) {
        printf("Usage %s <Scale> <Delay> <Rom>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));

    int videoScale = atoi(argv[1]);
    int delay = atoi(argv[2]);
    char const* romName = argv[3];

    initSDL("CHIP8 Emulator", SCREEN_WIDTH * videoScale, SCREEN_HEIGHT * videoScale, SCREEN_WIDTH, SCREEN_HEIGHT);
    printf("SDL init finished \n");

    initTables();

    pc = startingAddress;

    for (int i = 0; i < fontSetSize; ++i) {
        memory[fontSetStartAddress + i] = fontSet[i];
    }

    printf("loading ROM \n");
    loadROM(romName);
    printf("ROM done loading \n");

    int pitch = sizeof(display[0]) * SCREEN_WIDTH;

    uint32_t lastCycleTime = SDL_GetTicks();
    bool shouldStop = false;

    while (!shouldStop) {
        shouldStop = proccessInput();

        uint32_t currentTime = SDL_GetTicks();
        uint32_t dt = currentTime - lastCycleTime;
            
        if (dt > delay) {
            lastCycleTime = currentTime;

            fdeLoop();

            updateDisplay(display, pitch);
        }
    }

    return 0;
}

void initTables() {
    table[0x0] = Table0;
    table[0x1] = OP_1NNN;
    table[0x2] = OP_2NNN;
    table[0x3] = OP_3XKK;
    table[0x4] = OP_4XKK;
    table[0x5] = OP_5XY0;
    table[0x6] = OP_6XKK;
    table[0x7] = OP_7XKK;
    table[0x8] = Table8;
    table[0x9] = OP_9XY0;
    table[0xA] = OP_ANNN;
    table[0xB] = OP_BNNN;
    table[0xC] = OP_CXKK;
    table[0xD] = OP_DXYN;
    table[0xE] = TableE;
    table[0xF] = TableF;

    for (int i = 0; i <= 0xE; ++i) {
        table0[i] = OP_NULL;
        table8[i] = OP_NULL;
        tableE[i] = OP_NULL;
    }

    for (int i = 0; i <= 0x65; ++i) {
        tableF[i] = OP_NULL;
    }

    table0[0x0] = OP_00E0;
    table0[0xE] = OP_00EE;

    table8[0x0] = OP_8XY0;
    table8[0x1] = OP_8XY1;
    table8[0x2] = OP_8XY2;
    table8[0x3] = OP_8XY3;
    table8[0x4] = OP_8XY4;
    table8[0x5] = OP_8XY5;
    table8[0x6] = OP_8XY6;
    table8[0x7] = OP_8XY7;
    table8[0xE] = OP_8XYE;

    tableE[0x1] = OP_EXA1;
    tableE[0xE] = OP_EX9E;

    tableF[0x07] = OP_FX07;
    tableF[0x15] = OP_FX15;
    tableF[0x18] = OP_FX18;
    tableF[0x29] = OP_FX29;
    tableF[0x33] = OP_FX33;
    tableF[0x55] = OP_FX55;
    tableF[0x65] = OP_FX65;
    tableF[0x0A] = OP_FX0A;
    tableF[0x1E] = OP_FX1E;
}

void loadROM(char const* fileName) {
    FILE* romFile = fopen(fileName, "rb");
    
    if (romFile != NULL) {
        //Get file size then rewind to the start of the file stream
        fseek(romFile, 0, SEEK_END);
        long size = ftell(romFile);
        rewind(romFile);

        char* buffer = (char*)malloc(size);
        fread(buffer, size, 1, romFile);
        fclose(romFile);

        for (long i = 0; i < size; ++i) {
            memory[startingAddress + i] = buffer[i];
        }

        free(buffer);
    }
}

uint8_t randByte() {
    return rand() % 256;
}

//Instructions

//Vx is a register. x is the last 4 bits of the high byte in a opcode
//Vy is a register. y is the first 4 bits of the low byte in a opcode
//KK is the entire last byte in a opcode
//VF is a register. It's the last register so to refer to it use 0xF

/*
You need to know what ORing to do the rest of ts. So imagine ANDing but if 1 bit out of the 2 is flipped
then the resulting bit is 1. 

Sooo there's such thing as an exclusive OR ik it sucks. Essentially when you compare each bit if the bits aren't
the same then it is set as a 1 if not it's set as a 0

Also ima break down bitwise too here to so essentially logical operators (the ones for booleans)
only operate on the boolean expression level, BUTTTT bitwise operates on the individual integers! huzzzahhhh brudda
*/

//00E0/CLS clears the screen memory
void OP_00E0() {
    memset(display, 0, sizeof(display));
}

//00EE/RET retrieves the previous instruction off the stack and decrements the stack pointer
void OP_00EE() {
    --stackPointer;
    pc = stack[stackPointer];
}

//1NNN/JP jumps to a specific location nnn
void OP_1NNN() {
    pc = opcode & 0x0FFFu;
}

//2NNN/CALL adds a instruction to the stack and increments the stack pointer and then sets the program counter to nnn (12bit instruction)
void OP_2NNN() {
    stack[stackPointer] = pc;
    ++stackPointer;
    pc = opcode & 0x0FFFu;
}

//3XKK/SE skips next instruction if Vx == kk
void OP_3XKK() {
    //index for register Vx
    int x = (opcode & 0x0F00u) >> 8u;
    //Lowest 8 bits of the address
    int kk = opcode & 0x00FFu;

    if (registers[x] == kk) {
        pc += 2;
    } 
}

//4XKK/SNE skip next instruction if Vx != kk
void OP_4XKK() {
    //index for register Vx
    int x = (opcode & 0x0F00u) >> 8u;
    //Lowest 8 bits of the address
    int kk = opcode & 0x00FFu;

    if (registers[x] != kk) {
        pc += 2;
    } 
}

//5XY0/SE skip instruction if Vx == Vy
void OP_5XY0() {
    int x = (opcode & 0x0F00u) >> 8u;
    int y = (opcode & 0x00F0u) >> 4u;

    if (registers[x] == registers[y]) {
        pc += 2;
    }
}

//6XKK/LD interperter(ts program) puts the value KK into register Vx
void OP_6XKK() {
    int x = (opcode & 0x0F00u) >> 8u;
    int kk = (opcode & 0x00FFu);

    registers[x] = kk;
}

//7XKK/ADD adds the value KK to the value of register Vx and then inserts that into Vx
void OP_7XKK() {
    int x = (opcode & 0x0F00u) >> 8u;
    int kk = (opcode & 0x00FFu);

    registers[x] += kk;
}

//8XY0/LD stores the value of register Vy into register Vx
void OP_8XY0() {
    int x = (opcode & 0x0F00u) >> 8u;
    int y = (opcode & 0x00F0u) >> 4u;

    registers[x] = registers[y];
}

//8XY1/OR preform a bitwise OR on the values of Vx and Vy then store in Vx
void OP_8XY1() {
    int x = (opcode & 0x0F00u) >> 8u;
    int y = (opcode & 0x00F0u) >> 4u;

    registers[x] = registers[x] | registers[y];
}

//8XY2/AND preforms a bitwise AND on the values of Vx and Vy then store in Vx (im not explaining ANDing im lazy)
void OP_8XY2() {
    int x = (opcode & 0x0F00u) >> 8u;
    int y = (opcode & 0x00F0u) >> 4u;

    registers[x] = registers[x] & registers[y];
}

//8XY3/XOR preforms a bitwise exclusive OR on the values of Vx and Vy then store in Vx
void OP_8XY3() {
    int x = (opcode & 0x0F00u) >> 8u;
    int y = (opcode & 0x00F0u) >> 4u;

    registers[x] ^= registers[y];
}

/*
8XY4/ADD adds values of Vx and Vy together if the results are greater than 8 bits (255) then VF is set to 1
0 otherwise only lowest 8 bits are kept and stored in Vx
*/
void OP_8XY4() {
    int x = (opcode & 0x0F00u) >> 8u;
    int y = (opcode & 0x00F0u) >> 4u;

    int value = registers[x] + registers[y];

    if (value > 255) {
        registers[0xF] = 1;
        value = value & 0x00FFu;
    } else {
        registers[0xF] = 0;
    }

    registers[x] = value;
}

//8XY5/SUB if Vx > Vy then set Vf to 1 if not 0 then subtract Vx from Vy and store in Vx
void OP_8XY5() {
    int x = (opcode & 0x0F00u) >> 8u;
    int y = (opcode & 0x00F0u) >> 4u;

    if (registers[x] > registers[y]) {
        registers[0xF] = 1;
    } else {
        registers[0xF] = 0;
    }

    registers[x] -= registers[y];
}

//8XY6/SHR if the least signifcant bit of Vx is 1 then set Vf to 1 else 0 then Vx is divided by 2
void OP_8XY6() {
    int x = (opcode & 0x0F00u) >> 8u;
    
    registers[0xF] = (registers[x] & 0x1u);

    registers[x] >>= 1;
}

//8XY7/SUBN If Vy > Vx then Vf = 1 else Vf = 0. then set Vx to Vy - Vx.
void OP_8XY7() {
    int x = (opcode & 0x0F00u) >> 8u;
    int y = (opcode & 0x00F0u) >> 4u;

    if (registers[y] > registers[x]) {
        registers[0xF] = 1;
    } else {
        registers[0xF] = 0;
    }

    registers[x] = registers[y] - registers[x];
}

//8XYE/SHL If most significant bit of Vx is one Vf = 1 else 0 then multiply Vx by 2

/*
This confused me a bit so ima break it down. you get the x and then assign Vf to the most signifcant bit of that register.
Now remember that x is 8 bits since you're sliding the first 8 bits over. so then you get the MSB and slide it right another 7 to get the value of the MSB which is used to set the flag.
Then Vx is slid left by 1 to multiply it by 2 (2^n).
*/
void OP_8XYE() {
    int x = (opcode & 0x0F00u) >> 8u;
    
    registers[0xF] = (registers[x] & 0x80u) >> 7u;
    
    registers[x] <<= 1;
}

//9XY0/SNE skip next instruction if Vx != Vy
void OP_9XY0() {
    int x = (opcode & 0x0F00u) >> 8u;
    int y = (opcode & 0x00F0u) >> 4u;

    if (registers[x] != registers[y]) {
        pc += 2;
    }
}

//ANNN/LD set the value of register I to nnn
void OP_ANNN() {
    uint16_t address = opcode & 0x0FFFu;

    idx = address;
}

//BNNN/JP jump to location nnn + v0
void OP_BNNN() {
    uint16_t address = opcode & 0x0FFFu;

    pc = registers[0] + address;
}

//CXKK/RND set Vx = randbyte & kk
void OP_CXKK() {
    int x = (opcode & 0x0F00u) >> 8u;
    int kk = (opcode & 0x00FFu);

    registers[x] = randByte() & kk;
}

//I hate the amount of typing i did here like this is so much

/*
DXYN/DRW interpereter reads n bytes in memory starting with the address stored at I
and then the bytes are displayed on screen at (Vx, Vy) sprites are XORed on screen
if this causes pixels to be erased set VF to 1 else 0 if a sprite is positioned so it's outside
the cordinates it should wrap around to the opposite side of the screen.
*/

//N in this case is the height of the screen when you go through the for loop

/*
Ts function also gets a more in depth explanation because it happens to be the most confusing thing (so far). So obv you grab x, y, and n then you get the modulus of the values at Vx and Vy so that if its greater than the width or height it wraps around to the other side of the screen. then you start a for loop that goes through the rows of the screen that you need to draw the sprite onto the screen, but if your anything like me you may ask "hmmmm why do we have the index then" well we have it so that you start where your supposed to. Now lets move onto the column loop because thats also super complicated. so a sprite is 8xN n being the number given in the instruction its max is 15. you shift the sprite byte right by 1 every time to check the individual bits to see if the pixel is on or not. Then you get the position of the pixel on the screen you get the address and not the value since you want to actually be able to modify the value in memory and not a copy of the value. the if statement for spritepixel is used to check if the spritepixel is on. Then you check if the pixel is coliding or not if so check the overflow flag (Vf) this is used for collision detection. then you XOR the value in memory to flip the bits on and off.
*/

/*
Just thought of this you get the sprite byte from the rom you loaded into memory dont know how i didnt think of that before
*/
void OP_DXYN() {
    int x = (opcode & 0x0F00u) >> 8u;
    int y = (opcode & 0x00F0u) >> 4u;
    int n = (opcode & 0x000Fu);

    //mod to wrap around if theres overflow
    int xpos = registers[x] % SCREEN_WIDTH;
    int ypos = registers[y] % SCREEN_HEIGHT;

    registers[0xF] = 0;

    for (int row = 0; row < n; ++row) {
        //Get sprite byte starting at i
        int spriteByte = memory[idx + row];

        printf("spriteByte is: %s\n", (spriteByte == 0) ? "zero" : "non-zero");

        for (int col = 0; col < 8; ++col) {
            //Check the individual pixel 0x80 == 10000000 so shift right by num of colums to get a pixel
            int spritePixel = spriteByte & (0x80u >> col);

            //Calc screen pos
            uint32_t* screenPixel = &display[(ypos + row) * SCREEN_WIDTH + (xpos + col)];

            /*check if sprite pixel is on if it is then check for collision with the pixel on screen currently if there is set the collision flag then XOR the screenpixel to flip the pixel bit on and off*/
            if (spritePixel) {
                if (*screenPixel == 0xFFFFFFFF) {
                    registers[0xF] = 1;
                }

                *screenPixel ^= 0xFFFFFFFF;
            }
        }
    }
}

//EX9E/SKP Skip next instruction if the key with the value Vx is pressed
void OP_EX9E() {
    int x = (opcode & 0x0F00u) >> 8u;

    int key = registers[x];

    if (keys[key]) {
        pc += 2;
    }
}

//EXA1/SKNP Skip next instruction if the key with the value Vx is not pressed
void OP_EXA1() {
    int x = (opcode & 0x0F00u) >> 8u;

    int key = registers[x];

    if (!keys[key]) {
        pc += 2;
    }
}

//FX07/LD Set the value of Vx to the delay timer value
void OP_FX07() {
    int x = (opcode & 0x0F00u) >> 8u;

    registers[x] = delayTimer;
}

//FX0A/LD Wait for a key press then store the value of the key in Vx
//If you decrement the pc counter by 2 then it has the same effect has repeating the instruction
void OP_FX0A() {
    int x = (opcode & 0x0F00u) >> 8u;

    if (keys[0]) {
        registers[x] = 0;
    } else if (keys[1]) {
        registers[x] = 1;
    } else if (keys[2]) {
        registers[x] = 2;
    } else if (keys[3]) {
        registers[x] = 3;
    } else if (keys[4]) {
        registers[x] = 4;
    } else if (keys[5]) {
        registers[x] = 5;
    } else if (keys[6]) {
        registers[x] = 6;
    } else if (keys[7]) {
        registers[x] = 7;
    } else if (keys[8]) {
        registers[x] = 8;
    } else if (keys[9]) {
        registers[x] = 9;
    } else if (keys[10]) {
        registers[x] = 10;
    } else if (keys[11]) {
        registers[x] = 11;
    } else if (keys[12]) {
        registers[x] = 12;
    } else if (keys[13]) {
        registers[x] = 13;
    } else if (keys[14]) {
        registers[x] = 14;
    } else if (keys[15]) {
        registers[x] = 15;
    } else {
        pc -= 2;
    }
}

//FX15/LD set the delay timer to the value of Vx
void OP_FX15() {
    int x = (opcode & 0x0F00u) >> 8u;

    delayTimer = registers[x];
}

//FX18/LD set the sound timer to the value of Vx
void OP_FX18() {
    int x = (opcode & 0x0F00u) >> 8u;

    soundTimer = registers[x];
}

//FX1E/ADD I=I+Vx
void OP_FX1E() {
    int x = (opcode & 0x0F00u) >> 8u;

    idx += registers[x];
}

//FX29/LD I=Location of sprite for Vx value
//Yk where the address starts and that each char is 5 bytes so just offset by the digit * len of byte
void OP_FX29() {
    int x = (opcode & 0x0F00u) >> 8u;
    int digit = registers[x];

    idx = fontSetStartAddress + (5 * digit);
}

//FX33/LD gets the decimal value of Vx then converts to BCD, hundreds digit in I, tens in I + 1, and ones in I + 2

/*
The mod operator works here because when you divide by ten you either end up with no extra digit (0) or you end up with a decimal which is taken and put into memory. then you divide by ten to completely remove it so you can check the next digit.
*/
void OP_FX33() {
    int x = (opcode & 0x0F00u) >> 8u;
    int value = registers[x];

    memory[idx + 2] = value % 10;
    value /= 10;

    memory[idx + 1] = value % 10;
    value /= 10;

    memory[idx] = value % 10;
}

//FX55/LD stores registers V0 through Vx in memory starting at location I
void OP_FX55() {
    int x = (opcode & 0x0F00u) >> 8u;

    for (int i = 0; i <= x; ++i) {
        memory[idx + i] = registers[i];
    }
}

//FX65/LD reads registers V0 through Vx from memory starting at location I
void OP_FX65() {
    int x = (opcode & 0x0F00u) >> 8u;

    for (int i = 0; i <= x; ++i) {
        registers[i] = memory[idx + i];
    }
}

void OP_NULL() {}

//Fetch, decode, encode loop
void fdeLoop() {
    //OR to combine the high byte (gets from shifting 8 bits to the left) and the low byte (get from going into the next byte)
    opcode = (memory[pc] << 8u) | memory[pc + 1];
    
    //add 2 to the proccess counter to be able to get the next opcode
    pc += 2;

    //get the first nibble (just realized its called that ik im to far in) then shift it 
    table[(opcode & 0xF000u) >> 12u]();

    if (delayTimer > 0) {
        --delayTimer;
    }

    if (soundTimer > 0) {
        --soundTimer;
    }
}

void initSDL(char const* title, int windowWidth, int windowHeight, int textureWidth, int textureHeight) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("Couldn't init SDL SDL_ERROR: %s", SDL_GetError());
        return;
    }

    sdlVars.window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_SHOWN);

    if (sdlVars.window == NULL) {
        printf("Couldn't create window SDL_ERROR: %s", SDL_GetError());
        return;
    }

    sdlVars.renderer = SDL_CreateRenderer(sdlVars.window, -1, SDL_RENDERER_ACCELERATED);

    sdlVars.texture = SDL_CreateTexture(sdlVars.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight);
}

bool proccessInput() {
    SDL_Event event;
    bool shouldStop = false;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                shouldStop = true;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_1:
                        keys[1] = 1;
                        break;
                    case SDLK_2:
                        keys[2] = 1;
                        break;
                    case SDLK_3:
                        keys[3] = 1;   
                        break;
                    case SDLK_4:
                        keys[0xC] = 1;
                        break;
                    case SDLK_q:
                        keys[4] = 1;
                        break;
                    case SDLK_w:
                        keys[5] = 1;
                        break;
                    case SDLK_e:
                        keys[6] = 1;
                        break;
                    case SDLK_r:
                        keys[0xD] = 1;
                        break;
                    case SDLK_a:
                        keys[7] = 1;
                        break;
                    case SDLK_s:
                        keys[8] = 1;
                        break;
                    case SDLK_d:
                        keys[9] = 1;
                        break;
                    case SDLK_f:
                        keys[0xE] = 1;
                        break;
                    case SDLK_z:
                        keys[0xA] = 1;
                        break;
                    case SDLK_x:
                        keys[0] = 1;
                        break;
                    case SDLK_c:
                        keys[0xB] = 1;
                        break;
                    case SDLK_v:
                        keys[0xF] = 1;  
                        break;                                
                    case SDLK_ESCAPE:
                        shouldStop = true;
                        break;
                    default:
                        break;    
            }

            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case SDLK_1:
                        keys[1] = 0;
                        break;
                    case SDLK_2:
                        keys[2] = 0;
                        break;
                    case SDLK_3:
                        keys[3] = 0;   
                        break;
                    case SDLK_4:
                        keys[0xC] = 0;
                        break;
                    case SDLK_q:
                        keys[4] = 0;
                        break;
                    case SDLK_w:
                        keys[5] = 0;
                        break;
                    case SDLK_e:
                        keys[6] = 0;
                        break;
                    case SDLK_r:
                        keys[0xD] = 0;
                        break;
                    case SDLK_a:
                        keys[7] = 0;
                        break;
                    case SDLK_s:
                        keys[8] = 0;
                        break;
                    case SDLK_d:
                        keys[9] = 0;
                        break;
                    case SDLK_f:
                        keys[0xE] = 0;
                        break;
                    case SDLK_z:
                        keys[0xA] = 0;
                        break;
                    case SDLK_x:
                        keys[0] = 0;
                        break;
                    case SDLK_c:
                        keys[0xB] = 0;
                        break;
                    case SDLK_v:
                        keys[0xF] = 0;
                        break;
                    default:
                        break;    
                }
        }
    }

    return shouldStop;
}

void updateDisplay(void const* buffer, int pitch) {
    SDL_UpdateTexture(sdlVars.texture, NULL, display, pitch);
    SDL_RenderClear(sdlVars.renderer);
    SDL_RenderCopy(sdlVars.renderer, sdlVars.texture, NULL, NULL);
    SDL_RenderPresent(sdlVars.renderer);
}

