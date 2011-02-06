#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include "gui.h"

/* Used when you load another skin */
void redraw_screen(GLOBAL_SKIN_INFOS *gsi)
{
	GtkWidget *pImage;

	skin_unload(gsi->si);
	skin_load(gsi->si, gsi->SkinFileName);

	if (gsi->pAf)
		gtk_widget_destroy(gsi->pAf);
	if (gsi->pLayout)
		gtk_widget_destroy(gsi->pLayout);

	gsi->pAf = create_draw_area(gsi);
	gsi->pLayout = gtk_layout_new(NULL, NULL);

	if (gsi->view == 0) {
		pImage = gtk_image_new_from_pixbuf(gsi->si->image);
		gtk_layout_put(GTK_LAYOUT(gsi->pLayout), pImage, 0, 0);

		gtk_layout_put(GTK_LAYOUT(gsi->pLayout), gsi->pAf,
		               gsi->si->lcd_pos.left,
		               gsi->si->lcd_pos.top);

		gtk_window_resize(GTK_WINDOW(gsi->pWindow), gsi->si->width,
		                  gsi->si->height);
	}
	else {
		int screenwidth = gsi->si->lcd_pos.right - gsi->si->lcd_pos.left;
		int screenheight = gsi->si->lcd_pos.bottom - gsi->si->lcd_pos.top;

		gtk_layout_put(GTK_LAYOUT(gsi->pLayout), gsi->pAf, 0, 0);

		gtk_window_resize(GTK_WINDOW(gsi->pWindow), screenwidth, screenheight);
	}

	gtk_widget_add_events(gsi->pLayout, (GDK_BUTTON_PRESS_MASK
	                                     | GDK_BUTTON_RELEASE_MASK
	                                     | GDK_BUTTON1_MOTION_MASK
	                                     | GDK_POINTER_MOTION_HINT_MASK));

	g_signal_connect(gsi->pLayout, "button-press-event",
	                 G_CALLBACK(mouse_press_event), gsi);
	g_signal_connect(gsi->pLayout, "motion-notify-event",
	                 G_CALLBACK(pointer_motion_event), gsi);
	g_signal_connect(gsi->pLayout, "button-release-event",
	                 G_CALLBACK(mouse_release_event), gsi);

	gtk_container_add(GTK_CONTAINER(gsi->pWindow), gsi->pLayout);

	gtk_widget_show_all(gsi->pWindow);
}

/* Switch between skin and LCD-only mode */
void switch_view(GLOBAL_SKIN_INFOS * gsi)
{
	gsi->view = !gsi->view;
	redraw_screen(gsi);
}

/* Display the lcd image into the terminal */
void display_lcdimage_into_terminal(GLOBAL_SKIN_INFOS* gsi)  /* Absolument necessaire */
{
	
	int width, height;
	guchar* lcddata;
	int x, y;
	char c;
	width = gsi->emu->calc->hw.lcdwidth;
	height = gsi->emu->calc->hw.lcdheight;
	FILE* lcd_content_file;
	/* Alloc mmem */
	lcddata = g_new(guchar, (width / 8) * height);
		
	/* Get the lcd content using the function 's pointer from Benjamin's core */
	(*gsi->emu->calc->hw.get_lcd)(gsi->emu->calc, lcddata);
		
	/* Print a little demo just for fun;) */
	printf("\n\n\n");	
	printf("	 r     rr    r  rrrrr  rrr  r     rrrrr r   r  rr    r    rr     r                      \n");
	printf("  r     r     r     r     r     r   r     r     rr rr    r    r     r     r     r     r     r   \n");
	printf("   r   r      r    r      r     r   r     r     r r r   r      r    r      r     r     r     r  \n");
	printf("rrrrr r      r     r      r     r   r     rrrr  r r r  r       r     r      r rrrrr rrrrr rrrrr \n");
	printf("   r   r      r    r      r     r   r     r     r   r  rrr     r    r      r     r     r     r  \n");
	printf("  r     r     r     r     r     r   r     r     r   r         r     r     r     r     r     r   \n");
	printf("	 r     rr    r    r    rrr  rrrrr rrrrr r   r        r    rr     r                      \n");
	printf("\n(Here is just a sample...)\n\n");	
	
	/* Request user to know which char user wants */	
	
	printf("Which char to display FOR BLACK?\n");
	scanf("%c", &c); /* Choose wich char for the black */	
	
	//printf("Which char to display FOR GRAY ?\n");
	//scanf("%c", &b); /* Choose wich char for the black */	
	
	lcd_content_file = g_fopen("lcd_content.txt", "w");

	printf("\n\n\n### LCD CONTENT ###\n\n\n\n");
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			/*printf("%d ", lcddata[y * width + x]); */	
			if (lcddata[(y * width + x) / 8] & (0x80 >> (x % 8))) {
				printf("%c", c);
				if(lcd_content_file != NULL)	
					fprintf(lcd_content_file,"%c", c);
			} else {
				printf("%c", ' ');
				if(lcd_content_file != NULL)	
					fprintf(lcd_content_file,"%c", ' ');
			}
		}
		printf("\n");
		if(lcd_content_file != NULL)	
			fprintf(lcd_content_file,"%c", '\n');
	}	
	if(lcd_content_file != NULL) {	
		fclose(lcd_content_file);
		printf("\n### END ###\n\nSaved into lcd_content.txt (You're really geek!)");
	}	

}

