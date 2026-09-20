/***************************************************************************
 *
 * ENGLISH.C
 *
 * Description - This is the code for the phonem device driver.
 *
 * Author      - Jonathan Hornstein
 *
 * Change Log
 *
 * Date      Initials   Description
 * 04-06-92  JSH        Created today
 *
 ***************************************************************************/
#include <dos.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "execdrv.h"

#define RULE_SIZ_D 20

typedef enum { MATCH_E, BEFORE_E, AFTER_E, PHONEME_E } RULES;

typedef struct _TOKEN_T
{
    const char *pszToken;
          char  chTokenNdx;
} TOKEN_T;
/*********************************************************************
     #   One or more vowels [AEIOUY]
     +   One of E, I, Y: a front vowel
     :   Zero or more consonants [BCDFGHJKLMNPQRSTVWXZ]
     ^   One consonant
     .   One of B, V, D, G, J, L, M, N, R, W, Z: a voiced consonant
     %   One of ER, E, ES, ED, ING, ELY: a suffix
     &   One of S, C, G, Z, X, J, CH, SH: a siblant
     @   One of T, S, R, D, L, Z, N, J, TH, CH, SH:
         a consonant influencing following u
*********************************************************************/
static const char *apszAlphaRules[] =
{
    " /// "                 ,

    "A// /UH"               ,          // STRING : before / after -> phoneme
    "ARE/ / /AH-R"			,
    "AR/ /O/UH-R"			,
    "AR//#/EH-R"			,
    "AS/ ^/#/AE-A-S"		,
    "A//WA/UH"				,
    "AW///AW"				,
    "ANY/ ://EH-N-EE"		,
    "A//^+#/AE-A"			,
    "ALLY/#://UH-L-EE"		,
    "AL/ /#/UH-L"			,
    "AGAIN///UH-G-EH-N"		,
    "AG/#:/E/IH-J"			,
    "A//^+:#/AE"			,
    "A/ :/^+/AE-A"			,
    "ARR/ //UH-R"			,
    "ARR///AE-R"			,
    "AR/ ://AH-R"			,
    "AR// /AE-R"			,
    "AR///AH-R"				,
    "AIR///EH-R"			,
    "AI///AE-A"				,
    "AY///AE-A"				,
    "AU///AW"				,
    "AL/#:/ /UH-L"			,
    "ALS/#:/ /UH-L-Z"		,
    "ALK///AW-K"			,
    "AL//^/AW-L"			,
    "ABLE/ ://AE-A-B-UH-L"	,
    "ABLE///UH-B-UH-L"		,
    "ANG//+/AE-A-N-J"		,
    "ATHE/ C/ /AE-TH-EE"	,
    "A//A/AH"				,
    "A///AE"	            ,

    "BE/ /^#/B-IH"		    ,
    "BEING///B-EE-IH-N"		,
    "BOTH/ / /B-OH-TH"		,
    "BUS/ /#/B-IH-Z"		,
    "BUIL///B-IH-L"			,
    "B/ / /B-EE"			,
    "B///B"		            ,

    "CH/ /^/K"              ,
    "CH/^E//K"				,
    "CH///CH"				,
    "CI/ S/#/S-AH-EE"		,
    "CI//A/SH"				,
    "CI//O/SH"				,
    "CI//EN/SH"				,
    "C//+/S"				,
    "CK///K"				,
    "COM//%/K-AH-M"			,
    "C/ / /S-EE"			,
    "C///K"				    ,

    "DED/#:/ /D-IH-D"       ,
    "D/.E/ /D"              ,
    "D/#^:E/ /T"            ,
    "DE/ /^#/D-IH"          ,
    "DO/ / /D-OO"           ,
    "DOES/ //D-UH-Z"        ,
    "DOING/ //D-OO-IH-N"    ,
    "DOW/ //D-OH"           ,
    "DU//A/J-OO"            ,
    "D/ / /D-EE"            ,
    "DOUGH///D-OH"          ,
    "D///D"                 ,

    "E/#:/ /"               ,
    "E/'^:/ /"              ,
    "E/ :/ /EE"             ,
    "ED/#/ /D"              ,
    "E/#:/D /"              ,
    "ER//EV/EH-V"           ,
    "EVEN/ EL//EH-V-EH-N"   ,
    "EVEN/ S//EH-V-EH-N"    ,
    "E//^%/EE"              ,
    "E//PH%/EE"             ,
    "ERI//#/EE-R-EE"        ,
    "ER/#:/#/AE-R"          ,
    "ER//#/EH-R"            ,
    "ER///AE-R"             ,
    "EVEN/ //EE-V-EH-N"		,
    "E/#:/W/"				,
    "EW/@//OO"				,
    "EW///Y-OO"				,
    "E//O/EE"				,
    "ES/#:&/ /IH-Z"			,
    "E/#:/S /"				,
    "ELY/#://L-EE"			,
    "EMENT/#://M-EH-N-T"	,
    "EFUL///F-U-L"			,
    "EE///EE"				,
    "EARN///AE-R-N"			,
    "EAR/ /^/AE-R"			,
    "EAD///EH-D"			,
    "EA/#:/ /EE-UH"			,
    "EA//SU/EH"				,
    "EA///EE"				,
    "EIGH///AE-A"			,
    "EI///EE"				,
    "EYE/ //AH-EE"			,
    "EY///EE"				,
    "EU///Y-OO"				,
    "E/ / /EE"				,
    "E/^/ /"				,
    "E///EH"	            ,

    "FUL///F-U-L"           ,
    "F/F//"                 ,
    "F/ / /EH-F"            ,
    "F///F"                 ,

    "GIV///G-IH-V"          ,
    "G/ /I^/G"              ,
    "GE//T/G-EH"            ,
    "GGES/SU//G-J-EH-SS"    ,
    "G/G//"                 ,
    "G/ B#//G"              ,
    "G//+/J"                ,
    "GREAT///G-R-AE-A-T"    ,
    "GH/#//"                ,
    "G/ / /G-EE"            ,
    "G///G"                 ,

    "HAV/ //H-AE-V"			,
    "HERE/ //H-EE-R"		,
    "HOUR/ //OH-AE-R"		,
    "HOW///H-OH"			,
    "H//#/H"				,
    "H/ / /H-AE-CH"			,
    "H///"	                ,

    "IN/ //IH-N"			,
    "I/ / /AH-EE"			,
    "IN//D/IH-N"			,
    "IER///EE-AE-R"			,
    "IED/#:R//EE-D"			,
    "IED// /AH-EE-D"		,
    "IEN///EE-EH-N"			,
    "IE//T/AH-EE-EH"		,
    "I/ :/%/AH-EE"			,
    "I//%/EE"			    ,
    "IE///EE"			    ,
    "INE/N//AH-EE-N"		,
    "IME/T//AH-EE-M"		,
    "I//^+:#/IH"			,
    "IR//#/AH-EE-R"			,
    "IS//%/AH-EE-S"			,
    "IX//%/IH-K-S"			,
    "IZ//%/AH-EE-Z"			,
    "I//D%/AH-EE"			,
    "I/+^/^+/IH"			,
    "I//T%/AH-EE"			,
    "I/#^:/^+/IH"			,
    "I//^+/AH-EE"			,
    "IR///AE-R"			    ,
    "IGH///AH-EE"			,
    "ILD///AH-EE-L-D"		,
    "IGN// /AH-EE-N"		,
    "IGN//^/AH-EE-N"		,
    "IGN//%/AH-EE-N"		,
    "IQUE///EE-K"			,
    "I///IH"			    ,

    "J/ / /J-A-EE"			,
    "J///J"					,


    "K//N/"					,
    "K/ / /K-A-EE"          ,
    "K///K"					,

    "LO//C#/L-OH"			,
    "L/L//"					,
    "L/#^:/%/UH-L"			,
    "LEAD///L-EE-D"			,
    "L/ / /AE-L"			,
    "L///L"					,

    "MOV///M-OO-V"			,
    "M/ / /EH-M"			,
    "M///M"					,

    "NG/E/+/N-J"			,
    "NG//R/N"				,
    "NG//#/N"				,
    "NGL//%/N-UH-L"			,
    "NG///N"				,
    "NK///N-K"				,
    "NOW/ / /N-OH"			,
    "N/ / /EH-N"			,
    "N/N//"					,
    "N///N"					,

    "OF// /UH-V"			,
    "OROUGH///AE-R-OH"		,
    "OR/ F/TY/OH-R"			,
    "OR/#:/ /AE-R"			,
    "ORS/#:/ /AE-R-Z"		,
    "OR///AW-R"			    ,
    "ONE/ //W-UH-N"			,
    "OW//EL/OH"			    ,
    "OW///OH"			    ,
    "OVER/ //OH-V-AE-R"		,
    "OV///UH-V"			    ,
    "O//^%/OH"			    ,
    "O//^EN/OH"			    ,
    "O//^I#/OH"			    ,
    "OL//D/OH-L"			,
    "OUGHT///AH-T"			,
    "OUGH///UH-F"			,
    "OU/ /^L/UH"			,
    "OU/ //OH"			    ,
    "OU/H/S#/OH"			,
    "OUS///UH-S"			,
    "OUR/ F//OH-R"			,
    "OUR///AW-R"			,
    "OUD///U-D"			    ,
    "OUP///OO-P"			,
    "OU///OH"			    ,
    "OY///AW-EE"			,
    "OING///OH-IH-N"		,
    "OI///AW-EE"			,
    "OOR///OH-R"			,
    "OOK///U-K"			    ,
    "OOD///U-D"			    ,
    "OO///OO"			    ,
    "O//E/OH"			    ,
    "O// /OH"			    ,
    "OA// /OH"			    ,
    "ONLY/ //OH-N-L-EE"		,
    "ONCE/ //W-UH-N-S"		,
    "ON'T// /OH-N-T"		,
    "O/C/N/AH"			    ,
    "O//NG/AH"			    ,
    "O/^:/N/UH"			    ,
    "ON/I//UH-N"			,
    "ON/#:/ /UH-N"			,
    "ON/#^//UH-N"			,
    "O//ST /OH"			    ,
    "OF//^/AW-F"			,
    "OTHER///UH-TH-AE-R"	,
    "OSS// /AW-S"			,
    "OM/#^:/ /UH-M"			,
    "O///AH"			    ,

    "PH///F"				,
    "PEOP///P-EE-P"			,
    "POW///P-OH"			,
    "PUT// /P-U-T"			,
    "P/ / /P-EE"			,
    "P/P//"					,
    "P///P"					,

    "QUAR///K-W-AW-R"		,
    "QU/ //K-W"				,
    "QU///K"				,
    "Q/ / /K-OO"            ,
    "Q///K"					,

    "RE/ /^#/R-EE"			,
    "R/ / /AH"				,
    "R/R//"					,
    "R///R"					,

    "SH///SH"				,
    "SION/#//ZH-UH-N"		,
    "SOME///S-AH-M"			,
    "SUR/#/#/ZH-AE-R"		,
    "SUR//#/SH-AE-R"		,
    "SU/#/#/ZH-OO"			,
    "SSU/#/#/SH-OO"			,
    "SED/#/ /Z-D"			,
    "S/#/#/Z"				,
    "SAID///S-EH-D"			,
    "SION/^//SH-UH-N"		,
    "S/S//"					,
    "S/./ /Z"				,
    "S/#:.E/ /Z"			,
    "S/#^:##/ /Z"			,
    "S/#^:#/ /S"			,
    "S/U/ /S"				,
    "S/ :#/ /Z"				,
    "SCH/ //S-K"			,
    "S//C+/"				,
    "SM/#//Z-M"				,
    "SN/#/ /Z-UH-N"			,
    "S/ / /EH-S"			,
    "S///S"					,

    "THE/ / /TH-UH"			,
    "TO// /T-OO"			,
    "THAT///TH-AE-T"		,
    "THIS/ / /TH-IH-S"		,
    "THEY/ //TH-AE-A"		,
    "THERE/ //TH-EH-R"	    ,
    "THER///TH-AE-R"		,
    "THEIR///TH-EH-EH"		,
    "THAN/ / /TH-AE-N"		,
    "THEM/ / /TH-EH-M"		,
    "THESE// /TH-EE-Z"		,
    "THEN/ //TH-EH-N"		,
    "THROUGH///TH-R-OO"		,
    "THOSE///TH-OH-Z"		,
    "THOUGH// /TH-OH"		,
    "THUS/ //TH-UH-S"		,
    "TH///TH"				,
    "TED/#:/ /T-IH-D"		,
    "TI/S/#N/CH"			,
    "TI//O/SH"				,
    "TI//A/T"				,
    "TIEN///SH-UH-N"		,
    "TUR//#/CH-AE-R"		,
    "TU//A/CH-OO"			,
    "TWO/ //T-OO"			,
    "T/ / /T-EE"			,
    "T/T//"					,
    "T///T"					,

    "UN/ /I/Y-OO-N"			,
    "UN/ //UH-N"			,
    "UPON/ //UH-P-AW-N"		,
    "UR/@/#/AE-R"			,
    "UR//#/Y-AE-R"			,
    "UR///AE-R"				,
    "U//^ /UH"				,
    "U//^^/UH"				,
    "UY///AH-EE"			,
    "U/ G/#/"				,
    "U/G/%/"				,
    "U/G/#/W"				,
    "U/#N//Y-OO"			,
    "UI/@//OO"				,
    "U/@//UH"				,
    "U///Y-OO"				,

    "VIEW///V-Y-OO"			,
    "V/ / /V-EE"			,
    "V///V"					,

    "WHERE/ //W-AE-R"		,
    "WA//S/W-AH"			,
    "WA//T/W-AH"			,
    "WHERE///WH-EH-R"		,
    "WHAT///WH-AH-T"		,
    "WHOL///H-OH-L"			,
    "WHO///H-OO"			,
    "WH///WH"				,
    "WAR///W-AH-R"			,
    "WOR///W-AE-R"			,
    "WR///R"				,
    "W/ / /D-AH-B-L-Y-OO"	,
    "W///W"					,

    "X//^/EH-K-S"			,
    "X/ / /EH-K-S"			,
    "X/ /#/Z-EH"			,
    "X///K-S"				,

    "YOUNG///Y-UH-N"		,
    "YOU/ //Y-OO"			,
    "YES/ //Y-EH-S"			,
    "Y/ / /WH-UH-Y"			,
    "Y/ //Y"				,
    "Y/#^:/ /EE"			,
    "Y/#^:/I/EE"			,
    "Y/ :/ /AH-EE"			,
    "Y/ :/#/AH-EE"			,
    "Y/ :/^+:#/IH"			,
    "Y/ :/^#/AH-EE"			,
    "Y///IH"				,

    "ZZ///T-Z"				,
    "Z/ / /Z-EH-D"			,
    "Z///Z"					,
};
static const char *apszSuffixes[] =
{
    "ER",
    "ES",
    "ED",
    "ING",
    "ELY",
};
static const char *apszSiblants[] =
{
    "CH",
    "SH",
};
static const char *apszSubsequentConst[] =
{
    "TH",
    "CH",
    "SH",
};
/***************************************************************************
 * MatchPreToken
 *
 * Description - This function gives the search criterion for the siblant
 *
 * Arguements  -
 *
 * Returns     - Matching element or NULL
 *
 ***************************************************************************/
