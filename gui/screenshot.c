/*
 * TilEm II
 *
 * Copyright (c) 2010-2011 Thibault Duponchelle
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <ticalcs.h>
#include <tilem.h>

#include "gui.h"
#include "files.h"
#include "msgbox.h"

#define DEFAULT_WIDTH_96    192
#define DEFAULT_HEIGHT_96   128
#define DEFAULT_WIDTH_128   256
#define DEFAULT_HEIGHT_128  128
#define DEFAULT_FORMAT      "png"

static void on_screenshot();
static void on_save(G_GNUC_UNUSED GtkWidget* win, TilemCalcEmulator* emu);
static void on_record(G_GNUC_UNUSED GtkWidget* win, TilemCalcEmulator* emu);
static void on_stop(G_GNUC_UNUSED GtkWidget* win, TilemCalcEmulator* emu);
static void on_play(G_GNUC_UNUSED GtkWidget* win,TilemCalcEmulator* emu);
static void on_playfrom(G_GNUC_UNUSED GtkWidget* win,TilemCalcEmulator* emu);
static void on_destroy_playview(G_GNUC_UNUSED GtkWidget* playwin);
static void on_destroy_screenshot(GtkWidget* screenshotanim_win);
static void on_change_screenshot_directory(G_GNUC_UNUSED GtkWidget * win, TilemCalcEmulator* emu);
static void on_change_animation_directory(G_GNUC_UNUSED GtkWidget * win, TilemCalcEmulator* emu);
static gboolean save_screenshot(TilemCalcEmulator *emu, const char *filename, const char *format);
char* find_free_filename(const char* directory, const char* filename, const char* extension);
static void change_review_image(TilemCalcEmulator * emu, char * new_image);
static void refresh_size_spin(G_GNUC_UNUSED GtkWidget * win, TilemCalcEmulator* emu);
static int get_width_from_size_combo(char* size);
static int get_height_from_size_combo(char* size);

/* UNUSED */
void on_add_frame(G_GNUC_UNUSED GtkWidget* win, TilemCalcEmulator* emu);

static gboolean is_wide_screen(TilemCalcEmulator *emu)
{
	g_return_val_if_fail(emu != NULL, FALSE);
	g_return_val_if_fail(emu->calc != NULL, FALSE);

	return (emu->calc->hw.lcdwidth == 128);
}

/* Quick screenshot: save a screenshot with predefined settings,
   without prompting the user */
void quick_screenshot(TilemEmulatorWindow *ewin)
{
	char *folder, *filename, *format;
	int grayscale, w96, h96, w128, h128, width, height;
	TilemAnimation *anim;
	GError *err = NULL;

	tilem_config_get("quick_screenshot",
	                 "directory/f", &folder,
	                 "format/s", &format,
	                 "grayscale/b", &grayscale,
	                 "width_96x64/i", &w96,
	                 "height_96x64/i", &h96,
	                 "width_128x64/i", &w128,
	                 "height_128x64/i", &h128,
	                 NULL);

	anim = tilem_calc_emulator_get_screenshot(ewin->emu, grayscale);
	if (!anim) {
		g_free(folder);
		g_free(format);
		return;
	}

	if (is_wide_screen(ewin->emu)) {
		width = (w128 > 0 ? w128 : DEFAULT_WIDTH_128);
		height = (h128 > 0 ? h128 : DEFAULT_HEIGHT_128);
	}
	else {
		width = (w96 > 0 ? w96 : DEFAULT_WIDTH_96);
		height = (h96 > 0 ? h96 : DEFAULT_HEIGHT_96);
	}

	tilem_animation_set_size(anim, width, height);

	if (!folder)
		folder = get_config_file_path("screenshots", NULL);

	if (!format)
		format = g_strdup(DEFAULT_FORMAT);

	g_mkdir_with_parents(folder, 0755);

	filename = find_free_filename(folder, "screenshot", format);
	if (!filename) {
		g_free(folder);
		g_free(format);
		g_object_unref(anim);
		return;
	}

	if (!tilem_animation_save(anim, filename, format, NULL, NULL, &err)) {
		messagebox01(ewin->window, GTK_MESSAGE_ERROR,
		             "Unable to save screenshot",
		             "%s", err->message);
		g_error_free(err);
	}

	g_object_unref(anim);
	g_free(filename);
	g_free(folder);
	g_free(format);
}

