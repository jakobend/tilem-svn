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
#include <errno.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <ticalcs.h>
#include <tilem.h>
#include <tilemdb.h>
#include <scancodes.h>

#include "gui.h"
#include "disasmview.h"
#include "memmodel.h"
#include "files.h"
#include "filedlg.h"
#include "msgbox.h"

static GtkTreeModel* fill_varlist();
TilemReceiveDialog* create_receive_menu(TilemCalcEmulator *emu);

/* Stack list */
enum
{
	COL_INDEX = 0, 
	COL_NAME,
	COL_TYPE,
	COL_SIZE,
  	NUM_COLS
};


/* #### SIGNALS CALLBACK #### */

/* Close the window */
static void tilem_rcvmenu_on_close(G_GNUC_UNUSED GtkWidget* w, G_GNUC_UNUSED gpointer data) {
	TilemReceiveDialog* rcvdialog = (TilemReceiveDialog*) data;

	gtk_widget_hide(rcvdialog->window);
}

/* Event called on Send button click. Get the selected var/app and save it. */
static void tilem_rcvmenu_on_receive(G_GNUC_UNUSED GtkWidget* w, G_GNUC_UNUSED gpointer data) {
	TilemReceiveDialog* rcvdialog = (TilemReceiveDialog*) data;
	printf("receive !!!!\n");
	gchar* varname;
	int index;
	//gtk_tree_model_get (rcvdialog->model, &rcvdialog->iter, 0, &varname, -1);
	GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(rcvdialog->treeview));
	gtk_tree_selection_get_selected(selection, &rcvdialog->model, &rcvdialog->iter);
	gtk_tree_model_get (rcvdialog->model, &rcvdialog->iter, COL_INDEX, &index, COL_NAME, &varname, -1);
	printf("choice : %d\t%s\n", index, varname);
	
	char* dir;
	tilem_config_get("download", "receivefile_recentdir/f", &dir, NULL);	
	if(!dir)
		dir = g_get_current_dir();
	
	gchar* filename = prompt_save_file("Save file", GTK_WINDOW(rcvdialog->window), varname, dir, "*.82p", "TI82 file", "*.83p", "TI83 file", "*.8xp", "TI83+ or TI84+ file", "*.8xk", "TI83+ or TI84+ falsh app", NULL); /* FIXME : add the other extension */ 
	if(filename == NULL)
		return;
	printf("Destination : %s\n", filename);
	
	dir = g_path_get_dirname(filename);
	tilem_config_set("download", "receivefile_recentdir/f", dir, NULL);
	tilem_receive_var(rcvdialog->emu, rcvdialog->emu->varapp->vlist[index], filename);
	
	//tilem_calc_emulator_receive_file(rcvdialog->emu, rcvdialog->emu->varapp->vlist[index], filename);

	
	
	g_free(varname);
 
}


static void tilem_rcvmenu_on_refresh(G_GNUC_UNUSED GtkWidget* w, G_GNUC_UNUSED gpointer data) {
	TilemReceiveDialog* rcvdialog = (TilemReceiveDialog*) data;

	
	if(rcvdialog->emu->varapp->vlist)
		g_free(rcvdialog->emu->varapp->vlist);
	if(rcvdialog->emu->varapp)
		g_free(rcvdialog->emu->varapp);
	//tilem_get_dirlist(rcvdialog->emu);
	load_entries(rcvdialog->emu);
	rcvdialog->model = fill_varlist(rcvdialog, rcvdialog->emu->varapp->vlist_utf8);
        gtk_tree_view_set_model(GTK_TREE_VIEW(rcvdialog->treeview), rcvdialog->model);	
	gtk_widget_show(GTK_WIDGET(rcvdialog->window));
}


/* This function should be used to press enter automatically to have a better synchronization between 'launching transmission" and getting vars with libtis.
   For the moment, the user need to quickly prepare the calc for transmit, then click "transmit" then very quickly OK on the popup but if he's not as fast as needed it doesn't work...
   Here is a real problem, preparing the entire transmission by clicking automatically 5, 6 or more keys is bad it's seems (and I can't get it working currently).
   But clicking just on key with this function is too fast too (even in a separate thread). Maybe you could have a solution for that...
*/

