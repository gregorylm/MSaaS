
/*
 * Simple Sub/Pub test program.
 *
 * Subscribe to a redis channel and read from it. This is done asynchronously so the process
 * starts waiting for published data. The process turminates when a end of file (_EOF_) is recieved.
 * You need to have a functioning redis server running and the hiredis library installed. The
 * source code can be downloaded from:
 *
 * https://github.com/redis/hiredis.git
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GL/glut.h>

#include <ae.h>
#include <hiredis.h>
#include <async.h>
#include <adapters/ae.h>

#define FALSE 0
#define TRUE 1

// Cellspace dimensions
#define WIDTH 100
#define HEIGHT 100

// Window and cell dimensions. 
#define CELL_SIZE 6
const GLint win_width = WIDTH * CELL_SIZE;
const GLint win_height = HEIGHT * CELL_SIZE;

static aeEventLoop *loop;                                       // Put event loop in the global scope, so it
                                                                // can be explicitly stopped 

/*
 * This function will be called when data is received on the redis channel. See main routine
 * and the redisAsyncCommand function call.
 *
 * When data is received from the "Publish" program it will be parced and then plotted.
 */

void SubscribeCallback(redisAsyncContext * c, redisReply * r, void *privdata)
{
    redisReply *reply = r;                                      // Save the redisReply for use
    redisReply *rp;                                             // A pointer to loop throught the elements
    size_t j;                                                   // A loop varabil
    int x, y;
    char *startnum, *endnum;
    int exitSubPub = FALSE;                                     // A switch so that the async loop can be stopped

    if (reply == NULL)                                          // Handel error
        return;

    static int init = TRUE;                                     // Initilize the glut display
    if (init)
    {
        init = FALSE;
        glutUseLayer(GLUT_NORMAL);
        glClearColor(0.0, 0.0, 1.0, 1.0);
        glColor3f(1.0, 1.0, 1.0);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, (float) win_width, 0.0, (float) win_height, 1.0, -1.0);
    }

/*
 * The published data is recieved as a linked list of data elements. Data that was published is in the last
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
    if (reply->element != NULL)                                 // Check to make sure that data was returned
    {
        for (j = 0; j < reply->elements; j++)                   // Loop through the elements
        {
            if (reply->element[j] != NULL)                      // Handle elements with data
            {
                rp = reply->element[j];                         // Set a pointer to the element to access it data
                if (rp->str != NULL)                            // If we have a stirng display it on the terminal
                {
                    if (strncmp("data", rp->str, 4) == 0)       // If this is a data element extract the data
                    {
                        if (strstr(rp->str, "Alive"))           // Plot the data if the cell is alive
                        {
                            startnum = index(rp->str, ' ') + 1; // Get the X and Y data
                            endnum = index(startnum, ' ');
                            *endnum = '\0';
                            x = atoi(startnum);
                            startnum = endnum + 1;
                            endnum = index(startnum, ' ');
                            *endnum = '\0';
                            y = atoi(startnum);
                            GLint wx = CELL_SIZE * x;           // Convert to the display space and set the cell
                            GLint wy = CELL_SIZE * y;
                            glRecti(wx, wy, wx + CELL_SIZE, wy + CELL_SIZE);
                        }
                    }
                    else if (strncmp("swap", rp->str, 4) == 0)  // The publish program send when to swap buffers
                    {
                        glutSwapBuffers();
                    }
                    else if (strncmp("clear", rp->str, 5) == 0) // The publish program send when to clear and start over
                    {
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    }
                    else if (strcmp("_EOF_", rp->str) == 0)     // Check for end of file
                    {
                        exitSubPub = TRUE;                      // If so exit the program
                    }
                }
            }
        }
    }

    if (exitSubPub)                                             // Disconnect after receiving end of published messages
        redisAsyncDisconnect(c);
}

/*
 * Callback When a connections is made to check for errors.
 *
 */

void connectCallback(const redisAsyncContext * c, int status)
{
    if (status != REDIS_OK)                                     // Handel errors
    {
        printf("Error: %s\n", c->errstr);
        aeStop(loop);
        return;
    }

    printf("Connected...\n");
}

/*
 * Callback when a channel is disconnected to check for errors.
 *
 */

void disconnectCallback(const redisAsyncContext * c, int status)
{
    if (status != REDIS_OK)                                     //Handel errors
    {
        printf("Error: %s\n", c->errstr);
        aeStop(loop);
        return;
    }

    printf("Disconnected...\n");
    aeStop(loop);
}

/*
 * Display the command line usage on error.
 */

void Usage(char *command)
{
    printf("\nUsage: %s [-h host] [-p port]\n\n", command);
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
            default:                                           // Unknowen option print error message
                printf("\nError Option %s not found\n\n", argv[i]);
                Usage(argv[0]);
                return -1;
            }
        }
        printf("\nUsing host %s port %i\n\n", hostip, hostport);
    }

    redisAsyncContext *c = redisAsyncConnect(hostip, hostport); // Get the redis context
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

    loop = aeCreateEventLoop(64);                               // Create a loop event. argument is unknowen
    redisAeAttach(loop, c);                                     // Attached the context to the loop
    redisAsyncSetConnectCallback(c, connectCallback);           // Set the connection callback
    redisAsyncSetDisconnectCallback(c, disconnectCallback);     // Set the disconnect callback

    // Setup the display
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(win_width, win_height);
    glutCreateWindow("SubGlife");
    glutPositionWindow(0, 0);

    /*
     * This sets up the call back for the SUBSCRIBE redis data base. See above for the call back function.
     * The string "sscpactest" is private data that will be included in the call to the callback funciont. Iam
     * not sure just how that is intened to be used. The command that will be use is the last argument. The
     * Program will subscribe to the sscpactest channel and keep looking for new data until the loop is
     * terminated.
     */

    redisAsyncCommand(c, (redisCallbackFn *) SubscribeCallback, (char *) "AsyncTest", "SUBSCRIBE sscpactest");
    aeMain(loop);                                               // Start the async loop running
    return 0;
}