/* Look for a free filename by testing [folder]/[basename]000.[extension] to [folder]/[basename]999.[extension]
   Return a newly allocated string if success
   Return null if no filename found */
char* find_free_filename(const char* folder, const char* basename, const char* extension) {
	
	int i;
	char* filename;

	/* I do not use a while and limit number to 1000 because for any reason, if there's a problem in this scope
	   I don't want to freeze tilem (if tilem don't find a free filename and never return anything)
	   Limit to 1000 prevent this problem but if you prefer we could use a while wich wait a valid filename... */
	for(i=0; i<999; i++) {
		if(folder) {
			filename = g_build_filename(folder, basename, NULL);	
			/*printf("path : %s\n", filename); */
		} else {
			filename = g_build_filename(basename, NULL);	
			/*printf("path : %s\n", filename);*/
		}
		filename = g_strdup_printf("%s%03d%c%s", filename, i, '.',  extension );
		/*printf("path : %s\n", filename); */
		
		if(!g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
			return filename;
			break;
		}
	}
	
	return NULL;
}


/* Replace the current logo/review image by the recent shot/animation */
static void change_review_image(TilemCalcEmulator * emu, char * new_image) {

	/* Test if the widget exists (should exists), if not don't try to change the image */
	if(GTK_IS_WIDGET(emu->ssdlg->screenshot_preview_image)) {
		GtkWidget * screenshot_preview = gtk_widget_get_parent(GTK_WIDGET(emu->ssdlg->screenshot_preview_image));
		if(GTK_IS_SPINNER(emu->ssdlg->screenshot_preview_image)) {
			gtk_spinner_stop(GTK_SPINNER(emu->ssdlg->screenshot_preview_image));
		}
		gtk_object_destroy(GTK_OBJECT(emu->ssdlg->screenshot_preview_image));
		emu->ssdlg->screenshot_preview_image = gtk_image_new_from_file(new_image);
		gtk_widget_show(emu->ssdlg->screenshot_preview_image);
		gtk_layout_put(GTK_LAYOUT(screenshot_preview), emu->ssdlg->screenshot_preview_image, 10, 10);
		gtk_widget_show(screenshot_preview);
	}

}

/* Destroy the screenshot box */
static void on_destroy_screenshot(GtkWidget* screenshotanim_win)   {
	gtk_widget_destroy(GTK_WIDGET(screenshotanim_win));
}

/* Create the size combo for default size */
static void create_size_combobox(TilemCalcEmulator * emu) {
	
	emu->ssdlg->ss_size_combo = gtk_combo_box_new_text(); 
	
	/* We don't use the same default because ti85 and ti86 are wide screen */	
	if((strcmp(emu->calc->hw.name, "ti85") == 0) || (strcmp(emu->calc->hw.name, "ti86") == 0)) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(emu->ssdlg->ss_size_combo), "219 x 128");
		gtk_combo_box_append_text(GTK_COMBO_BOX(emu->ssdlg->ss_size_combo), "256 x 128"); /* DEFAULT */ 
		gtk_combo_box_append_text(GTK_COMBO_BOX(emu->ssdlg->ss_size_combo), "256 x 150");
		gtk_combo_box_append_text(GTK_COMBO_BOX(emu->ssdlg->ss_size_combo), "328 x 192");
		gtk_combo_box_append_text(GTK_COMBO_BOX(emu->ssdlg->ss_size_combo), "384 x 192");
		gtk_combo_box_set_active(GTK_COMBO_BOX(emu->ssdlg->ss_size_combo), 1); /* Default selection */
	} else {
		gtk_combo_box_append_text(GTK_COMBO_BOX(emu->ssdlg->ss_size_combo), "96 x 64");
		gtk_combo_box_append_text(GTK_COMBO_BOX(emu->ssdlg->ss_size_combo), "128 x 64"); /* DEFAULT */
		gtk_combo_box_append_text(GTK_COMBO_BOX(emu->ssdlg->ss_size_combo), "128 x 75");
		gtk_combo_box_append_text(GTK_COMBO_BOX(emu->ssdlg->ss_size_combo), "192 x 128");
		gtk_combo_box_append_text(GTK_COMBO_BOX(emu->ssdlg->ss_size_combo), "288 x 192");
		gtk_combo_box_set_active(GTK_COMBO_BOX(emu->ssdlg->ss_size_combo), 1); /* Default selection */
	}
}

