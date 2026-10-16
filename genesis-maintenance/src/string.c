/***************************************************************************
 *  File: string.c                                                         *
 *  Usage: various functions for new NOTE system                           *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  Much of this code came from CircleMUD.                                 *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 *                                                                         *
 *  Revised for Merc 2.1 by Jason Dinkel.                                  *
 *  Revised for Envy 1.0 by Jason Dinkel.				   *
 ***************************************************************************/

/***************************************************************************
 *  God Wars Mud improvements copyright (C) 1994, 1995, 1996 by Richard    *
 *  Woolcock.  This mud is NOT to be copied in whole or in part, or to be  *
 *  run without the permission of Richard Woolcock.  Nobody else has       *
 *  permission to authorise the use of this code.                          *
 ***************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"

int format_buffer( char *dest, size_t capacity, const char *format, ... )
{
    va_list args;
    int result;

    if (dest == NULL || format == NULL || capacity == 0)
        return -1;

    va_start(args, format);
    result = vsnprintf(dest, capacity, format, args);
    va_end(args);

    if (result < 0)
        dest[0] = '\0';
    else
        dest[capacity - 1] = '\0';

    return result;
}

/* Plain ASCII class tables: 24 + 2 + 51 = 77 visible columns.
 * Input is static menu text, not player names or colour-coded prose. */
void class_menu_header(CHAR_DATA *ch, const char *title,
                       const char *left, const char *right)
{
    char buf[320];
    format_buffer(buf,sizeof(buf),"\n\r%s\n\r%-24.24s  %s\n\r"
        "------------------------  ---------------------------------------------------\n\r",
        title,left,right);
    stc(buf,ch);
}

void class_menu_row(CHAR_DATA *ch, const char *label, const char *detail)
{
    char buf[96], part[52];
    size_t length, cut;
    bool first = TRUE;
    if (!ch || !label || !detail) return;
    do {
        while (*detail && isspace((unsigned char)*detail)) ++detail;
        if (!*detail && !first) break;
        length = 0;
        while (detail[length] && detail[length]!='\r'
               && detail[length]!='\n' && length<51) ++length;
        cut = length;
        if (length==51 && detail[length]
            && !isspace((unsigned char)detail[length])) {
            while (cut && !isspace((unsigned char)detail[cut])) --cut;
            if (!cut) cut=length;
        }
        memcpy(part,detail,cut); part[cut]='\0';
        while(cut && isspace((unsigned char)part[cut-1])) part[--cut]='\0';
        format_buffer(buf,sizeof(buf),"%-24.24s  %s\n\r",first ? label : "",part);
        stc(buf,ch);
        /* Advance over the original slice, including trailing spaces. */
        detail += length==51 && detail[length]
                  && !isspace((unsigned char)detail[length]) && cut ? cut : length;
        first=FALSE;
    } while (*detail);
}

void class_menu_rank(CHAR_DATA *ch, const char *name, long rank)
{
    char value[32];
    format_buffer(value,sizeof(value),"%li",rank);
    class_menu_row(ch,name,value);
}

/* Preserve legacy rank gating and command strings; only wrap the display. */
void class_menu_commands(CHAR_DATA *ch, const char *text)
{
    const char *colon = strchr(text,':');
    char label[25];
    size_t length;
    if (!colon) { class_menu_row(ch,"",text); return; }
    length=(size_t)(colon-text);
    if (length>24) length=24;
    memcpy(label,text,length); label[length]='\0';
    class_menu_row(ch,label,colon+1);
}

void class_menu_skill_line(CHAR_DATA *ch, const char *text)
{
    const char *divider = strstr(text," - ");
    char label[25];
    size_t length;
    if (!divider) { class_menu_row(ch,"",text); return; }
    length=(size_t)(divider-text);
    while(length && isspace((unsigned char)text[length-1])) --length;
    if(length>24) length=24;
    memcpy(label,text,length); label[length]='\0';
    class_menu_row(ch,label,divider+3);
}

