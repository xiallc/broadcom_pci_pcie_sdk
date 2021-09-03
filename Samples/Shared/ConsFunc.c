/******************************************************************************
 *
 * File Name:
 *
 *      ConsFunc.c
 *
 * Description:
 *
 *      Provides a common layer to work with the console
 *
 * Revision History:
 *
 *      05-01-13 : PLX SDK v7.10
 *
 ******************************************************************************/


#include <ctype.h>      // For tolower()
#include <stdarg.h>     // For va_start() & va_end()
#include "ConsFunc.h"
#include "PlxTypes.h"




/*************************************
 *            Globals
 ************************************/
static unsigned char  _Gbl_bThrottleOutput = FALSE;
static unsigned char  _Gbl_bPausePending   = FALSE;
static unsigned char  _Gbl_bOutputDisable  = FALSE;
static unsigned short _Gbl_LineCount       = 0;
static unsigned short _Gbl_LineCountMax    = DEFAULT_SCREEN_SIZE - SCREEN_THROTTLE_OFFSET;




/******************************************************************
 *
 * Function   :  ConsoleInitialize
 *
 * Description:  Initialize the console
 *
 *****************************************************************/
void
ConsoleInitialize(
    void
    )
{
    ConsoleScreenHeightGet();
    ConsoleCursorPropertiesSet( CONS_CURSOR_DEFAULT );
}




/******************************************************************
 *
 * Function   :  ConsoleEnd
 *
 * Description:  Restore the console
 *
 *****************************************************************/
void
ConsoleEnd(
    void
    )
{
}




/******************************************************************
 *
 * Function   :  ConsoleScreenHeightSet
 *
 * Description:  Returns the current size of the console in number of lines
 *
 *****************************************************************/
unsigned short ConsoleScreenHeightGet()
{
    unsigned short ConsoleSize;

#if defined(PLX_LINUX)

    int            ret;
    struct winsize ConsoleWindow;


    ret = ioctl( STDIN_FILENO, TIOCGWINSZ, &ConsoleWindow );
    if (ret == 0)
        ConsoleSize = (unsigned short)ConsoleWindow.ws_row;
    else
        ConsoleSize = DEFAULT_SCREEN_SIZE;

#elif defined(PLX_MSWINDOWS)

    CONSOLE_CURSOR_INFO        cci;
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo( GetStdHandle(STD_OUTPUT_HANDLE), &csbi );
    ConsoleSize = (unsigned short)((csbi.srWindow.Bottom - csbi.srWindow.Top) + 1);

    cci.dwSize   = 15;
    cci.bVisible = TRUE;
    SetConsoleCursorInfo( GetStdHandle(STD_OUTPUT_HANDLE), &cci );

#elif defined(PLX_DOS)

    struct text_info screen_info;


    gettextinfo( &screen_info );
    ConsoleSize = (unsigned short)screen_info.screenheight;
    _setcursortype( _NORMALCURSOR );

#endif

    // Set the max line count based on current screen size
    _Gbl_LineCountMax = ConsoleSize - SCREEN_THROTTLE_OFFSET;
    return ConsoleSize;
}




/******************************************************************
 *
 * Function   :  ConsoleScreenHeightSet
 *
 * Description:  Sets the size of the console in number of lines
 *
 *****************************************************************/
unsigned short
ConsoleScreenHeightSet(
    unsigned short NumLines
    )
{
#if defined(PLX_LINUX)

    // Not fully supported yet in Linux
    return -1;

    /*************************************************
     * The following code is capable of adjusting the
     * console height, but it does not re-size the
     * terminal window.  It is left here for future
     * reference, but is disabled at this time.
     ************************************************/
  #if 0
    int            ret;
    struct winsize ConsoleWindow;


    // Get current console size
    ret = ioctl( STDIN_FILENO, TIOCGWINSZ, &ConsoleWindow );

    if (ret != 0)
        return -1;

    // Set new window height
    ConsoleWindow.ws_row = NumLines;

    // Adjust window to new height
    ret = ioctl( STDIN_FILENO, TIOCSWINSZ, &ConsoleWindow );

    if (ret != 0)
        return -1;

    // Update internal limit
    _Gbl_LineCountMax = NumLines - SCREEN_THROTTLE_OFFSET;

    return ret;
  #endif

#elif defined(PLX_MSWINDOWS)

    SMALL_RECT WindowSize;


    // Set the new window size
    WindowSize.Left   = 0;
    WindowSize.Right  = 80 - 1;
    WindowSize.Top    = 0;
    WindowSize.Bottom = NumLines - 1;

    // Re-size window
    if (SetConsoleWindowInfo(
            GetStdHandle(STD_OUTPUT_HANDLE),
            TRUE,      // Treat coordinates as absolute
            &WindowSize
            ) == 0)
    {
        return -1;
    }

    // Update internal limit
    _Gbl_LineCountMax = NumLines - SCREEN_THROTTLE_OFFSET;

    return 0;

#elif defined(PLX_DOS)

    // Verify valid setting
    if ((NumLines != 25) &&
        (NumLines != 43) &&
        (NumLines != 50))
    {
        return -1;
    }

    // Set new screen size
    _set_screen_lines(
        NumLines
        );

    // Update internal limit
    _Gbl_LineCountMax = NumLines - SCREEN_THROTTLE_OFFSET;

    return 0;

#endif
}




