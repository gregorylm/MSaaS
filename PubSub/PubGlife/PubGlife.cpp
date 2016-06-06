
/*
 * Simple Sub/Pub test program.
 * 
 * Publish the game of life data, as it is generated, to the redis channel. This is done synchronously so the process
 * starts calculating and publishing data. The process turminates when the life span is reached and a end of file
 * (_EOF_) is published. You need to have a functioning redis server running and the hiredis library installed. The
 * source code can be downloaded from:
 *
 * https://github.com/redis/hiredis.git
 *
 * The published data is sent as a linked list of data elements. Data that was published is in the last
 * element. In the first element the string is "message", the second is the private data, the third is the 
 * published data. An example of the data:
 *
 * "message"
 * "sscpactest"
 * "data 99 84 Alive"
 *
 * Commands that are included in the data are as follows:
 *
 * data x y status => the data to be plotted, x and y values, and the life status.
 * swap            => in the glife program the screen buffer is swapped so swap the buffers
 * clear           => in the glife program the screen buffer is cleared so clear the buffer
 * _EOF_           => This is the End Of File so the the program will terminate
 *
 */

#include "adevs.h"
#include "Cell.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string.h>
#include <GL/freeglut_std.h>
#include <GL/freeglut_ext.h>
#include "hiredis.h"
using namespace std;

// Cellspace dimensions
#define WIDTH 100
#define HEIGHT 100
// Phase space to visualize
Phase phase[WIDTH][HEIGHT];

// Window and cell dimensions. 
#define CELL_SIZE 6
const GLint win_width = WIDTH * CELL_SIZE;
const GLint win_height = HEIGHT * CELL_SIZE;

redisContext *c;                                                // Redis context for publishing to the channel
static int life = 0;                                            // Loop vareable
static int lifeSpan = 6;                                        // Default life span

/*
 * This function is a callback that displays the game of life data calculated by the simulatoin
 */

void drawSpace()
{
    static bool init = true;                                    // initilize the displey on the first call
    char ioBuff[1024];
    if (init)
    {
        init = false;
        glutUseLayer(GLUT_NORMAL);
        glClearColor(0.0, 0.0, 1.0, 1.0);
        glColor3f(1.0, 1.0, 1.0);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, (float) win_width, 0.0, (float) win_height, 1.0, -1.0);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);         // Clear the display
    redisCommand(c, "PUBLISH sscpactest clear");                // Put the clear command on the channel
    for (int x = 0; x < WIDTH; x++)                             // loop throught the phase data
    {
        for (int y = 0; y < HEIGHT; y++)
        {
            if (phase[x][y] == Alive)                           // Phase is calculated in the simulation function
            {
                GLint wx = CELL_SIZE * x;                       // If the cell is alive plot the cell
                GLint wy = CELL_SIZE * y;
                glRecti(wx, wy, wx + CELL_SIZE, wy + CELL_SIZE);
                sprintf(ioBuff, "data %i %i Alive", x, y);      // Build the data buffer
            }
            redisCommand(c, "PUBLISH sscpactest %s", ioBuff);   // Put the data on the channel
        }
    }
    redisCommand(c, "PUBLISH sscpactest swap");                 // Put the swap command on the channel
    glutSwapBuffers();
    if (life++ > lifeSpan)                                      // Exit the simulation after the life span is exceded
    {
        redisCommand(c, "PUBLISH sscpactest _EOF_");            // Put end of file on the channel
        glutLeaveMainLoop();
    }

}

/*
 * This is a DEVS model that implements Conway's Game of Life. count_living_cells and simulateSpace are from
 * adevs-code-323-trunk/examples/glife
 *
 */

short int count_living_cells(int x, int y)
{
    short int nalive = 0;
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            int xx = (x + dx) % WIDTH;
            int yy = (y + dy) % HEIGHT;
            if (xx < 0)
                xx = WIDTH - 1;
            if (yy < 0)
                yy = HEIGHT - 1;
            if (phase[xx][yy] == Alive && !(xx == x && yy == y))
            {
                nalive++;
            }
        }
    }
    return nalive;
}

