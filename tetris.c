/*
 * Play Tetris!
 * I made a recreation of the classic falling-block game
 * This version does not support swapping pieces
 * It is not entirely accurate to any published versions, but the idea is the same
 * have fun!
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>

#define WIDTH 10
#define HEIGHT 26

#define RIGHT -1
#define LEFT   1

char pieces[8][4][2] = {
    {{0,0},{0,0},{0,0},{0,0}},
    {{0,0}, {1,0}, {1,1}, {0,1}}, // O
    {{0,-1},{0,0},{0,1},{0,2}},   // I
    {{-1,0},{0,0},{0,1},{1,1}},   // S
    {{-1,1},{0,1},{0,0},{1,0}},   // Z
    {{0,1},{0,0},{0,-1},{1,-1}},  // L
    {{0,1},{0,0},{0,-1},{-1,-1}}, // J
    {{-1,0},{0,0},{1,0},{0,-1}}   // T
};

typedef struct {
    char points[4][2];
    char pos[2];
    char type;
} piece;

int colors[8] = {0,226,87,34,196,208,33,129};

int clearRows(char map[HEIGHT][WIDTH]);         // clear all full rows, and drop remaining pieces down
void disableRaw();                              // reset terminal input to normal
void drop(piece *p, char map[HEIGHT][WIDTH]);   // hard drop piece
void enableRaw(int min);                        // enable return of read() after (int) min characters read. (don't wait for newline)
void fillBatch(char batch[7]);                  // batch system randomizer. fill batch[7] numbers 1 to 7, occuring once each 
int fullRow(char map[HEIGHT][WIDTH], int row);  // return number of filled spaces at (int) row of map

int move(piece *p, char map[HEIGHT][WIDTH], char dx, char dy);                          // moves pieces, if movement is not allowed, return 0, otherwise 1
char newPiece(piece *p, char type);                                                     // fill piece *p with info about the new piece specified by (char) type
void printMap(char map[HEIGHT][WIDTH], piece *active, char preview[3][4],int score);    // display the map
int rotate(piece *p, char map[HEIGHT][WIDTH], char direction);                          // rotate pieces. if rotate is not allowed return 0, otherwise 1
void setPreview(char type, char preview[3][4]);                                         // set preview that shows next piece. 
void setup(char map[HEIGHT][WIDTH],piece *active, char batch[7], char preview[3][4]);   // zero out map, position piece, fill batch, set preview

int main() {
    char map[HEIGHT][WIDTH];
    char batch[7];
    char preview[3][4];
    piece active;
    char c;
    clock_t timer;
    clock_t delay = 500000;
    int update = 1;
    int new = 0;
    srand(clock());
    srand(rand());

    int next = 1;
    int score = 0;
    int playing = 1;

    enableRaw(1); 
    // instructions, wait for player to be ready
    setup(map,&active,batch,preview);
    printf("\033cPlay tetris!\n'w' to turn\n'a' and 'd' to move left and right\n's' to soft drop\n' ' to hard drop\n Press any key to begin\n");
    read(0,NULL,1);
    enableRaw(0);
    timer = clock();
    while(playing) {
        if(update) { // update true only if timer expired or player moved piece
            printMap(map,&active,preview,score);
            update = 0;
        }
        if(read(0,&c,1)) { // player input
            switch(c) {
                case 'w': // rotate
                    rotate(&active,map,-1);
                    update = 1;
                    break;
                case 's': // soft drop
                    move(&active,map,0,-1);
                    update = 1;
                    break;
                case 'd': // move right
                    move(&active,map,1,0);
                    update = 1;
                    break;
                case 'a': // move left
                    move(&active,map,-1,0);
                    update = 1;
                    break;
                case ' ': // hard drop, get next piece
                    drop(&active,map);
                    update = new = 1;
                    break;
            }
        }
        if(playing) {
            if(clock() - timer > delay) { // timer expired
                if(!move(&active,map,0,-1)) { // piece hits bottom or previous pieces
                    new = 1;
                }
                update = clearRows(map);
                if(update) { // get faster if any rows were cleared
                    delay /= 1.1;
                    switch(update) { // increase score based on number of lines cleared
                        case 1:
                            score += 100;
                            break;
                        case 2:
                            score += 300;
                            break;
                        case 3:
                            score += 500;
                            break;
                        case 4:
                            score += 800;
                    }
                }
                update = 1;
                timer = clock(); // reset timer
            }
            if(new) { // if new piece needed
                for(int i = 0; i < 4; ++i)
                    map[active.pos[1] + active.points[i][1]][active.pos[0] + active.points[i][0]] = active.type;
                if(fullRow(map,20)) // check if any blocks are above the top line
                    playing = 0;
                else { // get the new piece, refilling the batch if needed
                    newPiece(&active,batch[next++]);
                    if(next > 6) {
                        fillBatch(batch);
                        next = 0;
                    }
                    setPreview(batch[next],preview);
                    new = 0;
                }
            }
        }
        if(!playing) {
            enableRaw(1);
            // ask to play again
            printf("Game over. Would you like to play again? (y/n):\n");
            c = 0;
            while(c != 'y' && c != 'n')
                read(0,&c,1);
            if(c == 'y') { // if affirmed, reset everything.
                delay = 500000;
                update = 1;
                new = 0;
                next = 1;
                score = 0;
                playing = 1;
                enableRaw(0);
                setup(map,&active,batch,preview);
                timer = clock();
            }
        }
    }
    disableRaw();
}

int clearRows(char map[HEIGHT][WIDTH]) {
    int offset = 0;
    for(int i = 0; i < HEIGHT; ++i) {
        while(fullRow(map,i+offset) == WIDTH)
            ++offset;
        if(offset) {
            if(i + offset >= HEIGHT) {
                memset(map[i], 0, 10);
            }
            else {
                memcpy(map[i],map[i+offset],10);
            }
        }
    }
    return offset;
}

void disableRaw() {
    struct termios t;
    tcgetattr(0,&t);
    t.c_lflag |= (ECHO|ICANON);
    tcsetattr(0,0,&t);
}

void drop(piece *p, char map[HEIGHT][WIDTH]) {
    int x, y;
    while(2) {
        p->pos[1] -= 1;
        for(int i = 0; i < 4; ++i) {
            x = p->pos[0] + p->points[i][0];
            y = p->pos[1] + p->points[i][1];
            if(map[y][x] || y < 0) {
                p->pos[1] += 1;
                return;
            }
        }
    }
}

void enableRaw(int min) {
    struct termios t;
    tcgetattr(0,&t);
    t.c_cc[VTIME] = 0;
    t.c_cc[VMIN] = min;
    t.c_lflag &= ~(ECHO|ICANON);
    tcsetattr(0,0,&t);
}

void fillBatch(char batch[7]) {
    char tmp[7] = {1,1,1,1,1,1,1};
    char a;
    for(int i = 0; i < 7; ++i) {
        a = (rand()%22)%7;
        for(; !tmp[a]; a = (a+1)%7);
        tmp[a] = 0;
        batch[i] = a + 1;
    }
}

int fullRow(char map[HEIGHT][WIDTH], int row) {
    int out = 0;
    for(int i = 0; i < WIDTH; ++i) {
        if(map[row][i])
            out++;
    }
    return out;
}

int move(piece *p, char map[HEIGHT][WIDTH], char dx, char dy) {
    piece tmp = *p;
    tmp.pos[0] += dx;
    tmp.pos[1] += dy;
    int x, y;
    for(int i = 0; i < 4; ++i) {
        x = tmp.points[i][0] +  tmp.pos[0];
        y = tmp.points[i][1] +  tmp.pos[1];
        if(x < 0 || x > WIDTH-1 || y < 0 || map[y][x]) // check whether movement is allowed
            return 0;
    }
    p->pos[0] = tmp.pos[0];
    p->pos[1] = tmp.pos[1];
    return 1;
}

char newPiece(piece *p, char type) {
    p->type = type;
    memcpy(p->points,pieces[type],8);
    p->pos[0] = 5;
    p->pos[1] = 23;
    return p->type;

}

void printMap(char map[HEIGHT][WIDTH], piece *active, char preview[3][4],int score) {
    printf("\033c"); // clear screen
    int y;
    int x;
    int color;
    for(int i = 0; i < 4; ++i) { // place the current piece onto the map
        if((y = active->pos[1] + active->points[i][1]) < HEIGHT)
            map[y][active->pos[0] + active->points[i][0]] = -active->type;
    }
    printf("Score: %d\n",score);
    for(y = 3; y >= 0; --y) { // display next piece
        for(x = 0; x < 3; ++x) {
            if((color = preview[x][y]))
                printf("\033[38;5;%dm[]\033[0m",colors[preview[x][y]]);
            else
                printf("  ");
        }
        printf("\n");
    }
    for(y = HEIGHT; y >= -1; --y) {
        for(x = -1; x <= WIDTH; ++x) {
            if(y == HEIGHT)
                printf("__"); // top of field
            else if(x == -1)
                printf("<!");
            else if(x == WIDTH)
                printf("!>");
            else if(y == -1)
                printf("=="); // bottom of field
            else if(y == 20 && !map[y][x])
                printf("--");
            else if(map[y][x] != 0) {
                if(map[y][x] < 0) { // if it is the active piece, remove it so it won't stay after the function finishes
                    color = -map[y][x];
                    map[y][x] = 0;
                }
                else
                    color = map[y][x];
                printf("\033[38;5;%dm[]\033[0m",colors[color]); // print the pieces with their colors
            }
            else 
                printf(" .");
        }
        printf("\n");
    }
}

int rotate(piece *p, char map[HEIGHT][WIDTH], char direction) {
    piece tmp = *p;
    char x, y;
    for(int i = 0; i < 4; ++i) {
        tmp.points[i][0] = -1*p->points[i][1]*direction;
        tmp.points[i][1] = p->points[i][0]*direction;
        x = tmp.points[i][0] +  tmp.pos[0];
        y = tmp.points[i][1] +  tmp.pos[1];
        if(x < 0 || x > WIDTH-1 || y < 0 || map[y][x])
            return 0;
    }
    *p = tmp;
    return 1;
}

void setPreview(char type, char preview[3][4]) {
    memset(preview,0,12);
    for(int i = 0; i < 4; ++i)
        preview[pieces[type][i][0] + 1][pieces[type][i][1] + 1] = type;
}

void setup(char map[HEIGHT][WIDTH],piece *active, char batch[7], char preview[3][4]) {
    memset(map,0,WIDTH*HEIGHT*sizeof(char));
    fillBatch(batch);
    newPiece(active,batch[0]);
    setPreview(batch[1],preview);
}