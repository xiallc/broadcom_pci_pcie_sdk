#ifndef _CONSFUNC_H
#define _CONSFUNC_H

/******************************************************************************
 *
 * File Name:
 *
 *      ConsFunc.h
 *
 * Description:
 *
 *      Header file for the Console functions
 *
 * Revision History:
 *
 *      02-01-10 : PLX SDK v6.40
 *
 ******************************************************************************/


#if defined(_WIN32) || defined(_WIN64)
    #include <stdio.h>
    #include <conio.h>
    #include <Windows.h>
#elif defined(PLX_LINUX)
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <termios.h> 
    #include <sys/ioctl.h> 
    #include <sys/time.h> 
    #include <sys/types.h>
#elif defined(PLX_DOS)
    #include <stdio.h>
    #include <conio.h>
    #include <string.h>
    #include <unistd.h>
    #include <dpmi.h>
#endif




/*************************************
 *          Definitions
 ************************************/
#if defined(_WIN32) || defined(_WIN64)

    #define Plx_sleep                   Sleep
    #define Plx_strcmp                  strcmp
    #define Plx_strcasecmp              stricmp
    #define Plx_strncasecmp             strnicmp
    #define Cons_clear                  Plx_clrscr
    #define Cons_fflush                 fflush
    #define Cons_flushinp()             FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE))
    #define Cons_fputs                  Plx_fputs
    #define Cons_kbhit                  _kbhit
    #define Cons_getch                  _getch
    #define Cons_puts                   puts
    #define Cons_putchar                putchar
    #define Cons_scanf                  scanf
    #define Cons_printf                 Plx_printf

#elif defined(PLX_LINUX)

    #define Plx_sleep(arg)              usleep((arg) * 1000)
    #define Plx_strcmp                  strcmp
    #define Plx_strcasecmp              strcasecmp
    #define Plx_strncasecmp             strncasecmp
    #define Cons_clear()                system("clear")
    #define Cons_fflush                 fflush
    #define Cons_flushinp               flushinp
    #define Cons_fputs                  Plx_fputs
    #define Cons_kbhit                  Plx_kbhit
    #define Cons_getch                  Plx_getch
    #define Cons_puts                   puts
    #define Cons_putchar                putchar
    #define Cons_scanf                  scanf
    #define Cons_printf                 Plx_printf

#elif defined(PLX_DOS)

    #define Plx_sleep(arg)              usleep((arg) * 1000)
    #define Plx_strcmp                  strcmp
    #define Plx_strcasecmp              strcasecmp
    #define Plx_strncasecmp             strncasecmp
    #define Cons_clear                  clrscr
    #define Cons_fflush                 fflush
    #define Cons_flushinp               (do {while (kbhit()) getch();} while (0))
    #define Cons_fputs                  Plx_fputs
    #define Cons_kbhit                  kbhit
    #define Cons_getch                  getch
    #define Cons_puts                   puts
    #define Cons_putchar                putchar
    #define Cons_scanf                  scanf
    #define Cons_printf                 Plx_printf

#endif


#if !defined(min)
    #define min(a, b)               (((a) < (b)) ? (a) : (b))
#endif


/******************************************************************
 * A 64-bit HEX value (0xFFFF FFFF FFFF FFFF) requires 20 decimal
 * digits or 22 octal digits. The following constant defines the
 * buffer size used to hold an ANSI string converted from a
 * 64-bit HEX value.
 *****************************************************************/
#define MAX_DECIMAL_BUFFER_SIZE         30

#define DEFAULT_SCREEN_SIZE             25  // Default lines to display before halting, if enabled
#define SCREEN_THROTTLE_OFFSET          2   // Num lines to offset for halting

#define _Pause                                                \
    do                                                        \
    {                                                         \
        Cons_printf("  -- Press any key to continue --");     \
        Cons_getch();                                         \
        Cons_printf("\r                                 \r"); \
    }                                                         \
    while(0)


#define _PauseWithExit                                                           \
    do                                                                           \
    {                                                                            \
        Cons_printf("  -- Press any key to continue or ESC to exit --");         \
        if (Cons_getch() == 27)                                                  \
        {                                                                        \
            Cons_printf("\r                                                \n"); \
            ConsoleEnd();                                                        \
            exit(0);                                                             \
        }                                                                        \
        Cons_printf("\r                                                \r");     \
    }                                                                            \
    while(0)




/*************************************
 *            Functions
 ************************************/
void
ConsoleInitialize(
    void
    );

void
ConsoleEnd(
    void
    );

unsigned short
ConsoleScreenHeightSet(
    unsigned short NumLines
    );

unsigned short
ConsoleScreenHeightGet(
    void
    );

void
ConsoleIoThrottle(
    unsigned char bEnable
    );

void
ConsoleIoThrottleReset(
    void
    );

void
ConsoleIoIncrementLine(
    void
    );

void
ConsoleIoOutputDisable(
    unsigned char bEnable
    );

int
Plx_fputs(
    const char *string,
    FILE       *stream
    );

int
Plx_printf(
    const char *format,
    ...
    );


// Windows-specific functions
#if defined(_WIN32) || defined(_WIN64)

void
Plx_clrscr(
    void
    );

#endif


// Linux-specific functions
#if defined(PLX_LINUX)

int
Plx_kbhit(
    void
    );

int
Plx_getch(
    void
    );

#endif



#endif
