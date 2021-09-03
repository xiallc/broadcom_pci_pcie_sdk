#ifndef _CONSFUNC_H
#define _CONSFUNC_H

/*******************************************************************************
 * Copyright 2013-2016 Avago Technologies
 * Copyright (c) 2009 to 2012 PLX Technology Inc.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directorY of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

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
 *      04-01-16 : PLX SDK v7.30
 *
 ******************************************************************************/


#if defined(_WIN32) || defined(_WIN64)
    #include <stdio.h>
    #include <conio.h>
    #include <Windows.h>
    #define PLX_MSWINDOWS
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
#if defined(PLX_MSWINDOWS)

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
    #define Cons_clear                  Plx_clrscr
    #define Cons_fflush                 fflush
    #define Cons_flushinp               do {while (Plx_kbhit()) Plx_getch();} while (0)
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
    #define Cons_flushinp()             do {while (kbhit()) getch();} while (0)
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

// Standard key codes
#define CONS_KEY_NULL                       '\0'
#define CONS_KEY_ESCAPE                     27
#define CONS_KEY_NEWLINE                    '\n'
#define CONS_KEY_CARRIAGE_RET               '\r'
#define CONS_KEY_TAB                        '\t'
#define CONS_KEY_BACKSPACE                  '\b'

// 1st extended key code
#if defined(PLX_LINUX)
    #define CONS_KEY_EXT_CODE               91
    #define CONS_KEY_KEYPAD_CODE            79
#elif defined(PLX_MSWINDOWS)
    #define CONS_KEY_EXT_CODE               224
    #define CONS_KEY_KEYPAD_CODE            1      // For code compatability, not actually used
#elif defined(PLX_DOS)
    #define CONS_KEY_EXT_CODE               0
    #define CONS_KEY_KEYPAD_CODE            1      // For code compatability, not actually used
#endif

// Extended key codes
#if defined(PLX_LINUX)
    #define CONS_KEY_EXT_BACKSPACE          127
    #define CONS_KEY_ARROW_UP               65
    #define CONS_KEY_ARROW_DOWN             66
    #define CONS_KEY_ARROW_LEFT             68
    #define CONS_KEY_ARROW_RIGHT            67
    #define CONS_KEY_HOME                   49
    #define CONS_KEY_HOME_XTERM             72     // Code different in GUI terminal
    #define CONS_KEY_END                    70
    #define CONS_KEY_END_XTERM              52     // Code different in GUI terminal
    #define CONS_KEY_INSERT                 50
    #define CONS_KEY_DELETE                 51
    #define CONS_KEY_PAGE_UP                53
    #define CONS_KEY_PAGE_DOWN              54
#else
    #define CONS_KEY_EXT_BACKSPACE          127
    #define CONS_KEY_ARROW_UP               72
    #define CONS_KEY_ARROW_DOWN             80
    #define CONS_KEY_ARROW_LEFT             75
    #define CONS_KEY_ARROW_RIGHT            77
    #define CONS_KEY_HOME                   71
    #define CONS_KEY_HOME_XTERM             254    // Added for code compatability
    #define CONS_KEY_END                    79
    #define CONS_KEY_END_XTERM              253    // Added for code compatability
    #define CONS_KEY_INSERT                 82
    #define CONS_KEY_DELETE                 83
    #define CONS_KEY_PAGE_UP                73
    #define CONS_KEY_PAGE_DOWN              81
#endif

// Preset cursor sizes/types
#define CONS_CURSOR_DISABLED                0
#define CONS_CURSOR_UNDERLINE               20
#define CONS_CURSOR_INSERT                  70
#define CONS_CURSOR_DEFAULT                 CONS_CURSOR_UNDERLINE




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
ConsoleCursorPropertiesSet(
    int size
    );

unsigned char
ConsoleIoThrottleGet(
    void
    );

void
ConsoleIoThrottleSet(
    unsigned char bEnable
    );

void
ConsoleIoThrottleReset(
    void
    );

void
ConsoleIoThrottleLock(
    unsigned char bLock
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

void
Plx_clrscr(
    void
    );

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