static int MatchPreToken (const void *pszSiblant, const void *pToken)
{
char *pszToken,
      chTokenNdx;

    pszToken   = (char *)((TOKEN_T *)pToken)->pszToken;
    chTokenNdx = ((TOKEN_T *)pToken)->chTokenNdx;
    if((int)strlen(*(char **)pszSiblant)-1 > chTokenNdx)
    {
        return(1);
    }
    return(memicmp(*(char **)pszSiblant, pszToken, strlen(*(char **)pszSiblant)));
}
/***************************************************************************
 * MatchSuffixes
 *
 * Description - This function gives the search criterion for the siblant
 *
 * Arguements  -
 *
 * Returns     - Matching element or NULL
 *
 ***************************************************************************/
static int MatchSuffixes (const void *pszSuffix, const void *pszToken)
{
    return(memicmp(*(char **)pszSuffix, pszToken, strlen(*(char **)pszSuffix)));
}
/***************************************************************************
 * SearchArray
 *
 * Description - This function searches fixed format array and returns a match
 *
 * Arguements  - aUser             Table to scan
 *               uiElemSiz         size of an element in the table
 *               uiNoOfElem        Size of array
 *               pfnCompare        comparison function
 *               pszElem           pointer to the element to match
 *
 * Returns     - Matching element or NULL
 *
 ***************************************************************************/