#define MICROSEC_PER_FRAME 10000

/* Thread for the gui */
static gpointer core_thread(gpointer data)
{
	TilemCalcEmulator* emu = data;
	GTimer* tmr;
	gulong tnext, tcur;

	tmr = g_timer_new();
	g_timer_start(tmr);

	g_timer_elapsed(tmr, &tnext);

	while (1) {
		g_mutex_lock(emu->run_mutex);
		if (emu->exiting) {
			g_mutex_unlock(emu->run_mutex);
			break;
		}
		g_mutex_unlock(emu->run_mutex);
		
		g_mutex_lock(emu->calc_mutex);
		tilem_z80_run_time(emu->calc, MICROSEC_PER_FRAME, NULL);
		g_mutex_unlock(emu->calc_mutex);

		g_mutex_lock(emu->lcd_mutex);
		tilem_gray_lcd_next_frame(emu->glcd, 0);
		g_mutex_unlock(emu->lcd_mutex);

		g_timer_elapsed(tmr, &tcur);
		tnext += MICROSEC_PER_FRAME;
		if (tnext - tcur < MICROSEC_PER_FRAME)
			g_usleep(tnext - tcur);
		else
			tnext = tcur;
	}

	g_timer_destroy(tmr);
	return 0;
}

/* Set the color palette for drawing the emulated LCD. */
void screen_restyle(GtkWidget* w, GtkStyle* oldstyle G_GNUC_UNUSED,
		    GLOBAL_SKIN_INFOS* gsi)
{
	TilemCalcEmulator* emu = gsi->emu;
	dword* palette;
	int r_dark, g_dark, b_dark;
	int r_light, g_light, b_light;
	double gamma = 2.2;

	if (gsi->view == 1 || !gsi->si) {
		/* no skin -> use standard GTK colors */

		r_dark = w->style->text[GTK_STATE_NORMAL].red / 257;
		g_dark = w->style->text[GTK_STATE_NORMAL].green / 257;
		b_dark = w->style->text[GTK_STATE_NORMAL].blue / 257;

		r_light = w->style->base[GTK_STATE_NORMAL].red / 257;
		g_light = w->style->base[GTK_STATE_NORMAL].green / 257;
		b_light = w->style->base[GTK_STATE_NORMAL].blue / 257;
	}
	else {
		/* use skin colors */

		r_dark = ((gsi->si->lcd_black >> 16) & 0xff);
		g_dark = ((gsi->si->lcd_black >> 8) & 0xff);
		b_dark = (gsi->si->lcd_black & 0xff);

		r_light = ((gsi->si->lcd_white >> 16) & 0xff);
		g_light = ((gsi->si->lcd_white >> 8) & 0xff);
		b_light = (gsi->si->lcd_white & 0xff);
	}

	/* Generate a new palette, and convert it into GDK format */

	if (emu->lcd_cmap)
		gdk_rgb_cmap_free(emu->lcd_cmap);

	palette = tilem_color_palette_new(r_light, g_light, b_light,
					  r_dark, g_dark, b_dark, gamma);
	emu->lcd_cmap = gdk_rgb_cmap_new(palette, 256);
	tilem_free(palette);

	gtk_widget_queue_draw(emu->lcdwin);
}