/* Create the screenshot menu */
void create_screenshot_window(TilemEmulatorWindow* ewin)
{
	TilemCalcEmulator *emu = ewin->emu;
	GtkWidget* screenshotanim_win;
	screenshotanim_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(screenshotanim_win), "Screenshot");
	gtk_window_set_default_size(GTK_WINDOW(screenshotanim_win) , 450, 300);
	
	g_signal_connect(GTK_OBJECT(screenshotanim_win), "delete-event", G_CALLBACK(on_destroy_screenshot), NULL);

	GtkWidget* hbox, *vbox, *parent_vbox;
	parent_vbox = gtk_vbox_new (0, 1);
	vbox = gtk_vbox_new(0,1);
	hbox = gtk_hbox_new (0, 1);
	gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);	
	
	
		
	GtkWidget * screenshot_preview = gtk_expander_new("preview");
	gtk_expander_set_expanded(GTK_EXPANDER(screenshot_preview), TRUE);
	GtkWidget* layout = gtk_layout_new(NULL,NULL);
	gtk_layout_set_size(GTK_LAYOUT(layout), 200, 100); 
	
	/* Print the nice logo (from old tilem) 
	   Maybe try to load the most recent gif/screenshot could be a good idea... (as it was done before)
	   But I think it's good like this, because we don't really need to see last session screenshot.
	   And maybe it doesn't exist or saved filename is bad...*/ 
	char* tilem_logo = get_shared_file_path("pixs", "tilem.png", NULL);
	if(tilem_logo)
		emu->ssdlg->screenshot_preview_image = gtk_image_new_from_file(tilem_logo);
	else 
		emu->ssdlg->screenshot_preview_image = gtk_image_new();
	g_free(tilem_logo);

	

	gtk_layout_put(GTK_LAYOUT(layout), emu->ssdlg->screenshot_preview_image, 10, 10);
	gtk_container_add(GTK_CONTAINER(screenshot_preview), layout);

	gtk_container_add(GTK_CONTAINER(screenshotanim_win), parent_vbox);
	gtk_box_pack_start(GTK_BOX(parent_vbox), hbox, 2, 3, 4);
	gtk_box_pack_start(GTK_BOX(hbox), screenshot_preview, 2, 3, 4);
	gtk_box_pack_end(GTK_BOX(hbox), vbox, 2, 3, 4);

		
	GtkWidget* screenshot = gtk_button_new_with_label ("Shoot!");
	GtkWidget* record = gtk_button_new_with_label ("Record");
	GtkWidget* stop = gtk_button_new_with_label("Stop");
	gtk_widget_set_sensitive(GTK_WIDGET(stop), FALSE);
	GtkWidget* play = gtk_button_new_with_label ("Replay (detached)");
	GtkWidget* playfrom = gtk_button_new_with_label ("Replay (browse)");
	GtkWidget* save = gtk_button_new_with_label("Save");
	gtk_widget_set_sensitive(GTK_WIDGET(save), FALSE);
	emu->ssdlg->screenshot = screenshot;
	emu->ssdlg->record = record;
	emu->ssdlg->stop = stop;
	emu->ssdlg->play = play;
	emu->ssdlg->playfrom = playfrom;
	emu->ssdlg->save = save;

	/* >>>> SOUTH */	
	GtkWidget * config_expander = gtk_expander_new("config");
	gtk_expander_set_expanded(GTK_EXPANDER(config_expander), TRUE);
	
	GtkWidget* vboxc0, *hboxc00, *hboxc1, *hboxc2; 
	vboxc0 = gtk_vbox_new(TRUE,2);
	hboxc00 = gtk_hbox_new (TRUE, 1);
	//hboxc0 = gtk_hbox_new (TRUE, 1);
	hboxc1 = gtk_hbox_new (TRUE, 1);
	hboxc2 = gtk_hbox_new (TRUE, 1);
	
	gtk_container_add(GTK_CONTAINER(config_expander), vboxc0);
	gtk_box_pack_start(GTK_BOX(vboxc0), hboxc00, 2, 3, 4);
	//gtk_box_pack_start(GTK_BOX(vboxc0), hboxc0, 2, 3, 4);
	gtk_box_pack_start(GTK_BOX(vboxc0), hboxc1, 2, 3, 4);
	gtk_box_pack_end(GTK_BOX(vboxc0), hboxc2, 2, 3, 4);


	/* Labels */	
	/*GtkWidget * screenshot_size = gtk_label_new("Screenshot size :");*/
	/*GtkWidget * screenshot_extension = gtk_label_new("Screenshot extension :");*/
	GtkWidget * screenshot_dir_label = gtk_label_new("Screenshot folder :");
	GtkWidget * animation_dir_label = gtk_label_new("Animations folder :");

	/* About the size */	
	emu->ssdlg->width_spin = gtk_spin_button_new_with_range(0, 500, 1);
	emu->ssdlg->height_spin = gtk_spin_button_new_with_range(0, 500, 1);

	create_size_combobox(emu);

	refresh_size_spin(emu->ssdlg->screenshot_win, emu); 
	

	/* FIXME : USE DEPRECATED SYMBOLS */
	emu->ssdlg->ss_ext_combo = gtk_combo_box_new_text(); 
	gtk_combo_box_append_text(GTK_COMBO_BOX(emu->ssdlg->ss_ext_combo), "png");
	gtk_combo_box_append_text(GTK_COMBO_BOX(emu->ssdlg->ss_ext_combo), "jpeg");
	gtk_combo_box_append_text(GTK_COMBO_BOX(emu->ssdlg->ss_ext_combo), "bmp");
	gtk_combo_box_append_text(GTK_COMBO_BOX(emu->ssdlg->ss_ext_combo), "gif");
	gtk_combo_box_set_active(GTK_COMBO_BOX(emu->ssdlg->ss_ext_combo), 3);

	/* GtkFileChooserButton */
	char *ssdir, *animdir;

	tilem_config_get("screenshot",
	                 "screenshot_directory/f", &ssdir,
	                 "animation_directory/f", &animdir,
	                 NULL);

	emu->ssdlg->folder_chooser_screenshot = gtk_file_chooser_button_new("Screenshot", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(emu->ssdlg->folder_chooser_screenshot), ssdir);
	emu->ssdlg->folder_chooser_animation = gtk_file_chooser_button_new("Animation", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(emu->ssdlg->folder_chooser_animation), animdir);
	g_free(ssdir);
	g_free(animdir);
	
	
	gtk_box_pack_start (GTK_BOX (hboxc00), emu->ssdlg->width_spin, 2, 3, 4);
	gtk_box_pack_start (GTK_BOX (hboxc00), emu->ssdlg->height_spin, 2, 3, 4);
	gtk_box_pack_start (GTK_BOX (hboxc00), emu->ssdlg->ss_size_combo, 2, 3, 4);
	//gtk_box_pack_start (GTK_BOX (hboxc0), screenshot_extension, 2, 3, 4);
	gtk_box_pack_start (GTK_BOX (hboxc00), emu->ssdlg->ss_ext_combo, 2, 3, 4);
	gtk_box_pack_start (GTK_BOX (hboxc1), screenshot_dir_label, 2, 3, 4);
	gtk_box_pack_end (GTK_BOX (hboxc1), emu->ssdlg->folder_chooser_screenshot, 2, 3, 4);
	gtk_box_pack_start (GTK_BOX (hboxc2), animation_dir_label, 2, 3, 4);
	gtk_box_pack_end (GTK_BOX (hboxc2), emu->ssdlg->folder_chooser_animation, 2, 3, 4);
	gtk_widget_show(screenshot_dir_label);
	gtk_widget_show(animation_dir_label);
	gtk_widget_show(emu->ssdlg->folder_chooser_animation);
	gtk_widget_show(emu->ssdlg->folder_chooser_screenshot);
	/* <<<< */	
	
	gtk_box_pack_start (GTK_BOX (vbox), screenshot, FALSE, 3, 4);
	gtk_widget_show(screenshot);
	gtk_box_pack_start (GTK_BOX (vbox), record, FALSE, 3, 4);
	gtk_widget_show(record);
	//gtk_box_pack_start (GTK_BOX (hbox), add_frame, 2, 3, 4);
	//gtk_widget_show(add_frame);
	gtk_box_pack_start (GTK_BOX (vbox), stop, FALSE, 3, 4);
	gtk_widget_show(stop);
	gtk_box_pack_start (GTK_BOX (vbox), play, FALSE, 3, 4);
	gtk_widget_show(play);
	gtk_box_pack_start (GTK_BOX (vbox), playfrom, FALSE, 3, 4);
	gtk_widget_show(playfrom);
	gtk_box_pack_start (GTK_BOX (vbox), emu->ssdlg->save, 2, 3, 4);
	gtk_widget_show(emu->ssdlg->save);

	gtk_box_pack_end (GTK_BOX (parent_vbox), config_expander, FALSE, 3, 4);
	
	g_signal_connect(GTK_OBJECT(screenshot), "clicked", G_CALLBACK(on_screenshot), emu);
	g_signal_connect(GTK_OBJECT(record), "clicked", G_CALLBACK(on_record), emu);
	//g_signal_connect(GTK_OBJECT(add_frame), "clicked", G_CALLBACK(on_add_frame), emu);
	g_signal_connect(GTK_OBJECT(stop), "clicked", G_CALLBACK(on_stop), emu);
	g_signal_connect(GTK_OBJECT(play), "clicked", G_CALLBACK(on_play), emu);
	g_signal_connect(GTK_OBJECT(playfrom), "clicked", G_CALLBACK(on_playfrom), emu);
	g_signal_connect(GTK_OBJECT(save), "clicked", G_CALLBACK(on_save), emu);
	g_signal_connect(GTK_OBJECT(emu->ssdlg->folder_chooser_screenshot), "selection-changed", G_CALLBACK(on_change_screenshot_directory), emu);
	g_signal_connect(GTK_OBJECT(emu->ssdlg->folder_chooser_animation), "selection-changed", G_CALLBACK(on_change_animation_directory), emu);
	g_signal_connect(GTK_OBJECT(emu->ssdlg->ss_size_combo), "changed", G_CALLBACK(refresh_size_spin), emu);
	gtk_widget_show_all(screenshotanim_win);
}