void *SearchArray (const void   **aUser,
                   const size_t   uiElemSiz,
                         size_t   uiNoOfElem,
                         int    (*pfnCompare)(const void *, const void *),
                   const void    *pszElem)
{
    for(;uiNoOfElem--; ((char *)aUser) += uiElemSiz)
    {
        if((*pfnCompare)(aUser, pszElem) == 0)
        {
            return(aUser);
        }
    }
    return(NULL);
}
/***************************************************************************
 * AnalyseAlphaRules
 *
 * Description - This function will return in a static buffer the requested
 *               part of the phoneme array. Remember the parts are separated
 *               by / characters.
 *
 * Arguements  - RULES eRule                  Which rule is required
 *               char  *pszPhonemeMatch       Rule line
 *
 * Returns     - Phoneme string
 *
 ***************************************************************************/
static char *AnalyseAlphaRules(const RULES eRule, const char *pszPhonemeMatch)
{
static char achRule[RULE_SIZ_D];
char        chRuleNdx,
            chNext;
RULES       eState;

    chRuleNdx = 0;
    *achRule = '\0';
    chNext = pszPhonemeMatch[chRuleNdx++];
    /*
     * Search for the rule you want and put it into achRule buffer
     */
    for(eState = MATCH_E; chNext != (char)'\0';)
    {
        if(chNext == (char)'/')
        {
            if(eRule < ++eState)
            {
                break;
            }
            chNext = pszPhonemeMatch[chRuleNdx++];
            continue;
        }
        if(eRule == eState)
        {
            strncat(achRule, &chNext, sizeof(chNext));
            if(strlen(achRule) == sizeof(achRule)-1)
            {
                break;
            }
        }
        chNext = pszPhonemeMatch[chRuleNdx++];
    }
    return(achRule);
}
/***************************************************************************
 * ParseTokenWithRegExpression
 *
 * Description - This function will recursively parse the token to find a match
 *               for the rule expression.
 *
 * Arguements  - RULES eRule
 *               char *pszRule
 *               char  chRuleNdx
 *               char *pszToken
 *               char  chTokenNdx
 *
 * Returns     - BOOL 1 (OK),  0 (BAD)
 *
 ***************************************************************************/