/******************************************************************
 *
 * Function   :  ConsoleIoThrottle
 *
 * Description:  Toggle throttling of the console output
 *
 *****************************************************************/
void
ConsoleIoThrottle(
    unsigned char bEnable
    )
{
    _Gbl_bThrottleOutput = bEnable;

    // Reset if disabled
    if (!bEnable)
    {
        _Gbl_LineCount      = 0;
        _Gbl_bPausePending  = FALSE;
        _Gbl_bOutputDisable = FALSE;
    }
}




/******************************************************************
 *
 * Function   :  ConsoleCursorPropertiesSet
 *
 * Description:  Sets cursor properties
 *
 *****************************************************************/
void ConsoleCursorPropertiesSet( int size )
{
#if defined(PLX_LINUX)

    // Not currently implemented for Linux

#elif defined(PLX_MSWINDOWS)

    CONSOLE_CURSOR_INFO cci;

    cci.dwSize = size;
    if (size == 0)
        cci.bVisible = FALSE;
    else
        cci.bVisible = TRUE;
    SetConsoleCursorInfo( GetStdHandle(STD_OUTPUT_HANDLE), &cci );

#elif defined(PLX_DOS)

    // Only 3 options available for DOS cursor
    if (size == 0)
        _setcursortype( _NOCURSOR );
    else if (size > 50)
        _setcursortype( _SOLIDCURSOR );
    else
        _setcursortype( _NORMALCURSOR );

#endif
}




/******************************************************************
 *
 * Function   :  ConsoleIoThrottleReset
 *
 * Description:  Resets console line count
 *
 *****************************************************************/
void
ConsoleIoThrottleReset(
    void
    )
{
    _Gbl_LineCount = 0;
}




/******************************************************************
 *
 * Function   :  ConsoleIoIncrementLine
 *
 * Description:  Increments the console output line count if enabled
 *
 *****************************************************************/
void
ConsoleIoIncrementLine(
    void
    )
{
    // Do nothing if throttling is disabled
    if (_Gbl_bThrottleOutput == FALSE)
        return;

    // Increment line count
    _Gbl_LineCount++;

    if (_Gbl_LineCount >= _Gbl_LineCountMax)
    {
        // Flag that next incoming line needs pause
        _Gbl_bPausePending = TRUE;
    }
}




/******************************************************************
 *
 * Function   :  ConsoleIoOutputDisable
 *
 * Description:  Toggle console output
 *
 *****************************************************************/
void
ConsoleIoOutputDisable(
    unsigned char bEnable
    )
{
    _Gbl_bOutputDisable = bEnable;
}




/*********************************************************************
 *
 * Function   :  Plx_fputs
 *
 * Description:  Emulates fputs() with support for throttling
 *
 ********************************************************************/
int
Plx_fputs(
    const char *string,
    FILE       *stream
    )
{
    char toggle;


    // Display pause if pending
    if (_Gbl_bPausePending)
    {
        _Gbl_bPausePending = FALSE;

        fputs(
            "-- More (Press any to continue, 'C' for continuous, or 'Q' to quit) --",
            stdout
            );

        // Get user input
        toggle = Cons_getch();

        // Clear 'More' message
        fputs(
            "\r                                                                      \r",
            stdout
            );

        // Switch to lowercase
        toggle = tolower( toggle );

        if (toggle == 'c')
            ConsoleIoThrottle( FALSE );         // Disable throttle output
        else if (toggle == 'q')
        {
            ConsoleIoOutputDisable( TRUE );     // Disable any further output
            return 0;
        }
        else
            ConsoleIoThrottleReset();           // Reset the line count
    }

    return fputs( string, stream );
}




/*********************************************************************
 *
 * Function   :  Plx_printf
 *
 * Description:  Outputs a formatted string
 *
 ********************************************************************/
int
Plx_printf(
    const char *format,
    ...
    )
{
    int      i;
    int      NumChars;
    char    *pNextLine;
    char     pOut[4000];
    char    *pChar;
    BOOLEAN  bNewLine;
    va_list  pArgs;


    // Exit if console output disabled
    if (_Gbl_bOutputDisable)
        return 0;

    // Do nothing for empty string
    if (*format == '\0')
        return 0;

    // Initialize the optional arguments pointer
    va_start(pArgs, format);

    // Build string to write
    NumChars = vsprintf(pOut, format, pArgs);

    // Terminate arguments pointer
    va_end(pArgs);

    // Start at beginning of string
    pChar     = pOut;
    pNextLine = pOut;
    bNewLine  = FALSE;

    // Set number of characters
    i = NumChars + 1;

    while (i)
    {
        // Check for new line
        if (*pChar == '\n')
        {
            bNewLine = TRUE;

            // Mark end of line
            *pChar = '\0';
        }

        // Display current line
        if (*pChar == '\0')
        {
            // Display current line
            Cons_fputs( pNextLine, stdout );

            // Halt if console output disabled
            if (_Gbl_bOutputDisable)
                goto _Exit_Plx_printf;

            if (bNewLine)
            {
                // Display newline character
                bNewLine = FALSE;
                Cons_putchar( '\n' );

                // Inform console of new line in case of pause
                ConsoleIoIncrementLine();

                // Increment to next line
                pNextLine = pChar + 1;

                // Check for end of string
                if (*pNextLine == '\0')
                    goto _Exit_Plx_printf;
            }
            else
            {
                // Reached end of string
                goto _Exit_Plx_printf;
            }
        }

        // Go to next character
        pChar++;

        // Decrement count
        i--;
    }

_Exit_Plx_printf:
    // Return number of characters printed
    return NumChars;
}




