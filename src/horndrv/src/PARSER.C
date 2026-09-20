#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "types.h"
#include "parser.h"
#include "english.h"
#include "ring.h"

typedef struct _EXCEPTION
{
    char  chException,
         *pszReplacement;
} EXCEPTION;

static char achTokenBuf[100];
static RING TokenRing;
static EXCEPTION aExceptionTbl[] =
{
    {'0',  "ZERO"               },
    {'1',  "ONE"                },
    {'2',  "TWO"                },
    {'3',  "THREE"              },
    {'4',  "FOUR"               },
    {'5',  "FIVE"               },
    {'6',  "SIX"                },
    {'7',  "SEVEN"              },
    {'8',  "EIGHT"              },
    {'9',  "NINE"               },
    {'+',  "PLUS"               },
    {'-',  "MYNUS"              },
    {'.',  "DOT"                },
    {', ', "COMMA"              },
    {'\\', "BACKSLASH"          },
    {'/',  "FORWARDSLASH"       },
    {'#',  "HASH"               },
    {'@',  "AT"                 },
    {'!',  "EXCLAMATION"        },
    {'%',  "PERCENTAGE"         },
    {'^',  "UPARROW"            },
    {'&',  "AMPERSAND"          },
    {'(',  "LEFTBRACKET"        },
    {')',  "RIGHTBRACKET"       },
    {'[',  "LEFTSQUAREBRACKET"  },
    {']',  "RIGHTSQUAREBRACKET" },
    {'{',  "LEFTCURLYBRACKET"   },
    {'}',  "RIGHTCURLYBRACKET"  },
    {'<',  "LEFTANGLEBRACKET"   },
    {'>',  "RIGHTANGLEBRACKET"  },
    {'*',  "STAR"               },
    {'=',  "EQUALS"             },
    {':',  "COLON"              },
    {';',  "SEMICOLON"          },
    {'\"', "QUOTE"              },
    {'\'', "APOSTROPHE"         },
    {'$',  "DOLLAR"             },
    {'\?', "QUESTIONMARK"       },
    {'_',  "UNDERSCORE"         },
};
/***************************************************************************
 * AddCharParseBuf
 *
 * Description - This function initalises the parse buffer and gets the
 *               first token.
 *
 * Arguements  - void
 *
 * Returns     - void
 *
 ***************************************************************************/
BOOL AddCharParseBuf (unsigned char uchInput)
{
    return(AddChRing(&TokenRing, toupper(uchInput)));
}
/***************************************************************************
 * RemoveCharParseBuf
 *
 * Description - This function initalises the parse buffer and gets the
 *               first token.
 *
 * Arguements  - void
 *
 * Returns     - void
 *
 ***************************************************************************/
BOOL RemoveCharParseBuf (unsigned char *puchInput)
{
    return(RemoveChRing(&TokenRing, puchInput));
}
/***************************************************************************
 * MatchException
 *
 * Description - This function is used to determine match condition for
 *               token replacement for single characters.
 *
 * Arguements  - const void *pException
 *               const void *pchMatch
 *
 * Returns     - BOOL match(0) no match (!= 0)
 *
 ***************************************************************************/
void InitParseRingBuf (unsigned char *achRingBuf, size_t uiRingBufSiz)
{
    InitialiseRing(&TokenRing, achRingBuf, uiRingBufSiz);
}
/***************************************************************************
 * MatchException
 *
 * Description - This function is used to determine match condition for
 *               token replacement for single characters.
 *
 * Arguements  - const void *pException
 *               const void *pchMatch
 *
 * Returns     - BOOL match(0) no match (!= 0)
 *
 ***************************************************************************/
static int MatchException (const void *pException, const void *pchMatch)
{
    return(((EXCEPTION *)pException)->chException != *(char *)pchMatch);
}
/***************************************************************************
 * NextToken
 *
 * Description - This function gets the next token from the token stream.
 *               If a token was found this function updates the global
 *				 buffer containing the token, and the current chParseBufNdx
 *               parse index pointer. The chParseBufNdx ALWAYS points to the
 *               next available character.
 *
 * Arguements  - void
 *
 * Returns     - Pointer to the token or NULL
 *
 ***************************************************************************/
char *NextToken (void)
{
unsigned char uchTmp;
EXCEPTION *pExceptionTbl;

    for(*achTokenBuf = '\0'; RemoveCharParseBuf(&uchTmp) != FALSE;)
    {
        if(isspace(uchTmp))
        {
            return(strcpy(achTokenBuf, " "));
        }
        if(isalpha(uchTmp))
        {
            if(strlen(strncat(achTokenBuf,
                              (const char *)&uchTmp,
                              sizeof(uchTmp))) >= sizeof(achTokenBuf)-1)
            {
                *achTokenBuf = '\0'; // Probably binary data file, clear it out
            }
            if(ReadChRing(&TokenRing, &uchTmp) != FALSE)
            {
                if(!isalpha(uchTmp))
                {
                    break;
                }
            }
        }
        else
        {
            pExceptionTbl = SearchArray((void **)aExceptionTbl,
                                        sizeof(aExceptionTbl[0]),
                                        sizeof(aExceptionTbl)/sizeof(aExceptionTbl[0]),
                                        MatchException,
                                        (void *)&uchTmp);
            if(pExceptionTbl != NULL)
            {
                return(strcpy(achTokenBuf, pExceptionTbl->pszReplacement));
            }
        }
    }
    return(*achTokenBuf != '\0'? achTokenBuf : NULL);
}