/* UNUSED CURRENTLY */
gpointer tilem_prepare_getvar_ns(gpointer data) {
	TilemCalcEmulator* emu = (TilemCalcEmulator*) data;
	
	printf("Prepare ti82 or ti85 to get var\n");

	
	tilem_calc_emulator_set_limit_speed(emu,FALSE);	
	g_mutex_lock(emu->calc_mutex);
	run_with_key(emu->calc, TILEM_KEY_ENTER);
	g_mutex_unlock(emu->calc_mutex);
	
	
	printf("End preparation\n");

	

	return NULL;
}
	
	
/* A popup wich is needed because of the fact that ti82 and ti85 need to be in the "transmit" sate to get vars */
static void on_ask_prepare_receive_response(G_GNUC_UNUSED GtkWidget* w, G_GNUC_UNUSED GtkResponseType t,   G_GNUC_UNUSED gpointer data) {
	TilemCalcEmulator* emu = (TilemCalcEmulator*) data;
	printf("on_ask_prepare_receive_response\n");

	/* If I use this function, I need to wait a little bit before launching tilem_get_dirlist_ns. 
	   But I've tried to use g_usleep or g_thread_join or twice but it does not work. 
	   I've tried to speed the core (set speed limit to false) too, and even a while (to make a pause). 
	   Nothing is correct...
	   The problem is, how to start getting vars at the good moment, not too fast, not too late...
	*/
	/*GThread* link_prepare_thread = g_thread_create(&tilem_prepare_getvar_ns, emu, TRUE, NULL); */

	/*	
	gtk_main_iteration();
	g_thread_join(link_prepare_thread);
	gtk_main_iteration();
	*/
	

	

	/*if(emu->rcvdlg)
		load_entries(emu);*/
	
	/* Waiting */
	/*long i = 0;
	while(i< 20) {
		gtk_main_iteration();
		i++;
	}*/
	
	gtk_main_iteration();
	
	/*tilem_calc_emulator_set_limit_speed(emu,TRUE); */
	/*if (!emu->link_thread)
		emu->link_thread = g_thread_create(&tilem_get_dirlist_ns, emu, TRUE, NULL);
	*/
	gtk_main_iteration();


	
	//g_thread_join(emu->link_thread); /* Do not create the menu if getting vars is not done */
	
	
	emu->rcvdlg = create_receive_menu(emu);
	//tilem_calc_emulator_set_limit_speed(emu,TRUE);	

	gtk_window_present(GTK_WINDOW(emu->rcvdlg->window));

	gtk_widget_destroy(GTK_WIDGET(w));
}
	

/* #### WIDGET CREATION #### */

/* Create a new scrolled window with sensible default settings. */
static GtkWidget *new_scrolled_window(GtkWidget *contents)
{
        GtkWidget *sw; 
        sw = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                       GTK_POLICY_AUTOMATIC,
                                       GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
                                            GTK_SHADOW_IN);
        gtk_container_add(GTK_CONTAINER(sw), contents);
        return sw;
}

/* Create the GtkTreeView to show the vars list */
static GtkWidget *create_varlist()
{
	GtkCellRenderer   *renderer;
	GtkWidget         *treeview;
	GtkTreeViewColumn *c1;
	GtkTreeViewColumn *c2;
	GtkTreeViewColumn *c3;
	GtkTreeViewColumn *c4;
	
	/* Create the stack list tree view and set title invisible */
	treeview = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(treeview), TRUE);

	/* Create the columns */
	renderer = gtk_cell_renderer_text_new ();
	c1 = gtk_tree_view_column_new_with_attributes ("INDEX", renderer, "text", COL_INDEX, NULL);
	c2 = gtk_tree_view_column_new_with_attributes ("NAME", renderer, "text", COL_NAME, NULL);
	c3 = gtk_tree_view_column_new_with_attributes ("TYPE", renderer, "text", COL_TYPE, NULL);
	c4 = gtk_tree_view_column_new_with_attributes ("SIZE", renderer, "text", COL_SIZE, NULL);

	gtk_tree_view_column_set_sizing(c1, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_expand(c1, TRUE);
	gtk_tree_view_column_set_visible(c1, FALSE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), c1);


	gtk_tree_view_column_set_sizing(c2, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_expand(c2, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), c2);

		
	gtk_tree_view_column_set_sizing(c3, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_expand(c3, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), c3);
		
	gtk_tree_view_column_set_sizing(c4, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_expand(c4, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), c4);
	
	return treeview;
}