long class_training_cost(long rank, long cap, long base, bool squared)
{
    long next;
    if (rank<0 || rank>=cap || base<=0) return 0;
    next=rank+1;
    return squared ? base*next*next : base*next;
}

void class_training_header(CHAR_DATA *ch, const char *title)
{
    char buf[320];
    format_buffer(buf,sizeof(buf),"\n\r%s\n\r"
        "%-20s %8s %12s %-8s %s\n\r"
        "-------------------- -------- ------------ -------- ----------------\n\r",
        title,"Training","Rank/Cap","Next cost","Currency","Status");
    stc(buf,ch);
}

void class_training_row(CHAR_DATA *ch, const char *name, long rank, long cap,
                        long cost, const char *currency, long funds,
                        const char *blocked)
{
    char buf[128], ranks[48], price[32];
    const char *status;
    format_buffer(ranks,sizeof(ranks),"%li/%li",rank,cap);
    if (rank>=cap) {
        strcpy(price,"--"); status="MAXED";
    } else {
        format_buffer(price,sizeof(price),"%li",cost);
        status=blocked ? blocked : cost>funds ? "Needs funds" : "Affordable";
    }
    format_buffer(buf,sizeof(buf),"%-20.20s %8.8s %12.12s %-8.8s %-16.16s\n\r",
        name,ranks,price,currency,status);
    stc(buf,ch);
}



void string_edit( CHAR_DATA *ch, char **pString )
{
    send_to_char( "-=======================================-\n\r", ch );
    send_to_char( " Entering line editing mode.\n\r", ch );
    send_to_char( " Terminate with a ~ or @ on a blank line.\n\r", ch );
    send_to_char( "-=======================================-\n\r", ch );

    if ( *pString == NULL )
    {
        *pString = str_dup( "" );
    }
    else
    {
        **pString = '\0';
    }

    ch->desc->pString = pString;

    return;
}



void string_append( CHAR_DATA *ch, char **pString )
{
    send_to_char( "-=======- Entering APPEND Mode -========-\n\r", ch );
    send_to_char( "    Type .h on a new line for help\n\r", ch );
    send_to_char( " Terminate with a ~ or @ on a blank line.\n\r", ch );
    send_to_char( "-=======================================-\n\r", ch );

    if ( *pString == NULL )
    {
        *pString = str_dup( "" );
    }
    send_to_char( *pString, ch );
    
    if ( *(*pString + strlen( *pString ) - 1) != '\r' )
    send_to_char( "\n\r", ch );

    ch->desc->pString = pString;

    return;
}



char * string_replace( char * orig, char * old, char * new )
{
    char xbuf[MAX_STRING_LENGTH];
    long i;

    xbuf[0] = '\0';
    strcpy( xbuf, orig );
    if ( strstr( orig, old ) != NULL )
    {
        i = strlen( orig ) - strlen( strstr( orig, old ) );
        xbuf[i] = '\0';
        strcat( xbuf, new );
        strcat( xbuf, &orig[i+strlen( old )] );
        free_string( orig );
    }

    return str_dup( xbuf );
}