/* These stuff will be improved */
/* Extract width from the "width x size" string */
static int get_width_from_size_combo(char* size) {
	if(size) {
		if(strncmp(size,"96", 2) == 0)
			return 96; 	
		if(strncmp(size,"128", 3) == 0)
			return 128; 	
		if(strncmp(size,"192", 3) == 0)
			return 192; 	
		if(strncmp(size,"219", 3) == 0)
			return 219; 	
		if(strncmp(size,"256", 3) == 0)
			return 256; 	
		if(strncmp(size,"328", 3) == 0)
			return 328; 	
		if(strncmp(size,"384", 3) == 0)
			return 384; 	
	}

	return 192;
}	

/* Extract height from the "width x size" string */
static int get_height_from_size_combo(char* size) {
	if(size) {
		if((strcmp(size,"96 x 64") == 0) || (strcmp(size, "128 x 64") == 0))
			return 64; 	
		if(strcmp(size,"128 x 75") == 0)
			return 75; 	
		if(strcmp(size,"192 x 128") == 0) 
			return 128; 	
		if(strcmp(size, "256 x 128") == 0)
			return 128; 	
		if(strcmp(size,"256 x 150") == 0)
			return 150; 	
		if(strcmp(size, "288 x 192") == 0)
			return 192; 	
		if(strcmp(size, "328 x 192")==0) 
			return 192; 	
		if(strcmp(size, "384 x 192") == 0)
			return 192; 	
	}

	return 128;
}