/* Fill the list of vars. In fact, add all vars from list to a GtkListStore */
static GtkTreeModel* fill_varlist(TilemReceiveDialog * rcvdialog, char** list)
{
	
	rcvdialog->store = gtk_list_store_new (4, G_TYPE_INT, G_TYPE_STRING, G_TYPE_CHAR, G_TYPE_INT);
	int i = 0;
	if(list){
		for(i = 0; list[i]; i++) {
			/* Append a row */
			gtk_list_store_append (rcvdialog->store, &rcvdialog->iter);
			/* Fill the row */ 
			printf("list[%d] : %s\n", i, list[i]);
			gtk_list_store_set (rcvdialog->store, &rcvdialog->iter, COL_INDEX, i, COL_NAME, list[i], COL_TYPE, rcvdialog->emu->varapp->vlist[i]->type, COL_SIZE, rcvdialog->emu->varapp->vlist[i]->size, -1);
			
		}
	}
	return GTK_TREE_MODEL (rcvdialog->store);
}

/* Create a new menu for receiving vars. */
TilemReceiveDialog* create_receive_menu(TilemCalcEmulator *emu)
{
	int defwidth, defheight;

	TilemReceiveDialog* rcvdialog = g_slice_new0(TilemReceiveDialog);
	rcvdialog->emu = emu;
	emu->rcvdlg = rcvdialog;
	//GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	rcvdialog->window = gtk_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(rcvdialog->window), GTK_WINDOW(emu->ewin->window));	
	gtk_window_set_title(GTK_WINDOW(rcvdialog->window), "TilEm receive Menu");
	rcvdialog->button_refresh = gtk_dialog_add_button(GTK_DIALOG(rcvdialog->window), "Refresh", 0);
	rcvdialog->button_save = gtk_dialog_add_button(GTK_DIALOG(rcvdialog->window), "Save file to disk", 1);
	rcvdialog->button_close = gtk_dialog_add_button(GTK_DIALOG(rcvdialog->window), "Close", 2);

	/* Set the size of the dialog */
	defwidth = 200;
	defheight = 400;
	gtk_window_set_default_size(GTK_WINDOW(rcvdialog->window), defwidth, defheight);
	
	/* Create and fill tree view */
	rcvdialog->treeview = create_varlist();  	
	if(!rcvdialog->model) {
		/*tilem_get_dirlist(emu);*/
		//load_entries(emu);
		if(emu->varapp) {
			rcvdialog->model = fill_varlist(rcvdialog, emu->varapp->vlist_utf8);
		} else { 
			rcvdialog->model = fill_varlist(rcvdialog, NULL);
		}
			
        	gtk_tree_view_set_model(GTK_TREE_VIEW(rcvdialog->treeview), rcvdialog->model);	
	}

	/* Allow scrolling the list because we can't know how many vars the calc contains */
	GtkWidget * scroll = new_scrolled_window(rcvdialog->treeview);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(rcvdialog->window))), scroll);

	
	//g_signal_connect_swapped (window, "response", G_CALLBACK (gtk_widget_hide), window);
	g_signal_connect(rcvdialog->button_refresh, "clicked", G_CALLBACK (tilem_rcvmenu_on_refresh), rcvdialog);
	g_signal_connect(rcvdialog->button_save, "clicked", G_CALLBACK (tilem_rcvmenu_on_receive), rcvdialog);
	g_signal_connect(rcvdialog->button_close, "clicked", G_CALLBACK (tilem_rcvmenu_on_close), rcvdialog);
	
	
	gtk_widget_show_all(GTK_WIDGET(rcvdialog->window));

	return rcvdialog;


}

/* #### GET DATA FROM CALC AND INSERT INTO THE WIDGETS #### */

/* Use the appropriate method to load entries 
 * This function is used to fill the emu->varapp structure
 */