static int ParseTokenWithRegExpression (RULES        eRule,
                                        char        *pszRule,
                                        signed char  chRuleNdx,
                                        char        *pszToken,
                                        signed char  chTokenNdx)
{
char  chInc,
     *pszTest;

    switch(eRule)
    {
        case AFTER_E:
            chInc = 1;
            /*
             * Check the rules for completion
             */
            if((size_t)chRuleNdx > strlen(pszRule))
            {
                return(1);
            }
            break;

        case BEFORE_E:
            chInc = -1;
            /*
             * Check the rules for completion
             */
            if(chRuleNdx < 0)
            {
                return(1);
            }
            break;

        default:
            return(0);
    }

    if(isalpha((int)pszRule[chRuleNdx]))
    {
        if(chTokenNdx < 0 || (size_t)chTokenNdx > strlen(pszToken)-1)
        {
            return(0);
        }
        if(pszToken[chTokenNdx] != pszRule[chRuleNdx])
        {
            return(0);
        }
        chTokenNdx+=chInc;
        chRuleNdx+=chInc;
    }
    else
    {
        /*******************************************************************
         #   One or more vowels [AEIOUY]
         +   One of E, I, Y: a front vowel
         :   Zero or more consonants [BCDFGHJKLMNPQRSTVWXZ]
         ^   One consonant
         .   One of B, V, D, G, J, L, M, N, R, W, Z: a voiced consonant
         %   One of ER, E, ES, ED, ING, ELY: a suffix
         &   One of S, C, G, Z, X, J, CH, SH: a siblant
         @   One of T, S, R, D, L, Z, N, J, TH, CH, SH:

             a consonant influencing following u
        ********************************************************************/
        switch((int)pszRule[chRuleNdx])
        {
            case ' ':
                chRuleNdx+=chInc;
                if(eRule == AFTER_E)
                {
                    if(chTokenNdx <= strlen(pszToken)-1)
                    {
                        return(0);
                    }
                }
                else
                {
                    if(chTokenNdx >= 0)
                    {
                        return(0);
                    }
                }
                chTokenNdx+=chInc;
                break;

            case '#':
                if(chTokenNdx < 0 || chTokenNdx > strlen(pszToken)-1)
                {
                    return(0);
                }
                pszTest = "AEIOUY";
                if(strchr(pszTest, pszToken[chTokenNdx]) == NULL)
                {
                    return(0);
                }
                if(ParseTokenWithRegExpression(eRule,
                                               pszRule,
                                               chRuleNdx,
                                               pszToken,
                                               chTokenNdx+chInc))
                {
                    return(1);
                }
                chRuleNdx+=chInc;
                chTokenNdx+=chInc;
                break;

            case '+':
                if(chTokenNdx < 0 || (size_t)chTokenNdx > strlen(pszToken)-1)
                {
                    return(0);
                }
                pszTest = "EIY";
                if(strchr(pszTest, pszToken[chTokenNdx]) == NULL)
                {
                    return(0);
                }
                chRuleNdx+=chInc;
                chTokenNdx+=chInc;
                break;

            case ':':
                if(chTokenNdx < 0 || (size_t)chTokenNdx > strlen(pszToken)-1)
                {
                    return(0);
                }
                pszTest = "BCDFGHJKLMNPQRSTVWXZ";
                if(strchr(pszTest, pszToken[chTokenNdx]) != NULL)
                {
                    if(ParseTokenWithRegExpression(eRule,
                                                   pszRule,
                                                   chRuleNdx,
                                                   pszToken,
                                                   chTokenNdx+chInc))
                    {
                        return(1);
                    }
                    chTokenNdx+=chInc;
                }
                chRuleNdx+=chInc;
                break;

            case '^':
                if(chTokenNdx < 0 || (size_t)chTokenNdx > strlen(pszToken)-1)
                {
                    return(0);
                }
                pszTest = "BCDFGHJKLMNPQRSTVWXZ";
                if(strchr(pszTest, pszToken[chTokenNdx]) == NULL)
                {
                    return(0);
                }
                chRuleNdx+=chInc;
                chTokenNdx+=chInc;
                break;

            case '.':
                if(chTokenNdx < 0 || (size_t)chTokenNdx > strlen(pszToken)-1)
                {
                    return(0);
                }
                pszTest = "BVDGJLMNRWZ";
                if(strchr(pszTest, pszToken[chTokenNdx]) == NULL)
                {
                    return(0);
                }
                chRuleNdx+=chInc;
                chTokenNdx+=chInc;
                break;

            case '%':
                if(chTokenNdx < 0 || (size_t)chTokenNdx > strlen(pszToken)-1)
                {
                    return(0);
                }
                pszTest = "E";
                if(strchr(pszTest, pszToken[chTokenNdx]) == NULL &&
                   SearchArray((void **)apszSuffixes,
                               sizeof(apszSuffixes[0]),
                               sizeof(apszSuffixes)/sizeof(apszSuffixes[0]),
                               MatchSuffixes,
                               (void *)&pszToken[chTokenNdx]) == NULL)
                {
                    return(0);
                }
                chRuleNdx+=chInc;
                chTokenNdx+=chInc;
                break;

            case '&':
                if(chTokenNdx < 0 || (size_t)chTokenNdx > strlen(pszToken)-1)
                {
                    return(0);
                }
                pszTest = "SCGZXJ";
                if(strchr(pszTest, pszToken[chTokenNdx]) == NULL)
                {
                TOKEN_T Token;

                    Token.chTokenNdx = chTokenNdx;
                    Token.pszToken   = pszToken;

                    if(SearchArray((void **)apszSiblants,
                               sizeof(apszSiblants[0]),
                               sizeof(apszSiblants)/sizeof(apszSiblants[0]),
                               MatchPreToken,
                               (void *)&Token) == NULL)
                    {
                        return(0);
                    }
                }
                chRuleNdx+=chInc;
                chTokenNdx+=chInc;
                break;

            case '@':
                if(chTokenNdx < 0 || (size_t)chTokenNdx > strlen(pszToken)-1)
                {
                    return(0);
                }
                pszTest = "TSRDLZNJ";
                if(strchr(pszTest, pszToken[chTokenNdx]) == NULL)
                {
                TOKEN_T Token;

                    Token.chTokenNdx = chTokenNdx;
                    Token.pszToken   = pszToken;

                    if(SearchArray((void **)apszSubsequentConst,
                                   sizeof(apszSubsequentConst[0]),
                                   sizeof(apszSubsequentConst)/sizeof(apszSubsequentConst[0]),
                                   MatchPreToken,
                                   (void *)&Token) == NULL)
                    {
                        return(0);
                    }
                }
                chRuleNdx+=chInc;
                chTokenNdx+=chInc;
                break;

            default:
                chRuleNdx++;
                break;
        }
    }
    return(ParseTokenWithRegExpression(eRule,
                                       pszRule,
                                       chRuleNdx,
                                       pszToken,
                                       chTokenNdx));
}
/***************************************************************************
 * MatchPhoneme
 *
 * Description - This function gets a phoneme within a token. This function
 *               will ALWAYS make a match
 *
 * Arguements  - char * pszToken
 *               pointer to where we're currently parsing
 *
 * Returns     - void
 *
 ***************************************************************************/
