#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include <tilem.h>
#include <gui.h>

/* Turn on recording macro */
void start_record_macro(GLOBAL_SKIN_INFOS* gsi) {
	gsi->isMacroRecording = 1;
}

/* Turn off recording macro */
void stop_record_macro(GLOBAL_SKIN_INFOS* gsi) {
	gsi->isMacroRecording = 0;
}
	
/* Create the macro file */
void create_or_replace_macro_file(GLOBAL_SKIN_INFOS* gsi) {
	FILE * macro_file;		
	
	/* if a macro already exists */
	if((macro_file= g_fopen("play.txt", "r"))!= NULL) {
		fclose(macro_file);
		macro_file = g_fopen("play.txt", "w");
		if(macro_file != NULL)
			fclose(macro_file);
	}


	macro_file = g_fopen("play.txt", "a+");
	gsi->macro_file= macro_file;
}

/* Recording */
void add_event_in_macro_file(GLOBAL_SKIN_INFOS* gsi, char * string) {
	
	/* First time ? So create the file */
	if(gsi->macro_file == NULL) {
		create_or_replace_macro_file(gsi);
	} else {
		fwrite(",", 1, 1, gsi->macro_file);
	}
	/* Write the key value */
	fwrite(string, 1, sizeof(int), gsi->macro_file);
	
	/* Write the comma to seperate */
}

/* Recording */
void add_load_file_in_macro_file(GLOBAL_SKIN_INFOS* gsi, int length, char* filename) {
	
	char * lengthchar;
	lengthchar= (char*) malloc(4 * sizeof(char));
	memset(lengthchar, 0 ,4);	
	
		
	DMACRO_L0_A0("************** fct : play_macro *******************\n");	
	DMACRO_L0_A2("* filename = %s, length = %d                *\n", filename, length);	
	
	/* First time ? So create the file */
	if(gsi->macro_file == NULL) {
		create_or_replace_macro_file(gsi);
	} else {
		fwrite(",", 1, 1, gsi->macro_file);
	}
	/* Write the key value */
	fwrite("file=", 1, 5, gsi->macro_file);
	sprintf(lengthchar, "%d",length);
	fwrite(lengthchar, 1, sizeof(int), gsi->macro_file);
	fwrite("-", 1, 1, gsi->macro_file);
	fwrite(filename, 1, length, gsi->macro_file);
	
	DMACRO_L0_A0("***************************************************\n");	
	/* Write the comma to seperate */
}

/* Open the file for reading value to play */
void open_macro_file(GLOBAL_SKIN_INFOS* gsi) {
	FILE * macro_file;
	macro_file= g_fopen("play.txt", "r");
	gsi->macro_file= macro_file;

}

/* Play the partition (macro) */
void play_macro(GLOBAL_SKIN_INFOS* gsi) {
	int code;
	char* codechar;
	char c;
	char* lengthchar;
	int length;
	char* filename;
	open_macro_file(gsi);
	DMACRO_L0_A0("************** fct : play_macro *******************\n");	
	while(c != EOF) {	
		codechar= (char*) malloc(4 * sizeof(char) + 1);
		memset(codechar, 0, 5);
		fread(codechar, 1, 4, gsi->macro_file);
		
		if(strcmp(codechar, "file") == 0 ) {
			c = fgetc(gsi->macro_file); /* Drop the "="*/
			lengthchar= (char*) malloc(4 * sizeof(char) + 1);
			memset(lengthchar, 0, 5);
			fread(lengthchar, 1, 4, gsi->macro_file);
			c = fgetc(gsi->macro_file); /* Drop the "-"*/
			length= atoi(lengthchar);
			DMACRO_L0_A2("* lengthchar = %s,    length = %d         *\n", lengthchar, length);
			filename= (char*) malloc(length * sizeof(char));
			fread(filename, 1, length, gsi->macro_file);
			load_file_from_file(gsi, filename);
			DMACRO_L0_A1("* send file = %s               *\n", filename);
			free(filename);
		} else {
			code = atoi(codechar);
			DMACRO_L0_A2("* codechar = %s,    code = %d         *\n", codechar, code);
			g_mutex_lock(gsi->emu->calc_mutex);
			run_with_key_slowly(gsi->emu->calc, code);			
			g_mutex_unlock(gsi->emu->calc_mutex);
		
		}
		c = fgetc(gsi->macro_file);
	}
	DMACRO_L0_A0("***************************************\n");	

}



/* Run slowly to play macro */
void run_with_key_slowly(TilemCalc* calc, int key)
{
	tilem_z80_run_time(calc, 500000, NULL);
	tilem_keypad_press_key(calc, key);
	tilem_z80_run_time(calc, 10000, NULL);
	tilem_keypad_release_key(calc, key);
	tilem_z80_run_time(calc, 5000000, NULL);
}