void load_entries(TilemCalcEmulator *emu) {

	if (emu->calc->hw.model_id == TILEM_CALC_TI81) {
		// Nothing
	} else if (emu->calc->hw.model_id == TILEM_CALC_TI82) {
		/*g_mutex_lock(emu->calc_mutex);
		prepare_for_link_send(emu->calc);
		tilem_z80_run_time(emu->calc, 50, NULL);
		g_cond_broadcast(emu->calc_wakeup_cond);
		g_mutex_unlock(emu->calc_mutex);
		*/
		tilem_get_dirlist_ns(emu);
	} else if (emu->calc->hw.model_id == TILEM_CALC_TI85) {
		/* TODO : use a atomatic prepare for link */
		tilem_get_dirlist_ns(emu);
	} else {
		tilem_get_dirlist(emu);
	}


}


/* #### ENTRY POINT #### */

/* Ask the user to to some action to have the calc in the send state */
void ask_prepare_receive (TilemCalcEmulator* emu)
{
	GtkWidget *dialog, *label, *content_area;

	/* Start to wait the transmit state */
	if (!emu->link_thread)
		emu->link_thread = g_thread_create(&tilem_get_dirlist_ns, emu, TRUE, NULL);

	/* Create the widgets */
	dialog = gtk_dialog_new_with_buttons ("Message", GTK_WINDOW(emu->ewin->window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_NONE, NULL);
	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

	/* Add the image */
	/* FIXME : need to do the ti85 version */
	char* shared = NULL;
	if (emu->calc->hw.model_id == TILEM_CALC_TI82) {
		label = gtk_label_new ("In order to transmit vars to your computer,\n ti82 needs some preparing tasks : \n\t - Firstly go to home\n\t - Then press 2nd, link\n\t - Then press enter\n\t - Then press right arrow\n\t - Press enter then very quickly click OK...");
		shared = get_shared_file_path("pixs", "prepare_ti82.gif", NULL); /* Get the gif image */
	} else if (emu->calc->hw.model_id == TILEM_CALC_TI85) {
		label = gtk_label_new ("In order to transmit vars to your computer,\n ti85 needs some preparing tasks : \n\t - Firstly go to home\n\t - Then press 2nd, link\n\t - Then press F1\n\t - Then press F5\n\t - Then press F3\n\t - Then press F1\n\t - Then very quickly click OK...");
		shared = get_shared_file_path("pixs", "prepare_ti82.gif", NULL); /* Get the gif image */
	}
	
	if(!shared) 
		return;
	
	GtkWidget* image = gtk_image_new_from_file(shared); 

	GtkWidget* vbox = gtk_vbox_new(FALSE, 10);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), image);
	gtk_box_pack_end_defaults(GTK_BOX(vbox), image);
	gtk_container_add(GTK_CONTAINER(content_area), vbox);
	gtk_widget_show(vbox);

	/* Ensure that the dialog box is destroyed when the user responds */
	gtk_signal_connect (GTK_OBJECT (dialog), "response", GTK_SIGNAL_FUNC (on_ask_prepare_receive_response), emu);

	gtk_container_add (GTK_CONTAINER (content_area), label);
	gtk_widget_show_all (dialog);
	
	
}


/* Popup the receive window */
/* This is the entry point */
void popup_receive_menu(TilemEmulatorWindow *ewin)
{
	TilemReceiveDialog* rcvdlg;

	g_return_if_fail(ewin != NULL);
	g_return_if_fail(ewin->emu != NULL);


	if (ewin->emu->calc->hw.model_id == TILEM_CALC_TI81) {
	} else if (ewin->emu->calc->hw.model_id == TILEM_CALC_TI82) {
		ask_prepare_receive(ewin->emu); /* This function will create the receive menu when preparation is ok */
	} else if (ewin->emu->calc->hw.model_id == TILEM_CALC_TI85) {
		ask_prepare_receive(ewin->emu); /* This function will create the receive menu when preparation is ok */
	} else {
		if(!ewin->emu->rcvdlg)
			load_entries(ewin->emu);
		ewin->emu->rcvdlg = create_receive_menu(ewin->emu);
		rcvdlg = ewin->emu->rcvdlg;

		gtk_window_present(GTK_WINDOW(rcvdlg->window));
	}
}






