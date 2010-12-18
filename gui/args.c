#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <gui.h>

#include <getopt.h>

void help(char *name, int ret) 
{
        fprintf(stdout,"Usage: %s -r <rom> [OPTIONS]\n"
                        "\n\t--== TI Linux EMulator 2.00 ==--\n"
                        "\t    Tilem Is a Linux EMulator\n"
                        "\t--help\t\tshow this message\n"
                        "\t-r <rom>\trom to run\n"
                        "\t-f <file>\tfile to load\n"
                        "\t-k <skin>\tskin to display\n"
                        "\t-m <macro>\tmacro to run\n"
                        "\t-s <save>\tsave state to load\n"
                        "\t-l \t\tstart in skinless mode\n", name);

        exit(ret);
}


int getargs(int argc, char* argv[], GLOBAL_SKIN_INFOS* gsi) {
       	
       	char options;

	if (argc <= 2)
                help(argv[0],-1);
        else if (strcmp(argv[1], "--help") == 0)
                help(argv[0],0);

        while((options = getopt(argc,argv, "s:f:r:k:m:l")) != -1)
        {
                switch(options) //options -X de la ligne
                {		//de commande
			case 's':
                        printf("arg : s, optarg = %s\n", optarg);
			gsi->SavName = optarg;
                        printf("gsi->SavName = %s\n", gsi->SavName);
			break;
                        case 'r':
                        printf("arg : r, optarg = %s\n", optarg);
			gsi->RomName = optarg;
                        printf("gsi->RomName = %s\n", gsi->RomName);
                        break;
                        case 'f':
                        printf("arg : f, optarg = %s\n", optarg);
			gsi->FileToLoad = optarg;
                        printf("gsi->FileToLoad = %s\n", gsi->FileToLoad);
                        break;
                        case 'k':
                        printf("arg : k, optarg = %s\n", optarg);
			gsi->SkinFileName = optarg;
                        printf("gsi->SkinFileName = %s\n", gsi->SkinFileName);
                        break;
                        case 'm':
                        printf("arg : m, optarg = %s\n", optarg);
			gsi->MacroName = optarg;
                        printf("gsi->MacroName = %s\n", gsi->MacroName);
                        break;
			case 'l':
                        printf("arg : l\n");
			gsi->isStartingSkinless = TRUE;
                        break;
			
                        default :
                        fprintf(stderr,"Erreur d'option\n"); 
                        help(argv[0],-1);
                        break;
                                                                                                                   
                }
        }

return 0;
}

/* Create the savname */
void create_savname(GLOBAL_SKIN_INFOS* gsi) {

	char* p;
	
	if(gsi->RomName != NULL) 
		if(gsi->SavName == NULL) {
			
			gsi->SavName = g_malloc(strlen(gsi->RomName) + 5); /* sav/ (4 char) + romname + .sav (4 char) + \0 (1 char) */
			memset(gsi->SavName, 0 , strlen(gsi->RomName));
			strcat(gsi->SavName, gsi->RomName);
			
			if ((p = strrchr(gsi->SavName, '.'))) 
			{
				strcpy(p, ".sav");
				DGLOBAL_L0_A0("**************** fct : main ****************************\n");
				DGLOBAL_L0_A2("*  gsi->RomName=%s gsi->SavName=%s           *\n",gsi->RomName, gsi->SavName);	
				DGLOBAL_L0_A0("********************************************************\n");
			} else {
				strcat(gsi->SavName, ".sav");
		}
	}
	/* end */
}