/* Callback for record button */
static void on_record(G_GNUC_UNUSED GtkWidget* win, TilemCalcEmulator* emu) {
	g_print("record event\n");
	gtk_widget_set_sensitive(GTK_WIDGET(emu->ssdlg->screenshot), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(emu->ssdlg->record), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(emu->ssdlg->stop), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(emu->ssdlg->play), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(emu->ssdlg->playfrom), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(emu->ssdlg->save), FALSE);
	tilem_calc_emulator_begin_animation(emu, TRUE);
	char* size = NULL;
	

	/* You can choose to hide current animation while recording or not
	   It's as you prefer... For the moment I hide it */	
	gtk_widget_hide(GTK_WIDGET(emu->ssdlg->screenshot_preview_image));
	if(GTK_IS_COMBO_BOX(emu->ssdlg->ss_size_combo)) {
		size = gtk_combo_box_get_active_text(GTK_COMBO_BOX(emu->ssdlg->ss_size_combo));
		printf("size : %s\n", size);
	}
	int height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(emu->ssdlg->height_spin));
	int width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(emu->ssdlg->width_spin));

	/* This protection is used if you don't choose a value (default) */
	if(height <= 0 || height > 384)
		height = 128;
	
	if(width <= 0 || width > 384)
		height = 128;

	tilem_animation_set_size(emu->anim, width, height); 
	g_free(size);
}