void string_add( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if ( *argument == '.' )
    {
        char arg1 [MAX_INPUT_LENGTH];
        char arg2 [MAX_INPUT_LENGTH];
        char arg3 [MAX_INPUT_LENGTH];

        argument = one_argument( argument, arg1 );
        argument = first_arg( argument, arg2, FALSE );
        argument = first_arg( argument, arg3, FALSE );

        if ( !str_cmp( arg1, ".c" ) )
        {
            send_to_char( "String cleared.\n\r", ch );
            **ch->desc->pString = '\0';
            return;
        }

        if ( !str_cmp( arg1, ".s" ) )
        {
            send_to_char( "String so far:\n\r", ch );
            send_to_char( *ch->desc->pString, ch );
            return;
        }

        if ( !str_cmp( arg1, ".r" ) )
        {
            if ( arg2[0] == '\0' )
            {
                send_to_char( "usage:  .r \"old string\" \"new string\"\n\r", ch );
                return;
            }

            *ch->desc->pString = string_replace( *ch->desc->pString, arg2, arg3 );
            sprintf( buf, "'%s' replaced with '%s'.\n\r", arg2, arg3 );
            send_to_char( buf, ch );
            return;
        }

        if ( !str_cmp( arg1, ".f" ) )
        {
            *ch->desc->pString = format_string( *ch->desc->pString );
            send_to_char( "String formatted.\n\r", ch );
            return;
        }
        
        if ( !str_cmp( arg1, ".h" ) )
        {
            send_to_char( 
"Sedit help (commands available from the beginning of a new line only):  \n\r", ch );
            send_to_char( 
".r 'old string' 'new string'   - replace a substring with a new string  \n\r", ch );
            send_to_char( 
"                                (requires '', \"\" around the arguments)\n\r", ch );
            send_to_char( 
".h                             - get help (this info)                   \n\r", ch );
            send_to_char( 
".s                             - show string so far                     \n\r", ch );
            send_to_char( 
".f                             - format (word wrap) string              \n\r", ch );
            send_to_char( 
".c                             - clear string so far                    \n\r", ch );
            send_to_char( 
"@  or  ~                       - end string editor (end string)         \n\r", ch );
            return;
        }
            

        send_to_char( "SEdit:  Invalid dot command.\n\r", ch );
        return;
    }

    strcpy( buf, *ch->desc->pString );

    if ( *argument == '~' || *argument == '@' )
    {
        ch->desc->pString = NULL;
        return;
    }

    if ( strlen( buf ) + strlen( argument ) >= ( MAX_STRING_LENGTH - 4 ) )
    {
        send_to_char( "String too long, truncating.\n\r", ch );
        strncat( buf, argument, MAX_STRING_LENGTH - strlen(buf) - 1 );
        free_string( *ch->desc->pString );
        *ch->desc->pString = str_dup( buf );
        ch->desc->pString = NULL;
        return;
    }

    strcat( buf, argument );
    strcat( buf, "\n\r" );
    free_string( *ch->desc->pString );
    *ch->desc->pString = str_dup( buf );
    return;
}



/*
 * Thanks to Kalgen for the new procedure (no more bug!)
 * Original wordwrap() written by Surreality.
 */