void simulateSpace()
{
    // Seed the random number generator
    srand(time(NULL));
    // Dynamic cellspace model and simulator
    static adevs::CellSpace < Phase > *cell_space = NULL;
    static adevs::Simulator < CellEvent > *sim = NULL;
    // Reset the space if everything has died
    if (cell_space == NULL)
    {
        // Create the cell state variable array
        for (int x = 0; x < WIDTH; x++)
        {
            for (int y = 0; y < HEIGHT; y++)
            {
                if (rand() % 8 == 0)
                    phase[x][y] = Alive;
                else
                    phase[x][y] = Dead;
            }
        }
        // Create the cellspace model
        cell_space = new adevs::CellSpace < Phase > (WIDTH, HEIGHT);
        for (int x = 0; x < WIDTH; x++)
        {
            for (int y = 0; y < HEIGHT; y++)
            {
                // Count the living neighbors
                short int nalive = count_living_cells(x, y);
                cell_space->add(new Cell(x, y, WIDTH, HEIGHT, phase[x][y], nalive, &(phase[x][y])), x, y);
            }
        }
        // Create a simulator for the model
        sim = new adevs::Simulator < CellEvent > (cell_space);
    }
    // If everything has died, then restart on the next call
    if (sim->nextEventTime() == DBL_MAX)
    {
        delete cell_space;
        delete sim;
        sim = NULL;
        cell_space = NULL;
    }
    // Run the next simulation step
    else
    {
        sim->execNextEvent();
    }
    // Draw the updated display
    drawSpace();
}

/*
 * Display the command line usage on error.
 */

void Usage(char *command)
{
    printf("\nUsage: %s [-h host] [-p port] [-l lifespan\n\n", command);
}

int main(int argc, char **argv)
{
    char hostip[256], sChar;
    int hostport, i;
    strcpy(hostip, "127.0.0.1");                                // Radis default host
    hostport = 6379;                                            // Redis default port

    if ((argc - 1) % 2 == 1)                                    // If argc odd arg miss match
    {
        printf("\nInsufficient arguments");
        Usage(argv[0]);
        return -1;
    }

    if (argc == 1)                                              // If host and/or port option are not set use defaults
    {
        printf("\nUsing defaults host %s port %i\n\n", hostip, hostport);
    }
    else
    {
        for (i = 1; i < argc; i += 2)                           // Loop through the arguments. Assume pairs
        {
            sChar = *(argv[i] + 1);                             // Get the option
            if (*argv[i + 1] == '-')
            {
                printf("\nError missing argument '%s %s' Check the command line", argv[i], argv[i + 1]);
                Usage(argv[0]);
                return -1;
            }
            switch (sChar)
            {
            case 'h':                                          // Host option set get host name/IP
                strcpy(hostip, argv[i + 1]);
                break;
            case 'p':                                          // Port option set get port number
                hostport = atoi(argv[i + 1]);
                break;
            case 'l':                                          // Port option set get port number
                lifeSpan = atoi(argv[i + 1]);
                break;
            default:                                           // Unknowen option print error message
                printf("\nError Option %s not found\n\n", argv[i]);
                Usage(argv[0]);
                return -1;
            }
        }
        printf("\nUsing host %s port %i\n\n", hostip, hostport);
    }

    c = redisConnect(hostip, hostport);                         // Get the redis context
    if (c == NULL || c->err)                                    // Handle connection errors
    {
        if (c->err)                                             // If redisConnect returned an error print it
        {
            printf("Error: %s\n", c->errstr);
        }
        else
        {
            printf("Can't allocate redis context\n");
        }
        return -1;                                              // Return error status
    }
    // Setup the display
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(win_width, win_height);
    glutCreateWindow("PubGlife");
    glutPositionWindow(0, 0);
    glutDisplayFunc(drawSpace);
    glutIdleFunc(simulateSpace);
    glutMainLoop();
    // Done
    return 0;
}