static void on_save(G_GNUC_UNUSED GtkWidget* win, TilemCalcEmulator* emu) {
	printf("on_save\n");
	/* FIXME : use Benjamin's function instead mine */
	char* filename = select_file_for_save(emu, NULL);
	if(filename) {
		char* format = strdup("gif");	
		if(GTK_IS_COMBO_BOX(emu->ssdlg->ss_ext_combo))
			format = gtk_combo_box_get_active_text(GTK_COMBO_BOX(emu->ssdlg->ss_ext_combo));
		tilem_animation_save(emu->ssdlg->current_anim, filename,  format, NULL, NULL, NULL);
		printf("Saved as : %s\n", filename);
		g_free(format);
		g_free(filename);
	}


}

/* Only used for testing purpose */
/* NEVER USED  (but it works) xD */
void on_add_frame(G_GNUC_UNUSED GtkWidget* win, TilemCalcEmulator* emu) {
	g_print("add_frame event\n");
	tilem_animation_add_frame(emu);
}

/* Change the review image to set the current animation */
static void set_current_animation(TilemScreenshotDialog *ssdlg,
                                  TilemAnimation *anim)
{
	if (ssdlg->current_anim)
		g_object_unref(ssdlg->current_anim);
	ssdlg->current_anim = anim;
	if(GTK_IS_IMAGE(ssdlg->screenshot_preview_image)) {
		gtk_image_set_from_animation(GTK_IMAGE(ssdlg->screenshot_preview_image),
	                             GDK_PIXBUF_ANIMATION(anim));
		/* Need to call gtk_widget_show because we hide it while recording */
		gtk_widget_show(GTK_WIDGET(ssdlg->screenshot_preview_image));
	}
}

/* Callback for stop button (stop the recording) */
static void on_stop(G_GNUC_UNUSED GtkWidget* win, TilemCalcEmulator* emu)
{
	TilemAnimation *anim;

	anim = tilem_calc_emulator_end_animation(emu);
	set_current_animation(emu->ssdlg, anim);
	gtk_widget_set_sensitive(GTK_WIDGET(emu->ssdlg->screenshot), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(emu->ssdlg->record), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(emu->ssdlg->stop), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(emu->ssdlg->play), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(emu->ssdlg->playfrom), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(emu->ssdlg->save), TRUE);
	
	/*
	g_print("stop event\n");
	char* dest = NULL, *dir;

	tilem_animation_stop(emu) ;
	
	if(emu->ssdlg->isAnimScreenshotRecording) {
		emu->ssdlg->isAnimScreenshotRecording = FALSE;
		stop_spinner(emu);

		tilem_config_get("screenshot",
		                 "animation_directory/f", &dir,
		                 NULL);
		dest = select_file_for_save(emu, dir);
		g_free(dir);
	}

	if(dest) {
		dir = g_path_get_dirname(dest);
		tilem_config_set("screenshot",
		                 "animation_recent/f", dest,
		                 "animation_directory/f", dir,
		                 NULL);
		g_free(dir);

		copy_paste("gifencod.gif", dest);
		change_review_image(emu, dest);
	}
	g_free(dest);

	delete_spinner_and_put_logo(emu);
	*/
}

/* Callback for screenshot button (take a screenshot) */
static void on_screenshot(G_GNUC_UNUSED GtkWidget* win, TilemCalcEmulator* emu)
{
	TilemAnimation *anim;

	anim = tilem_calc_emulator_get_screenshot(emu, TRUE);
	set_current_animation(emu->ssdlg, anim);
	gtk_widget_set_sensitive(GTK_WIDGET(emu->ssdlg->save), TRUE);

	/*
	screenshot(emu->ewin);
	g_print("screenshot event\n");
	*/
}