char *format_string( char *oldstring /*, bool fSpace */)
{
  char xbuf[MAX_STRING_LENGTH];
  char xbuf2[MAX_STRING_LENGTH];
  char *rdesc;
  long i=0;
  bool cap=TRUE;
  
  xbuf[0]=xbuf2[0]=0;
  
  i=0;
  
  for (rdesc = oldstring; *rdesc; rdesc++)
  {
    if (*rdesc=='\n')
    {
      if (xbuf[i-1] != ' ')
      {
        xbuf[i]=' ';
        i++;
      }
    }
    else if (*rdesc=='\r') ;
    else if (*rdesc==' ')
    {
      if (xbuf[i-1] != ' ')
      {
        xbuf[i]=' ';
        i++;
      }
    }
    else if (*rdesc==')')
    {
      if (xbuf[i-1]==' ' && xbuf[i-2]==' ' && 
          (xbuf[i-3]=='.' || xbuf[i-3]=='?' || xbuf[i-3]=='!'))
      {
        xbuf[i-2]=*rdesc;
        xbuf[i-1]=' ';
        xbuf[i]=' ';
        i++;
      }
      else
      {
        xbuf[i]=*rdesc;
        i++;
      }
    }
    else if (*rdesc=='.' || *rdesc=='?' || *rdesc=='!') {
      if (xbuf[i-1]==' ' && xbuf[i-2]==' ' && 
          (xbuf[i-3]=='.' || xbuf[i-3]=='?' || xbuf[i-3]=='!')) {
        xbuf[i-2]=*rdesc;
        if (*(rdesc+1) != '\"')
        {
          xbuf[i-1]=' ';
          xbuf[i]=' ';
          i++;
        }
        else
        {
          xbuf[i-1]='\"';
          xbuf[i]=' ';
          xbuf[i+1]=' ';
          i+=2;
          rdesc++;
        }
      }
      else
      {
        xbuf[i]=*rdesc;
        if (*(rdesc+1) != '\"')
        {
          xbuf[i+1]=' ';
          xbuf[i+2]=' ';
          i += 3;
        }
        else
        {
          xbuf[i+1]='\"';
          xbuf[i+2]=' ';
          xbuf[i+3]=' ';
          i += 4;
          rdesc++;
        }
      }
      cap = TRUE;
    }
    else
    {
      xbuf[i]=*rdesc;
      if ( cap )
        {
          cap = FALSE;
          xbuf[i] = UPPER( xbuf[i] );
        }
      i++;
    }
  }
  xbuf[i]=0;
  strcpy(xbuf2,xbuf);
  
  rdesc=xbuf2;
  
  xbuf[0]=0;
  
  for ( ; ; )
  {
    for (i=0; i<77; i++)
    {
      if (!*(rdesc+i)) break;
    }
    if (i<77)
    {
      break;
    }
    for (i=(xbuf[0]?76:73) ; i ; i--)
    {
      if (*(rdesc+i)==' ') break;
    }
    if (i)
    {
      *(rdesc+i)=0;
      strcat(xbuf,rdesc);
      strcat(xbuf,"\n\r");
      rdesc += i+1;
      while (*rdesc == ' ') rdesc++;
    }
    else
    {
      bug ("No spaces", 0);
      *(rdesc+75)=0;
      strcat(xbuf,rdesc);
      strcat(xbuf,"-\n\r");
      rdesc += 76;
    }
  }
  while (*(rdesc+i) && (*(rdesc+i)==' '||
                        *(rdesc+i)=='\n'||
                        *(rdesc+i)=='\r'))
    i--;
  *(rdesc+i+1)=0;
  strcat(xbuf,rdesc);
  if (xbuf[strlen(xbuf)-2] != '\n')
    strcat(xbuf,"\n\r");

  free_string(oldstring);
  return(str_dup(xbuf));
}



/*
 * Pick off one argument from a string and return the rest.
 * Understands quotes, parenthesis (barring ) ('s) and percentages.
 */
char *first_arg( char *argument, char *arg_first, bool fCase )
{
    char cEnd;

    while ( *argument == ' ' )
	argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"'
      || *argument == '%'  || *argument == '(' )
    {
        if ( *argument == '(' )
        {
            cEnd = ')';
            argument++;
        }
        else cEnd = *argument++;
    }

    while ( *argument != '\0' )
    {
	if ( *argument == cEnd )
	{
	    argument++;
	    break;
	}
    if ( fCase ) *arg_first = LOWER(*argument);
            else *arg_first = *argument;
	arg_first++;
	argument++;
    }
    *arg_first = '\0';

    while ( *argument == ' ' )
	argument++;

    return argument;
}



char * string_unpad( char * argument )
{
    char buf[MAX_STRING_LENGTH];
    char *s;

    s = argument;

    while ( *s == ' ' )
        s++;

    strcpy( buf, s );
    s = buf;

    if ( *s != '\0' )
    {
        while ( *s != '\0' )
            s++;
        s--;

        while( *s == ' ' )
            s--;
        s++;
        *s = '\0';
    }

    free_string( argument );
    return str_dup( buf );
}



char * string_proper( char * argument )
{
    char *s;

    s = argument;

    while ( *s != '\0' )
    {
        if ( *s != ' ' )
        {
            *s = UPPER(*s);
            while ( *s != ' ' && *s != '\0' )
                s++;
        }
        else
        {
            s++;
        }
    }

    return argument;
}



long arg_count( char *argument )
{
    long total;
    char *s;

    total = 0;
    s = argument;

    while ( *s != '\0' )
    {
        if ( *s != ' ' )
        {
            total++;
            while ( *s != ' ' && *s != '\0' )
                s++;
        }
        else
        {
            s++;
        }
    }

    return total;
}