/*************************************************
 *
 *         Windows-specific functions
 *
 ************************************************/

#if defined(PLX_MSWINDOWS)

/******************************************************************
 *
 * Function   :  Plx_clrscr
 *
 * Description:  Clears the console window
 *
 *****************************************************************/
void
Plx_clrscr(
    void
    )
{
    HANDLE                     hConsole;
    COORD                      CoordScreen;
    DWORD                      cCharsWritten;
    DWORD                      dwWinSize;
    CONSOLE_SCREEN_BUFFER_INFO csbi;


    // Get handle to the console
    hConsole =
        GetStdHandle(
            STD_OUTPUT_HANDLE
            );

    // Get the current console information
    GetConsoleScreenBufferInfo(
        hConsole,
        &csbi
        );

    dwWinSize = csbi.dwSize.X * csbi.dwSize.Y;

    // Set screen coordinates
    CoordScreen.X = 0;
    CoordScreen.Y = 0;

    // Fill the entire screen with blanks
    FillConsoleOutputCharacter(
        hConsole,
        (TCHAR) ' ',
        dwWinSize,
        CoordScreen,
        &cCharsWritten
        );

    // Put the cursor at origin
    SetConsoleCursorPosition(
        hConsole,
        CoordScreen
        );
}

#endif // PLX_MSWINDOWS




/*************************************************
 *
 *         Linux-specific functions
 *
 ************************************************/

#if defined(PLX_LINUX)

/******************************************************************
 *
 * Function   :  Plx_kbhit
 *
 * Description:  Determines if input is pending
 *
 *****************************************************************/
int
Plx_kbhit(
    void
    )
{
    int            count;
    struct timeval tv;
    struct termios Tty_Save;
    struct termios Tty_New;


    // Get current terminal attributes
    tcgetattr( STDIN_FILENO, &Tty_Save );

    // Copy attributes
    Tty_New = Tty_Save;

    // Disable canonical mode (handles special characters)
    Tty_New.c_lflag &= ~ICANON;

    // Disable character echo
    Tty_New.c_lflag &= ~ECHO;

    // Set timeouts
    Tty_New.c_cc[VMIN]  = 1;   // Minimum chars to wait for
    Tty_New.c_cc[VTIME] = 1;   // Minimum wait time

    // Set new terminal attributes
    if (tcsetattr( STDIN_FILENO, TCSANOW, &Tty_New ) != 0)
        return 0;

    // Set to no characters pending
    count = 0;

    // Check stdin for pending characters
    if (ioctl( STDIN_FILENO, FIONREAD, &count ) != 0)
        return 0;

    // Restore old settings
    tcsetattr( STDIN_FILENO, TCSANOW, &Tty_Save );

    // Small delay needed to give up CPU slice & allow use in a tight loop
    tv.tv_sec  = 0;
    tv.tv_usec = 1;
    select(1, NULL, NULL, NULL, &tv);

    return count;
}




/******************************************************************
 *
 * Function   :  Plx_getch
 *
 * Description:  Gets a character from the keyboard (with blocking)
 *
 *****************************************************************/
int
Plx_getch(
    void
    )
{
    int            retval;
    char           ch;
    struct termios Tty_Save;
    struct termios Tty_New;


    // Make sure all output data is flushed
    fflush( stdout );

    // Get current terminal attributes
    tcgetattr( STDIN_FILENO, &Tty_Save );

    // Copy attributes
    Tty_New = Tty_Save;

    // Disable canonical mode (handles special characters)
    Tty_New.c_lflag &= ~ICANON;

    // Disable character echo
    Tty_New.c_lflag &= ~ECHO;

    // Set timeouts
    Tty_New.c_cc[VMIN]  = 1;   // Minimum chars to wait for
    Tty_New.c_cc[VTIME] = 1;   // Minimum wait time

    // Set new terminal attributes
    if (tcsetattr( STDIN_FILENO, TCSANOW, &Tty_New ) != 0)
        return 0;

    // Get a single character from stdin
    retval = read( STDIN_FILENO, &ch, 1 ); 

    // Restore old settings
    tcsetattr( STDIN_FILENO, TCSANOW, &Tty_Save );

    if (retval > 0)
        return (int)ch;

    return 0;
}

#endif // PLX_LINUX