static int MatchPhoneme (const void *pszPhonemeArray, const void *pToken)
{
char  achMatch[RULE_SIZ_D],
      achRule[RULE_SIZ_D],
     *pszToken,
      chTokenNdx;

    strcpy(achMatch, AnalyseAlphaRules(MATCH_E , *(char **)pszPhonemeArray));

    pszToken   = (char *)((TOKEN_T *)pToken)->pszToken;
    chTokenNdx = ((TOKEN_T *)pToken)->chTokenNdx;
    /* The match is Ok */
    if(!memicmp(&pszToken[chTokenNdx], achMatch, strlen(achMatch)))
    {
        strcpy(achRule, AnalyseAlphaRules(BEFORE_E, *(char **)pszPhonemeArray));
        /* Check the before condition */
        if(ParseTokenWithRegExpression(BEFORE_E,
                                       achRule,
                                       strlen(achRule)-1,
                                       pszToken,
                                       chTokenNdx-1))
        {
            strcpy(achRule, AnalyseAlphaRules(AFTER_E, *(char **)pszPhonemeArray));
            /* Check after condition */
            if(ParseTokenWithRegExpression(AFTER_E,
                                           achRule,
                                           0,
                                           pszToken,
                                           chTokenNdx+strlen(achMatch)))
            {
                return(0);  /* match found */
            }
        }
    }
    return(1);
}
/***************************************************************************
 * ConvertTokenPhoneme
 *
 * Description - This function gets a phoneme within a token. This function
 *               will ALWAYS make a match
 *
 * Arguements  - char * pszToken
 *               pointer to where we're currently parsing
 *
 * Returns     - void
 *
 ***************************************************************************/