/* Screenshot saver */
static gboolean save_screenshot(TilemCalcEmulator *emu, const char *filename,
                                const char *format)
{
	int width = emu->calc->hw.lcdwidth * 2;
	int height = emu->calc->hw.lcdheight * 2;
	guchar *buffer;
	dword *palette;
	GdkPixbuf *pb;
	gboolean status;
	GError *err = NULL;

	buffer = g_new(guchar, width * height * 3);

	/* FIXME: this palette should be cached for future use.  Also
	   might want the option to save images in skinned colors. */
	palette = tilem_color_palette_new(255, 255, 255, 0, 0, 0, 2.2);

	g_mutex_lock(emu->lcd_mutex);
	tilem_draw_lcd_image_rgb(emu->lcd_buffer, buffer,
	                         width, height, width * 3, 3,
	                         palette, TILEM_SCALE_SMOOTH);
	g_mutex_unlock(emu->lcd_mutex);

	tilem_free(palette);

	pb = gdk_pixbuf_new_from_data(buffer, GDK_COLORSPACE_RGB, FALSE, 8,
	                              width, height, width * 3, NULL, NULL);

	status = gdk_pixbuf_save(pb, filename, format, &err, NULL);

	g_object_unref(pb);
	g_free(buffer);

	if (!status) {
		fprintf(stderr, "*** unable to save screenshot: %s\n", err->message);
		g_error_free(err);
	} else {
		printf("Screenshot successfully saved as : %s\n", filename);
	}

	return status;
}

/* Destroy the screenshot box */
static void on_destroy_playview(GtkWidget* playwin)   {
	
	gtk_widget_destroy(GTK_WIDGET(playwin));
}

/* Callback for play button (replay the last gif) */
static void on_play(G_GNUC_UNUSED GtkWidget * win, G_GNUC_UNUSED TilemCalcEmulator* emu) {
	printf("play\n");
	GtkWidget *image = NULL;
	char *filename;

	tilem_config_get("screenshot",
	                 "animation_recent/f", &filename,
	                 NULL);
	if (!filename)
		return;

	emu->ssdlg->screenshot_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(emu->ssdlg->screenshot_win),"destroy",G_CALLBACK(on_destroy_playview), NULL);

	image = gtk_image_new_from_file(filename);
	gtk_container_add(GTK_CONTAINER(emu->ssdlg->screenshot_win),image);

	gtk_widget_show_all(emu->ssdlg->screenshot_win);
	g_free(filename);
}

/* Callback for play button (replay the last gif) */
static void on_playfrom(G_GNUC_UNUSED GtkWidget * win, TilemCalcEmulator* emu) {
	char* src = NULL, *dir;

	tilem_config_get("screenshot",
	                 "animation_directory/f", &dir,
	                 NULL);

	src = select_file_for_save(emu, dir);
	g_free(dir);
	if(src) {
		dir = g_path_get_dirname(src);
		tilem_config_set("screenshot",
		                 "animation_recent/f", src,
		                 "animation_directory/f", dir,
		                 NULL);
		g_free(dir);

		change_review_image(emu, src);
	}

	g_free(src);
}

/* This method refresh the size spin button using the size combo box as input values */
/* It's called at screenshot window and as callback (when combo box value change) */
static void refresh_size_spin(G_GNUC_UNUSED GtkWidget * win, TilemCalcEmulator* emu) {
	if(GTK_IS_COMBO_BOX(emu->ssdlg->ss_size_combo)) {
		char *size = gtk_combo_box_get_active_text(GTK_COMBO_BOX(emu->ssdlg->ss_size_combo));
		printf("size : %s\n", size);
		if(GTK_IS_SPIN_BUTTON(emu->ssdlg->width_spin))
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(emu->ssdlg->width_spin), get_width_from_size_combo(size));
		if(GTK_IS_SPIN_BUTTON(emu->ssdlg->height_spin))
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(emu->ssdlg->height_spin), get_height_from_size_combo(size));
		g_free(size);
	}
} 

/* When user change directory, save the new value in the config file */
static void on_change_screenshot_directory(G_GNUC_UNUSED GtkWidget * win, TilemCalcEmulator* emu) {
	char* folder = NULL;
	folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(emu->ssdlg->folder_chooser_screenshot)); 
	if(folder) 
		tilem_config_set("screenshot",
		                 "screenshot_directory/f", folder,
		                 NULL);
	g_free(folder);
}


/* When user change directory, save the new value in the config file */
static void on_change_animation_directory(G_GNUC_UNUSED GtkWidget * win, TilemCalcEmulator* emu) {
	char* folder = NULL;
	folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(emu->ssdlg->folder_chooser_animation)); 
	if(folder)
		tilem_config_set("screenshot",
		                 "animation_directory/f", folder,
		                 NULL);
	g_free(folder);
}
