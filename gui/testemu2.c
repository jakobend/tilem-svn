#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>

//#include <tilem.h>

#include <gui.h>
//#include <skinops.h>

/***** TESTING *****/
#include <gdk/gdkkeysyms.h>
#include <scancodes.h>

/*  contra-sh : 
* ---18/08/09---
* - Draw the TI83 
* - Copy from testemu.c : Create the savname, Open the Rom, Get the model, Draw the lcdscreen (no animation for the moment)
* - Function event OnDestroy 
* - Modification of the Makefile (I hope it's good !? If you can control this... thx)
* - LEARNING HOW COMMIT !!   :D
* ---19/08/09---
* - New structure TilemCalcSkin : define the different filenames for the SkinSet (4 pieces).
* - Draw the other models _automatically_ . ;D
* ---20/08/09---
* - Create skin.c
* - Create gui.h (equivalent of tilem.h for the gui directory)
* - Move the code struct in gui.h and TilemCalcSkin* tilem_guess_skin_set(TilemCalc* calc) into skin.c (only one call in the main file to define the skin set) ;D
* - Detect a keyboard event (function keyboard_event() in skin.c actually).No treatment.
* - Detect an event click on mouse
* ---21/08/09---
* - Get the x and y values when click on mouse (now it will be easy to know how key is click on the pixmap, todo in priority : detect right click)
* ---24/08/09---
* - Detect right click.
* ---26/08/09---
* - New function : TilemKeyMap* tilem_guess_key_map(int id).Choose a TilemKeyMap with an id given in parameter. 
* ---27/08/09---
* - Extract the choice of the key_map from the mouse_event function.Execute only one time and it's more properly (and it will be easier to add the possibility to manage many skins and many keymaps).
* ---01/09/09---
* - Choose automatically the key_list. The TilemKeyList is already included in the TilemKeyMap structure...
* - New structure TilemKeyCoord (old TilemKeyMap).TilemKeyMap already contains TilemKeyCoord and TilemKeyList... 
* ---08/09/09---
* - New function tilem_set_coord to change the keypad coord.
* - New file event.c to group the GDKevent handling.
* - New function tilem_set_skin to change the skin.
* ---10/09/09---
* - Add the right click menu :D (0 signal connected without OnDestroy).Largely inspired from Tilem 0.973 and http://www.linux-france.org/article/devl/gtk/gtk_tut-11.html was a great inspiration too. 
* ---21/09/09---
* - Aouch I had seen that the left click doesn't work now! Problem : two callback for the only one button_press_event. (sorry for this version...)
* ---22/09/09---
* - Correction : only one callback now. mouse_event contains the create_menu and the gtk_menu_popup. (lot of time was spent on this problem)*
* ---23/09/09---
* - Change TilemKeyCoord to the 82 stats skin.Change Event.c too.
* ---06/10/09---
* - Beginning the correction : change the method for testing coordinates clicks (one line now) . The coordinates will be imported from a file soon.
* ---20/11/09---
* - After 1 week to learn Tiemu skin format...I have made my own Tilem skin Generator.Inspired from skinedit.It just generate compatible file with 0 coordinates values, and an default lcd coordinates.Not really necessary but very useful for testing and for learning how this f****** skin format works.It will be called skintool for the moment.
* ---27/11/09---
* - After blocking a problem for a week grrr... I succeed to adapt the TiEmu skinloader to TilEm (skinops.c and skinops.h).Little modifications
* ---28/11/09---
* - The mouse_event function now use a SKIN_INFOS struct. So delete TilemKeyCoord struct.
* ---02/12/09---
* - Add a GtkFileSelection (access by right click menu) and try to load another skin with this method.
* ---03/12/09---
* - Create a GLOBAL_SKIN_INFOS that contains a KEY_LIST struct (old TilemKeyList) and a SKIN_INFOS struct.
* ----04/12/09---
* - Delete the TilemKeyCoord, TilemKeyMap, TilemCalcSkin and TilemKeyList structs...
* ---05/12/09---
* - The GtkWidget *pWindow creation was moved from testemu2.c to event.c .The function is called GtkWidget* Draw_Screen(GLOBAL_SKIN_INFOS *gsi);
* ---07/12/09---
* - New feature : TilEm could load a skin by the right_click_menu ! It use GtkWidget* ReDraw_Screen(GLOBAL_SKIN_INFOS *gsi) in event.c. WAOUH !
* ---08/12/09---
* - Move Draw_Screen, ReDraw_Screen and create_menus in a new screen.c file. Change Makefile.
* ---14/12/09---
* - New feature : add a popup rom type selector (radio button) at the start of tilem. Showed but not used for the moment.
* - Connect the thread (no animation yet). Thanks to the "battle programming" 's spirit (hey bob ^^)
* ---18/12/09---
* - Launch the interactivity with emulation core. Could print into the draw area.
* ---09/03/10---
* - Restart working on this program ;D
* ---11/03/10---
* - I finally succeeded to connect the core. To print something into the lcd screen ! WahoOO ! This day is a great day !
* - I succeded to type numbers etc...
* - And now it works very well !! (the "button-release-event" is not catched by pWindow but by pLayout)
* ---15/03/10---
* - Create the scan_click function.Return wich keys was clicked.Print debug infos.
* - Create the debuginfos.h to group the ifdef for debugging. (different level and different type of debug msg)
* ---17/03/10---
* - Create the rom_choose_popup function to replace choose_rom_type.It use GtkDialog instead of GtkWindow.
* - rom_choose_popup _freeze_ the system... and get wich radio button is selected. So it will be easy to create the good emu.calc (and choose the default skin).
* ---18/03/10---
* - Resize the (printed) lcd area (gsi->emu->lcdwin) to fit(perfectly) into the skin.
* - Replace a lot of printf by D****_L*_A* to easily switch what debug infos were printed.
* - Try to make a nice debugging output (frames in ASCII ^^) :p
* - WahooOO , load a skin works perfectly.You can easily change skin _while running_, no error, no warning.
* - Could load automatically the good skin and run the good core using the choose_rom_popup() function and choose_skin_filename() function.
* ---30/03/10---
* - Create skin for following models : 73, 81, 82, 83+ and 84+.
* - Fix bug in tool.c .Modification of tool.c to create radio button more properly.
* ---31/03/10---
* - Create skin for following model : 83 . Based on my own calc (take a foto with my iphone 3GS :D)
* ---01/04/10---
* - New feature : Save calc state and load state. State are stored in a separate dir called sav/ . (using benjamin 's function)
* - New feature : Change view to see only the lcd. I finally choose to add it into a GtkLayout. So you can maximize it, but there was problem with add_event.
* ---02/04/10---
* - Add popup function to just print error message.You must give the message and gsi as parameter, and it run a modal dialog.
* - Some cleaning.
* ---23/04/10---
* - Add config.c file to manage config.dat (create, read, modif etc...).
* ---31/05/10---
* - Start from scratch a totally new debugger :D.Just draw button with nice GtkImages.Actually in essai2 directory.
* - Get and resize pixmaps (png) to 36 * 36 pixels for the debugger.
* ---01/06/10---
* - Add the debugger to tilem. Load registers values.
* - Add a new feature : switch the top level window "borderless".It can be switch by clicking on right click menu entry.
* ---02/06/10---
* - Create the GtkTreeView (debugger).
* - Refresh register on click.
* ---05/08/10---
* - More than one month without working on tilem...Only commit old modif, and skn/README file.
* - Refresh stack on click (number of entry is correct but value not)				      
* ---06/08/10---
* - Working on a new system of configuration file.The config.dat is now a binary file (as .skn but totally differennt). At start, a CONFIG_INFOS struct is filling, then when clicking on right menu, this struct is modified, then when you stop tilem, config.dat is writed on disc. 
* ---09/08/10---
* - Correction of the SEG_FAULT (never use sizeof(char*) inside malloc, strlen(char*) is better :P).
* ---10/08/10---
* - Working on a new config file called romconfig.dat (inspired from config.dat) using to do not ask for model each time tilem is started.
* - It works :D
* ---12/08/10---
* - NEW feature : Can load a file using libticalcs2 (inspired from the wokr of Benjamin Moody). Basically it works, but it's not OK. (many bugs)
* - Drop the deprecated GtkFileSelection.Use GtkFileChooser instead. :)  
* ---13/08/10---
* - NEW feature : Add the screenshot feature.
* ---17/08/10---
* - Change the ti84plus.skn (old was really ugly).
* - Add doc/ directory : add romconfig_file_format.txt and skinconfig_file_format.txt.
* ---18/08/10---
* - Correct the bug in link.c (unlock mutex ...)
* - Start working on macro handling : Always do the same things to load and launch a file into an emulator become quickly noisy for the programmer (1 time, 10 times, 30 times, 50 times...argh!). Simply record a sequence and play it to test a program, this is one solution. (feature request from Guillaume Hoffman gh@gmail.com).
* ---19/08/10---
* - The macro feature works including loading file (very important). The implementation is very basic (record and read a text file) so many bug could (should?) appear. But I wait to see how it will be use.
* ---20/08/10--
* - Better implementation of GtkFileChooser (to be cleaner).
* - Some work on macro (no seg fault now ^^).
* - Add a Save state... entry in right click menu (no need to stop tilem to save state)
* - Add a Play macro with GtkFileChooser (to play another macro than play.txt).
* - Fix minor bug in GtkFileChooser (forget to init a char* to NULL).
* ---23/08/10---
* - Add a new args handler using getop library (add -m for macro and -k for skin).
* ---06/09/10---
* - NEW feature : could print the lcd content into the terminal. So useless but so funny ;) (feature request from Guillaume Hoffman).
* - The output is saved into a file called lcd_content.txt.
* ---08/09/10---
* - Could choose wich char to display. This not really works as I want, but this is not a really important feature (works 1/2)
* - Add an option at startup (-l). Could now start in skinless mode.
* ---04/10/10---
* - Start working on animated gif. Some research on GIF file format. Oops it will be hard, file is encoded (!).
* ---09/10/10---
* - Creation of 1 little static gif file seems working, but always no LZW encoding.
* ---12/10/10---
* - Finally I decided to use external source to encode the pix data.I use a file called "gifencode.c" by Hans Dinsen-Hansen and Michael A. Mayer. And it works !!!
* ---14/10/10---
* - It works ! It works !!! Tilem2 is now able to generate animated gif, functions are currently working (but totally buggy) and it successfully create animated gif :)
* - Need to be integrated (and lot of debug).I commit it just to save it...Wait another commit to really use this feature :P
* ---01/02/11---
* - Starting to work on a new config file using glibc to do not hard code keypad values.
* - And it works !!!! (but only load one keypad model currently)
* - Add the other models into keylist.ini (but the content is completely false). Change scan_click method (correct a bug) to use kp->nb_of_buttons. Only need to give correct value into the keylist.ini file. For the rest it's seems ok.
* ---07/02/11---
* - Start to work on config.ini. A new generation config file using GKeyFile library (glib). Add some work in essai4 directory.
* ---08/02/11---
* - Remove romconfig.c romconfig.h config.c config.h (handle binary config file). Remove ROM_CONFIG_INFOS and CONFIG_INFOS from gsi.
* - Add a new config.c and config.h file to handle config (last rom used, default skin to load, etc...). It uses glib GKeyFile library.
* - Fix the macro bug pointed by Benjamin.
* ---15/02/11---
* - Replace correct copyright/licence informations into skinops.* .Sorry for the inconvenience.
* ---16/02/11---
* - Fix an important mistake into the gif creator function (palette should be before gif frame information).
* ---18/02/11---
* - Connect a timer to automatically add frame to a animated screenshot (using screenshot box).
*/


