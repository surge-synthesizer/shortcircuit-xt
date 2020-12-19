#include "logfile.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

bool log_updated = false;
char log_entry[256];

void write_log(const char *text)
{
    log_updated = true;
    strcpy(log_entry, text);

    // open file
    /*FILE *f = fopen("c:\\shortcircuit.log","a+");

    char tmpbuf[128];
    _strtime( tmpbuf );

    // write timestamp
    fprintf(f,"[%s] ", tmpbuf);
    // write message
    fprintf(f,"%s\n",text);

    //fflush(f);
    //close file
    //fclose(f);*/
}

void write_log(int number)
{
    /*// open file
    //f = fopen(filename,"a+");
    FILE *f = fopen("c:\\shortcircuit.log","a+");

    char tmpbuf[128];
    _strtime( tmpbuf );

    // write timestamp
    fprintf(f,"[%s] ", tmpbuf);
    // write message
    fprintf(f,"%i\n",number);

    //fflush(f);
    //close file
    fclose(f);*/
}