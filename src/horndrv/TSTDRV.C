#include <dos.h>
#include <mem.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void main(int argc, char **argv)
{
char    achSay[BUFSIZ],
       *pszSay;
FILE   *pFile,
       *pPhonemFile;

    if(argc > 1)
    {
        if((pPhonemFile = fopen("$talk$", "r+")) != NULL)
        {
            /* Flush previous words */
            rewind(pPhonemFile);
            fgets(achSay, sizeof(achSay), pPhonemFile);
            /* Open the words file */
            if((pFile = fopen(argv[1], "r")) != NULL)
            {
                while(fgets(achSay, sizeof(achSay), pFile) != NULL)
                {
                    if(achSay[0] != '#') // If not a comment process
                    {
                        /* Get the first word  */
                        pszSay = strtok(achSay, " ");
                        printf("Word is [%s] ", pszSay);
                        /* And speak it */
                        fprintf(pPhonemFile, "%s\n", pszSay);
                        /* Must be rewind for this driver */
                        rewind(pPhonemFile);
                        /* Return the phoneme translation */
                        fgets(achSay, sizeof(achSay), pPhonemFile);
                        printf("phoneme is [%s]\n", achSay);
                    }
                }
                fclose(pFile);
            }
            fclose(pPhonemFile);
        }
    }
}