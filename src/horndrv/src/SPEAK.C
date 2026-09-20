/***************************************************************************
 *
 * SPEAK.C
 *
 * Description - This is the code brings together the english code and
 *               the phonetic code to allow speech.
 *
 * Author      - Jonathan Hornstein
 *
 * Change Log
 *
 * Date      Initials   Description
 * 22-02-93  JSH        Created today
 *
 ***************************************************************************/
#include <dos.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "english.h"
#include "phoneme.h"
#include "execdrv.h"

typedef struct _EXCEPTION
{
    char       chException,
         near *pszReplacement;
} EXCEPTION;
static EXCEPTION aExceptionTbl[] =
{
    {'0' , "ZERO"                },
    {'1' , "ONE"                 },
    {'2' , "TWO"                 },
    {'3' , "THREE"               },
    {'4' , "FOUR"                },
    {'5' , "FIVE"                },
    {'6' , "SIX"                 },
    {'7' , "SEVEN"               },
    {'8' , "EIGHT"               },
    {'9' , "NINE"                },
    {'+' , "PLUS"                },
    {'-' , "MYNUS"               },
    {'.' , "DOT"                 },
    {', ', "COMMA"               },
    {'\\', "BACK SLASH"          },
    {'/' , "FORWARD SLASH"       },
    {'#' , "HASH"                },
    {'@' , "AT"                  },
    {'!' , "EXCLAMATION"         },
    {'%' , "PER CENTAGE"         },
    {'&' , "AMPERSAND"           },
    {'(' , "LEFT BRACKET"        },
    {')' , "RIGHT BRACKET"       },
    {'[' , "LEFT SQUARE BRACKET" },
    {']' , "RIGHT SQUARE BRACKET"},
    {'{' , "LEFT CURLY BRACKET"  },
    {'}' , "RIGHT CURLY BRACKET" },
    {'<' , "LEFT ANGLE BRACKET"  },
    {'>' , "RIGHT ANGLE BRACKET" },
    {'*' , "ASTERISK"            },
    {'=' , "EQUALS"              },
    {':' , "COLON"               },
    {';' , "SEMI COLON"          },
    {'\"', "QUOTE"               },
    {'\'', "APOSTROPHE"          },
    {'$' , "DOLLAR"              },
    {'\?', "QUESTION MARK"       },
    {'_' , "UNDER SCORE"         },
    {'~' , "TILDE"               },
    {'^' , "CARET"               },
};
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
 * TalkPhoneme
 *
 * Description - This function translates the phoneme to sound by caling
 *               FindPhoneme to locate the phoneme and PlayPhoneme to sound
 *               it.
 *
 * Arguements  - pszPhoneme  Phoneme
 *
 * Returns     - None
 *
 ***************************************************************************/
void TalkPhoneme (void)
{
PHONEME *pPhoneme;
char    achPhonemeData[3],
        chTransferBuf;

    for(;!IsRingEmpty(&WriteRing);)
    {
        *achPhonemeData = '\0';
        for(;RemoveChRing(&WriteRing, &(unsigned char)chTransferBuf);)
        {
            if(!isalpha(chTransferBuf) && !isspace(chTransferBuf))
            {
                break;
            }
            strncat(achPhonemeData, &chTransferBuf, sizeof(chTransferBuf));
            if(strlen(achPhonemeData) == sizeof(achPhonemeData)-1)
            {
                break;
            }
        }
        pPhoneme = FindPhoneme(achPhonemeData);
        if(pPhoneme != NULL)
        {
            PlayPhoneme(pPhoneme);
        }
    }
}
/***************************************************************************
 * EnglishTokenToPhoneme
 *
 * Description - This function assembles a token. Converts the token to a phoneme,
 *               then passes the phoneme to the phoneme speaking code.
 *
 * Arguements  - char character
 *
 * Returns     - None
 *
 ***************************************************************************/
void EnglishTokenToPhoneme (const char chTransferBuf)
{
static char  achToken[50]={0};
EXCEPTION   *pExceptionTbl;
char        *pszReplacement;

    pExceptionTbl = SearchArray((void **)aExceptionTbl,
                                sizeof(aExceptionTbl[0]),
                                sizeof(aExceptionTbl)/sizeof(aExceptionTbl[0]),
                                MatchException,
                                (void *)&chTransferBuf);
    if(pExceptionTbl != NULL)
    {
        if(*achToken)
        {
            /* Analyse each token in turn and generate the phoneme string */
            ConvertTokenPhoneme(achToken);
            ConvertTokenPhoneme(" ");
            *achToken = '\0';
        }
        for(pszReplacement=pExceptionTbl->pszReplacement;*pszReplacement;)
        {
            EnglishTokenToPhoneme(*pszReplacement++);
        }
        EnglishTokenToPhoneme('\n');
    }
    else
    if(isspace(chTransferBuf))
    {
        /* Analyse each token in turn and generate the phoneme string */
        ConvertTokenPhoneme(achToken);
        ConvertTokenPhoneme(" ");
    }
    else
    if(isalpha(chTransferBuf))
    {
        strncat(achToken, &chTransferBuf, sizeof(chTransferBuf));
        if(strlen(achToken) < sizeof(achToken)-1)
        {
            return;
        }
        /* Analyse each token in turn and generate the phoneme string */
        ConvertTokenPhoneme(achToken);
    }
    *achToken = '\0';
}
#ifdef TEST
char achRingBuf[BUFSIZ];
/***************************************************************************
 * main
 *
 * Description - This function exercises the english to phoneme translator
 *               and pipes the output to stdout
 *
 * Arguements  - filename
 *
 * Returns     - None
 *
 ***************************************************************************/
void main(const int argc, const char **argv)
{

    if(argc == 1)
    {
    char chInputBuf;

         InitParseRingBuf(achRingBuf, sizeof(achRingBuf));
         while(fread(&chInputBuf, sizeof(chInputBuf), 1, stdin))
         {
            SpeakEnglish(chInputBuf);
         }
    }
    else
    {
        while(gets(achRingBuf))
        {
            TalkPhoneme(achRingBuf);
        }
    }
}
#endif