/* #########  MAIN  ######### */

int main(int argc, char **argv)
{
	
	FILE *romfile,*savfile;
	char default_model;


	/* Some allocation to do not crash */	
	GLOBAL_SKIN_INFOS *gsi;
	gsi=malloc(sizeof(GLOBAL_SKIN_INFOS));
	gsi->si=NULL;
	gsi->pWindow = NULL;
	gsi->FileToLoad = NULL;
	gsi->MacroName = NULL;
	gsi->mouse_key = 0;
	gsi->macro_file = NULL;
	gsi->isMacroRecording = 0;
	gsi->isAnimScreenshotRecording = FALSE;

	gsi->key_queue = NULL;
	gsi->key_queue_len = 0;
	gsi->key_queue_timer = 0;

	/***** TESTING *****/

	gsi->nkeybindings = 3;
	gsi->keybindings = g_new(TilemKeyBinding, 3);
	gsi->keybindings[0].keysym = GDK_A;
	gsi->keybindings[0].modifiers = 0;
	gsi->keybindings[0].nscancodes = 2;
	gsi->keybindings[0].scancodes = g_new(byte, 2);
	gsi->keybindings[0].scancodes[0] = TILEM_KEY_ALPHA;
	gsi->keybindings[0].scancodes[1] = TILEM_KEY_MATH;
	gsi->keybindings[1].keysym = GDK_B;
	gsi->keybindings[1].modifiers = 0;
	gsi->keybindings[1].nscancodes = 2;
	gsi->keybindings[1].scancodes = g_new(byte, 2);
	gsi->keybindings[1].scancodes[0] = TILEM_KEY_ALPHA;
	gsi->keybindings[1].scancodes[1] = TILEM_KEY_MATRIX;
	gsi->keybindings[2].keysym = GDK_1;
	gsi->keybindings[2].modifiers = 0;
	gsi->keybindings[2].nscancodes = 1;
	gsi->keybindings[2].scancodes = g_new(byte, 1);
	gsi->keybindings[2].scancodes[0] = TILEM_KEY_1;

	/*****/
	
	/* Init GTK+ */
	g_thread_init(NULL);
	gtk_init(&argc, &argv);
	
	/* Init isDebuggerRunning */
	gsi->isDebuggerRunning=FALSE;

	/* Default start with skin */
	gsi->isStartingSkinless = FALSE;
	
	/* Get the romname */
	getargs(argc, argv, gsi);
	/* End */	
	
	/* Create the SavName */
	create_savname(gsi);
	/* End */

	/* Open the romfile */
	romfile = g_fopen(gsi->RomName, "rb");
	if (!romfile) 
	{
		perror(gsi->RomName);
		return 1;
	}
	/* end */
	

	if((gsi->calc_id = get_modelcalcid(gsi->RomName)) != '0')
	{
		DCONFIG_FILE_L0_A1("Saved model id : %c\n", gsi->calc_id);
	} else {
		/* prompt user for calculator model */
		default_model = tilem_guess_rom_type(romfile);
		gsi->calc_id = choose_rom_popup(NULL, gsi->RomName, default_model);
	}

	if (!gsi->calc_id) {
		fclose(romfile);
		return 0;
	}
	
	/* Create the calc */
	gsi->emu = g_new0(TilemCalcEmulator, 1);
	gsi->emu->calc = tilem_calc_new(gsi->calc_id);
	/* End */
	
	/* Load save state */
	savfile = g_fopen(gsi->SavName, "rt");
	tilem_calc_load_state(gsi->emu->calc, romfile, savfile);
	/* End */

	if (savfile)
		fclose(savfile);

	fclose(romfile);

	/* Init emulator */
	gsi->emu->run_mutex = g_mutex_new();
	gsi->emu->calc_mutex = g_mutex_new();
	gsi->emu->lcd_mutex = g_mutex_new();
	gsi->emu->exiting = FALSE;
	gsi->emu->calc->lcd.emuflags = TILEM_LCD_REQUIRE_DELAY;
	gsi->emu->calc->flash.emuflags = TILEM_FLASH_REQUIRE_DELAY;
	/* end */

	gsi->emu->glcd = tilem_gray_lcd_new(gsi->emu->calc, 8, 200);

	DGLOBAL_L0_A0("**************** fct : main ****************************\n");
	DGLOBAL_L0_A1("*  calc_id= %c                                            *\n",gsi->calc_id);
	DGLOBAL_L0_A1("*  emu.calc->hw.model= %c                               *\n",gsi->emu->calc->hw.model_id);	
	DGLOBAL_L0_A1("*  emu.calc->hw.name= %s                             *\n",gsi->emu->calc->hw.name);		
	DGLOBAL_L0_A1("*  emu.calc->hw.name[3]= %c                             *\n",gsi->emu->calc->hw.name[3]);
	DGLOBAL_L0_A0("********************************************************\n");
	

	if(gsi->SkinFileName == NULL) { /* Given as parameter ? */
		/* Test if exists into config.ini */
		if(get_defaultskin(gsi->RomName) != NULL) 
		{
			/* it exists */
			gsi->SkinFileName = get_defaultskin(gsi->RomName);
			printf("Load saved skin : %s\n", gsi->SkinFileName);
			DCONFIG_FILE_L0_A1("Saved model id : %c\n", gsi->calc_id);
		} else {
			/* User does not have choosen another skin for this model, choose officials :) */
			printf("skin default not found : %s\n", gsi->RomName ); 
			choose_skin_filename(gsi->emu->calc,gsi);
		}
	}	

	/* Draw skin */	
	gsi->pWindow=draw_screen(gsi);
		
	if(gsi->FileToLoad != NULL) /* Given as parameter ? */
		load_file_from_file(gsi, gsi->FileToLoad);
	if(gsi->MacroName != NULL) /* Given as parameter ? */
		play_macro_default(gsi, gsi->MacroName); 		
	if(gsi->isStartingSkinless) /* Given as parameter ? */
		switch_view(gsi); /* Start without skin */
	
	
	/* ####### BEGIN THE GTK_MAIN_LOOP ####### */
	gtk_main();
	
	
	
	/* Save the state */
	if(SAVE_STATE==1) {
		romfile = g_fopen(gsi->RomName, "wb");
		savfile = g_fopen(gsi->SavName, "wt");
		tilem_calc_save_state(gsi->emu->calc, romfile, savfile);
	}
	
    return EXIT_SUCCESS;
}