gboolean screen_repaint(GtkWidget *w, GdkEventExpose *ev G_GNUC_UNUSED,
			GLOBAL_SKIN_INFOS *gsi)
{
	TilemCalcEmulator* emu = gsi->emu;
	int width, height;

	width = w->allocation.width;
	height = w->allocation.height;

	/* If image buffer is not the correct size, allocate a new one */

	if (!emu->lcd_image_buf
	    || width != emu->lcd_image_width
	    || height != emu->lcd_image_height) {
		emu->lcd_image_width = width;
		emu->lcd_image_height = height;
		g_free(emu->lcd_image_buf);
		emu->lcd_image_buf = g_new(byte, width * height);
	}

	/* Draw LCD contents into the image buffer */

	g_mutex_lock(emu->lcd_mutex);
	tilem_gray_lcd_draw_image_indexed(emu->glcd, emu->lcd_image_buf,
					  width, height, width,
					  TILEM_SCALE_SMOOTH);
	g_mutex_unlock(emu->lcd_mutex);

	/* Render buffer to the screen */

	gdk_draw_indexed_image(w->window, w->style->fg_gc[w->state],
			       0, 0, width, height, GDK_RGB_DITHER_NONE,
			       emu->lcd_image_buf, width, emu->lcd_cmap);
	return TRUE;
}

/* Update the lcd */
gboolean screen_update(gpointer data)
{
	DLCD_L2_A0(">screen_update\n");
	TilemCalcEmulator* emu = data;
	gtk_widget_queue_draw(emu->lcdwin);
	DLCD_L2_A0("<screen_update\n");
	return TRUE;
}



void create_menus(GtkWidget *window,GdkEvent *event, GtkItemFactoryEntry * menu_items, int thisitems, const char *menuname,gpointer* gsi)
{
	
	DLCD_L2_A0("Entering : create_menus...\n");
	GtkAccelGroup *accel_group;
	GtkItemFactory *factory;
	GtkWidget *menu;
	GdkEventButton *bevent = (GdkEventButton *) event;


	accel_group = gtk_accel_group_new();
	factory = gtk_item_factory_new(GTK_TYPE_MENU, menuname, accel_group);
	/* translatefunc */
	gtk_item_factory_create_items(factory, thisitems, menu_items, gsi);
	menu = factory->widget;

	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, bevent->button, bevent->time);
	gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
	DLCD_L2_A0("Exiting create_menus...\n");

}