void ConvertTokenPhoneme (const char *pszToken)
{
char   **pszPhonemeMatch,
        *pszPhoneme;
TOKEN_T  Token;

    Token.chTokenNdx = 0;
    Token.pszToken   = pszToken;
    while(Token.chTokenNdx < strlen(pszToken))
    {
        pszPhonemeMatch = SearchArray((void **)apszAlphaRules,
                                      sizeof(apszAlphaRules[0]),
                                      sizeof(apszAlphaRules)/sizeof(apszAlphaRules[0]),
                                      MatchPhoneme,
                                      (void *)&Token);
        if(pszPhonemeMatch == NULL) // No match so exit
        {
            break;
        }
        pszPhoneme = AnalyseAlphaRules(PHONEME_E, *pszPhonemeMatch);
        PhonemeWrite(pszPhoneme);
        PhonemeWrite("-");
        Token.chTokenNdx += strlen(AnalyseAlphaRules(MATCH_E, *pszPhonemeMatch));
    }
    nosound(); // Reset the sound hardware to give maximum volume
}
#ifdef TESTMAIN
#include <stdio.h>
#include <ctype.h>
void PhonemeWrite (const char *pszToken)
{
static int iBuflen=0;

    printf("%s", pszToken);
    iBuflen += strlen(pszToken);
    if(iBuflen > 50)
    {
        iBuflen = 0;
        printf("\n");
    }
}
#pragma argsused
void main (int argc, char **argv)
{
    ConvertTokenPhoneme(strupr(argv[1]));
}
#endif