GtkWidget* draw_screen(GLOBAL_SKIN_INFOS *gsi)  
{
	//g_thread_init(NULL);
	
	DLCD_L0_A0("**************** fct : draw_screen *********************\n");
	DLCD_L0_A0("*  - load skin                                         *\n");
	DLCD_L0_A0("*  - create GtkLayout                                  *\n");
	DLCD_L0_A0("*  - add skin, add lcd area                            *\n");
	DLCD_L0_A0("*  - connect events (callback)                         *\n");
	DLCD_L0_A0("*  - print top level window                            *\n");
	DLCD_L0_A0("*  - launch thread                                     *\n");
	DLCD_L0_A0("********************************************************\n");
	GtkWidget *pAf;
	GThread *th;

	gsi->view=0;
	/* LOAD A SKIN */
	SKIN_INFOS *si;
	si=malloc(sizeof(SKIN_INFOS));
	gsi->si=(SKIN_INFOS*)si;
	skin_load(gsi->si,gsi->SkinFileName);
	
	/* Create the window */
	GtkWidget *pWindow,  *pImage, *pLayout;
	gsi->pWindow = pWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);	// GTK_WINDOW_LEVEL : define how is the window 
	gtk_window_set_title(GTK_WINDOW(pWindow),"TilEm");	// define title of the window 
	gtk_window_set_position(GTK_WINDOW(pWindow),GTK_WIN_POS_CENTER); // GTK_WIN_POS_CENTER : define how the window is displayed 
	gtk_window_set_default_size(GTK_WINDOW(pWindow),gsi->si->width,gsi->si->height);	// define size of the window
	

	pImage=gtk_image_new_from_pixbuf(gsi->si->image);
	
	g_signal_connect(G_OBJECT(pWindow),"destroy",G_CALLBACK(on_destroy),NULL); 
	gtk_widget_add_events(pWindow, (GDK_KEY_PRESS_MASK
	                                | GDK_KEY_RELEASE_MASK));
	g_signal_connect(pWindow, "key-press-event",
	                 G_CALLBACK(key_press_event), gsi);
	g_signal_connect(pWindow, "key-release-event",
	                 G_CALLBACK(key_release_event), gsi);
	
	/* Create the draw area */
	pAf=create_draw_area(gsi);
	
	/* Add the lcd to the pWindow using a GtkLayout */
	pLayout=gtk_layout_new(NULL,NULL);
	gsi->pLayout=pLayout;
	gtk_widget_show(pAf);
	gtk_layout_put(GTK_LAYOUT(pLayout),pImage,0,0);
	gtk_layout_put(GTK_LAYOUT(pLayout),pAf,gsi->si->lcd_pos.left,gsi->si->lcd_pos.top);
	
	g_signal_connect(gsi->emu->lcdwin, "style-set", G_CALLBACK(screen_restyle), gsi); 
	gtk_widget_add_events(pLayout, GDK_BUTTON_PRESS_MASK);	
	gtk_signal_connect(GTK_OBJECT(pLayout), "button-press-event", G_CALLBACK(mouse_press_event),gsi);
	gtk_widget_add_events(pLayout, (GDK_BUTTON1_MOTION_MASK
	                                | GDK_POINTER_MOTION_HINT_MASK));
	gtk_signal_connect(GTK_OBJECT(pLayout), "motion-notify-event", G_CALLBACK(pointer_motion_event),gsi);
	gtk_widget_add_events(pLayout, GDK_BUTTON_RELEASE_MASK);	
	gtk_signal_connect(GTK_OBJECT(pLayout), "button-release-event", G_CALLBACK(mouse_release_event), gsi); 
	g_signal_connect(GTK_OBJECT(gsi->emu->lcdwin), "expose-event",G_CALLBACK(screen_repaint), gsi);
	gtk_container_add(GTK_CONTAINER(pWindow),pLayout);

	/* Set up color palette */
	screen_restyle(gsi->emu->lcdwin, NULL, gsi);
	
	gtk_widget_show_all(pWindow);	/* display the window and all that it contains. */
	
	/* THREAD */
	th = g_thread_create(&core_thread, gsi->emu, TRUE, NULL);
	g_timeout_add(50, screen_update, gsi->emu);
	


	return pWindow;
}



GtkWidget * create_draw_area(GLOBAL_SKIN_INFOS * gsi) 
{
	

	GtkWidget *pAf;
	/* Get the size of the lcd area in the SKIN_INFOS struct */
	int screenwidth=gsi->si->lcd_pos.right-gsi->si->lcd_pos.left;	
	int screenheight=gsi->si->lcd_pos.bottom-gsi->si->lcd_pos.top; 
	
	
	DLCD_L0_A0("**************** fct : create_draw_area ****************\n");
	DLCD_L0_A1("*  screenwidth = %d                                   *\n",screenwidth);
	DLCD_L0_A1("*  screenheight = %d                                  *\n",screenheight);
	DLCD_L0_A0("********************************************************\n");
	pAf = gtk_aspect_frame_new(NULL, 0.5, 0.5, 1.0, TRUE);	
                         gtk_frame_set_shadow_type(GTK_FRAME(pAf),GTK_SHADOW_NONE);
                         {
                                 gsi->emu->lcdwin = gtk_drawing_area_new();
                                 gtk_widget_set_name(gsi->emu->lcdwin, "tilem-lcd");
                                 gtk_widget_set_size_request(gsi->emu->lcdwin,screenwidth,screenheight);
                                 gtk_container_add(GTK_CONTAINER(pAf),gsi->emu->lcdwin);
                                 gtk_widget_show(gsi->emu->lcdwin);
                       }
	g_signal_connect(gsi->emu->lcdwin, "expose-event",
	                 G_CALLBACK(screen_repaint), gsi);
	g_signal_connect(gsi->emu->lcdwin, "style-set",
	                 G_CALLBACK(screen_restyle), gsi);

	return pAf;
}






