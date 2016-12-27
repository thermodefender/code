#include <gtk/gtk.h>
#include <stdio.h>
#include <stdbool.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>
//#include "MagickCore.h"
#include "wand/magick-wand.h"

#include "monitor.h"
#include "tdio.h"
//#include "qdbmp.h"



// --- UI Global Variables
GMainContext *mainc;
GtkApplication *app_ui;
GtkWindow *main_window;
GtkLabel *statusLabel;
GtkButton *monitorButton;
GtkImage *captureImage;
GtkDrawingArea *videoArea;




 typedef struct {
	GtkEntry *waterDiff;
	GtkEntry *bodyDiff;
	GtkEntry *fireDiff;
	GtkEntry *minWaterPC;
	GtkEntry *minBodyPC;
	GtkEntry *minFirePC;
} VariableEntries;

pthread_t tid[2];
bool _active = false;


// --- Forward Declarations
//static void toggle_status (GtkWidget *widget, gpointer *data);

void set_capture_image_from_current_array(GtkImage *image)
{
	char* imageName = save_pgm_file();
	
	// Scale the image up and display it (using ImageMagick commands)
	char largeImageName[34];
	snprintf(largeImageName, sizeof largeImageName, "l_%s", imageName);
	
	char scaleCommand[100];
	snprintf(scaleCommand, sizeof scaleCommand, "convert %s -resize 600%% %s", imageName, largeImageName);
	system(scaleCommand);
	
	gtk_image_set_from_file (image, largeImageName);	
}

static void capture_image (GtkWidget *widget, gpointer *data)
{
	int fd = connect_to_lepton();
	while(transfer(fd)!=59){}
	close(fd);
	set_capture_image_from_current_array(captureImage);
}
/*

gboolean video_area_expose (GtkWidget *da, GdkEvent *event, gpointer data)
void video_area_expose (GtkWidget *da, BMP *bmp)
{
	
	//(void)event; void(data);
	GdkPixbuf *pix;
	GError *err = NULL;
		
	MagickWand *m_wand = NULL;

	MagickWandGenesis();

	// Create a wand
	m_wand = NewMagickWand();

	// Read the input image
	MagickReadImage(m_wand,"no-video.gif");
	
	IplImage *ocvImage;
	//ocvImage = cvLoadImage("no-video.gif",1);
	
	pix = gdk_pixbuf_new_from_data (
			(guchar*)ocvImage->imageData,
			GDK_COLORSPACE_RGB,
			FALSE,
			ocvImage->depth,
			ocvImage->width,
			ocvImage->height,
			(ocvImage->widthStep),
			NULL,
			NULL);
	
	
	//if(data == NULL){
		pix = gdk_pixbuf_new_from_file ("no-video.gif", &err);
	//}
 	if(m_wand) m_wand = DestroyMagickWand(m_wand);

	MagickWandTerminus();
	//	pix = gdk_pixbuf_new_from_file ("capture.bmp", &err);
		
	//else
	//{
		//
		//UCHAR *pixData = (UCHAR*)data;
		
		//BMP *bmp = (BMP*)data;
		//BMP *bmp = BMP_ReadFile( "capture.bmp" );
		
		//UCHAR *pixels = (UCHAR*)data;
		
		pix = gdk_pixbuf_new_from_data ((guchar*)BMP_GetBytes(bmp),
			(guchar*)block,
			GDK_COLORSPACE_RGB,
			FALSE,
			8,80,60,240,
			pix_destroy,
			bmp);
	//}
    cairo_t *cr;
    cr = gdk_cairo_create (gtk_widget_get_window(da));
    //    cr = gdk_cairo_create (da->window);
    gdk_cairo_set_source_pixbuf(cr, pix, 0, 10);
    cairo_paint(cr);
    //    cairo_fill (cr);
    cairo_destroy (cr);
	
	BMP_Free(bmp);
	//return TRUE;
	
	
	
	
}
*/



/*********************************** Calibration & Settings **********************************/
// <editor-fold>

//static void save_settings (GtkWidget *widget, VariableEntries *data)
static void save_settings (GtkWidget *widget, gpointer data)
{

	//VariableEntries *d = (VariableEntries*)data;
	
	water_tc_differential = (int)strtol(gtk_entry_get_text (vEntries.waterDiff), (char **) NULL, 10);
	body_tc_differential = (int)strtol(gtk_entry_get_text (vEntries.bodyDiff), (char **) NULL, 10);
	fire_tc_differential = (int)strtol(gtk_entry_get_text (vEntries.fireDiff), (char **) NULL, 10);
	water_min_detected_pc = (int)strtol(gtk_entry_get_text (vEntries.minWaterPC), (char **) NULL, 10);
	body_min_detected_pc = (int)strtol(gtk_entry_get_text (vEntries.minBodyPC), (char **) NULL, 10);
	fire_min_detected_pc = (int)strtol(gtk_entry_get_text (vEntries.minFirePC), (char **) NULL, 10);
	
	printf("Water Diff: %d\nBody Diff: %d\nFire Diff: %d\n", water_tc_differential, body_tc_differential, fire_tc_differential);
	printf("Water PC: %d\nBody PC: %d\nFire PC: %d\n", water_min_detected_pc, body_min_detected_pc, fire_min_detected_pc);

}

static void calibrate_water (GtkWidget *widget, gpointer *data)
{
	set_reference_frame();
	
	GtkWidget *dialog;
	GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
	dialog = gtk_message_dialog_new (main_window,
									 flags,
									 GTK_MESSAGE_INFO,
									 GTK_BUTTONS_OK,
									 "Reference Image Set.\nPlace the water in the camera frame click 'OK'");
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	
	// Get the thermal count difference
	int tc_diff = get_tc_difference(40, water_min_detected_pc); // Water
	
	if(tc_diff != 0){
		gchar varStr[8];
		
		if(tc_diff < 0)
			sprintf(varStr, "%d", tc_diff + 10);
		else
			sprintf(varStr, "%d", tc_diff - 5);
		
		gtk_entry_set_text(vEntries.waterDiff, varStr);
	} else {
		printf("Could not detect difference.");	
	}	
}


static void calibrate_body (GtkWidget *widget, gpointer *data)
{
	set_reference_frame();
	
	GtkWidget *dialog;
	GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
	dialog = gtk_message_dialog_new (main_window,
									 flags,
									 GTK_MESSAGE_INFO,
									 GTK_BUTTONS_OK,
									 "Reference Image Set.\nPlace your hand in the camera frame click 'OK'");
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	
	// Get the thermal count difference
	int tc_diff = get_tc_difference(100, body_min_detected_pc);
		
	if(tc_diff != 0){
		gchar varStr[8];
		sprintf(varStr, "%d", tc_diff - 5); // Reducing the difference since the average is returned.
		gtk_entry_set_text(vEntries.bodyDiff, varStr);
	} else {
		printf("Could not detect difference.");	
	}	
}


static void calibrate_fire (GtkWidget *widget, gpointer *data)
{
	set_reference_frame();
	
	GtkWidget *dialog;
	GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
	dialog = gtk_message_dialog_new (main_window,
									 flags,
									 GTK_MESSAGE_INFO,
									 GTK_BUTTONS_OK,
									 "Reference Image Set.\nPlace some fire in the camera frame click 'OK'");
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	
	// Get the thermal count difference
	int tc_diff = get_tc_difference(300, fire_min_detected_pc); 
			
	if(tc_diff != 0){
		gchar varStr[8];
		sprintf(varStr, "%d", tc_diff - 20); // Reducing the difference since the average is returned.
		gtk_entry_set_text(vEntries.fireDiff, varStr);
	} else {
		printf("Could not detect difference.");	
	}	
}



static void done_settings (GtkWidget *widget, gpointer window)
{
	gtk_widget_destroy(GTK_WIDGET(window));
}


static void show_settings(GtkWidget *widget, gpointer *data)
{
	
  GtkBuilder *builder;
  GObject *window;
  GObject *button;
  GObject *entry;
  
  builder = gtk_builder_new ();
  //gtk_builder_set_application(builder, app_ui);
  gtk_builder_add_from_file (builder, "builder-settings.xml", NULL);
  
  window = gtk_builder_get_object (builder, "settingsWindow");
  gtk_window_set_transient_for ((GtkWindow*)window, main_window);
  gtk_window_set_keep_above((GtkWindow*)window, TRUE);
  gtk_window_set_modal((GtkWindow*)window, TRUE);
  
  //--- Detection Variables
    
  gchar varStr[8];
  sprintf(varStr, "%d", water_tc_differential);
  vEntries.waterDiff = (GtkEntry*)gtk_builder_get_object (builder, "eWaterTcDiff");
  gtk_entry_set_text(vEntries.waterDiff, varStr);
  
  sprintf(varStr, "%d", body_tc_differential);
  vEntries.bodyDiff = (GtkEntry*)gtk_builder_get_object (builder, "eBodyTcDiff");
  gtk_entry_set_text(vEntries.bodyDiff, varStr);
  
  sprintf(varStr, "%d", fire_tc_differential);
  vEntries.fireDiff = (GtkEntry*)gtk_builder_get_object (builder, "eFireTcDiff");
  gtk_entry_set_text(vEntries.fireDiff, varStr);
    
  sprintf(varStr, "%d", water_min_detected_pc);
  vEntries.minWaterPC = (GtkEntry*)gtk_builder_get_object (builder, "eWaterPC");
  gtk_entry_set_text(vEntries.minWaterPC, varStr);
  
  sprintf(varStr, "%d", body_min_detected_pc);
  vEntries.minBodyPC = (GtkEntry*)gtk_builder_get_object (builder, "eBodyPC");
  gtk_entry_set_text(vEntries.minBodyPC, varStr);
  
  sprintf(varStr, "%d", fire_min_detected_pc);
  vEntries.minFirePC = (GtkEntry*)gtk_builder_get_object (builder, "eFirePC");
  gtk_entry_set_text(vEntries.minFirePC, varStr);
  
  //--- Save
  button = gtk_builder_get_object (builder, "btnSaveSettings");
  g_signal_connect (button, "clicked", G_CALLBACK (save_settings), NULL);  
  
  
  //--- Calibrate Variables
  button = gtk_builder_get_object (builder, "btnCalibrateWater");
  g_signal_connect (button, "clicked", G_CALLBACK (calibrate_water), NULL);
  
  button = gtk_builder_get_object (builder, "btnCalibrateBody");
  g_signal_connect (button, "clicked", G_CALLBACK (calibrate_body), NULL);
  
  button = gtk_builder_get_object (builder, "btnCalibrateFire");
  g_signal_connect (button, "clicked", G_CALLBACK (calibrate_fire), NULL);
  
  
  //--- Done
  button = gtk_builder_get_object (builder, "btnDoneSettings");
  g_signal_connect (button, "clicked", G_CALLBACK (done_settings), (GtkWindow*)window);
  
  gtk_widget_show ((GtkWidget*)window);
}

// </editor-fold>


/*********************************** Capture **********************************/
// <editor-fold>

void set_capture_image_from_current_array(GtkImage *image)
{
	char* imageName = save_pgm_file();
	
	// Scale the image up and display it (using ImageMagick commands)
	char largeImageName[34];
	snprintf(largeImageName, sizeof largeImageName, "l_%s", imageName);
	
	char scaleCommand[100];
	snprintf(scaleCommand, sizeof scaleCommand, "convert %s -resize 600%% %s", imageName, largeImageName);
	system(scaleCommand);
	
	gtk_image_set_from_file (image, largeImageName);	
}


static void capture_image (GtkWidget *widget, gpointer *data)
{
	int fd = connect_to_lepton();
	while(transfer(fd)!=59){}
	close(fd);
	set_capture_image_from_current_array(captureImage);
}

// </editor-fold>


/*********************************** Monitor **********************************/
// <editor-fold>

void update_monitor_status_labels(char *labelValue)
{	
	if(!_active){
		if(labelValue == NULL)
			gtk_label_set_text(statusLabel, "Inactive");
		else
			gtk_label_set_text(statusLabel, labelValue);
		
		gtk_button_set_label(monitorButton, "Monitor");
	} else {
		if(labelValue == NULL)
			gtk_label_set_text(statusLabel, "Monitoring");
		else
			gtk_label_set_text(statusLabel, labelValue);
		
		gtk_button_set_label(monitorButton, "Stop");
	}
}


static void toggle_monitor (GtkWidget *widget, gpointer *data)
{	
	_active = !_active;
	update_monitor_status_labels(NULL);
	
	if(_active && !_monitorActive) {
		
		// Spin up thread to start monitoring
		int err;
		err = pthread_create(&(tid[0]), NULL, &f_monitor, NULL);
		if(err != 0)
			printf("\nThreading error: [%s]", strerror(err));
		else
			printf("\nMonitoring started\n");
	}
}

// </editor-fold>



/*
void resize_image(void)
{
	MagickWand *m_wand = NULL;

	int width, height;
	
	MagickWandGenesis();

	// Create a wand
	m_wand = NewMagickWand();

	// Read the input image
	MagickReadImage(m_wand,"capture.bmp");
	
	width = MagickGetImageWidth(m_wand) * 6;
	height = MagickGetImageHeight(m_wand) * 6;
	
	MagickResizeImage(m_wand,width,height,LanczosFilter,1);
	
	MagickSetImageCompressionQuality(m_wand,95);
	
	
	// write it 
	MagickWriteImage(m_wand,"capture.jpg");

	// Tidy up
 	if(m_wand) m_wand = DestroyMagickWand(m_wand);

	MagickWandTerminus();
}
*/

/*
void generate_image_from_pixels()
{
	MagickWand *m_wand = NULL;
	PixelWand *p_wand = NULL;
	PixelIterator *iterator = NULL;
	PixelWand **pixels = NULL;
	int x,y,gray;
	char hex[128];

	MagickWandGenesis();

	p_wand = NewPixelWand();
	PixelSetColor(p_wand,"white");
	m_wand = NewMagickWand();
	// Create a 100x100 image with a default of white
	MagickNewImage(m_wand,100,100,p_wand);
	// Get a new pixel iterator 
	iterator=NewPixelIterator(m_wand);
	for(y=0;y<100;y++) {
		// Get the next row of the image as an array of PixelWands
		pixels=PixelGetNextIteratorRow(iterator,&x);
		// Set the row of wands to a simple gray scale gradient
		for(x=0;x<100;x++) {
			gray = x*255/100;
			sprintf(hex,"#%02x%02x%02x",gray,gray,gray);
			PixelSetColor(pixels[x],hex);
		}
		// Sync writes the pixels back to the m_wand
		PixelSyncIterator(iterator);
	}
	MagickWriteImage(m_wand,"bits_demo.gif");

	// Clean up
	iterator=DestroyPixelIterator(iterator);
	DestroyMagickWand(m_wand);
	MagickWandTerminus();

}
*/



/*
void test_wand()
{
	MagickWand *m_wand = NULL;

	MagickWandGenesis();

	// Create a wand
	m_wand = NewMagickWand();

	// Read the input image
	MagickReadImage(m_wand,"no-video.gif");
	
	
	
	
	
	

	// Tidy up
 	if(m_wand) m_wand = DestroyMagickWand(m_wand);

	MagickWandTerminus();

}
*/




/*
int main (int argc, char *argv[])
{

	test_wand();
	return 0;
}
*/



int main (int argc, char *argv[])
{
  GtkBuilder *builder;
  GObject *window;
  GObject *button;
  GObject *grid;
  GObject *layout;
  GtkWidget *bgImage;
  
  
  // Initialize the IO
  initializeGpio();
  
  //--- Construct a GtkBuilder instance and load our UI description
  gtk_init (&argc, &argv);
  mainc = g_main_context_default();
  app_ui = gtk_application_new ("org.gnome.example", G_APPLICATION_FLAGS_NONE);
  
  builder = gtk_builder_new ();
  gtk_builder_set_application(builder, app_ui);
  gtk_builder_add_from_file (builder, "builder.xml", NULL);
  //gtk_builder_add_from_file (builder, "builder2.glade", NULL);
    
  //--- Main Window
  window = gtk_builder_get_object (builder, "mainWindow");
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  main_window = (GtkWindow*)window;
  //gtk_window_fullscreen(main_window);
  
  // -- Set the background image of the window.
  //layout = gtk_builder_get_object (builder, "mainLayout");
  //gtk_container_add(GTK_CONTAINER(window), (GtkWidget*)layout);
  //bgImage = gtk_image_new_from_file("demo_bg.png");
  //gtk_layout_put(GTK_LAYOUT(layout), bgImage, 0, 0);  
  
  
  // --- Grid
  grid = gtk_builder_get_object (builder, "grid");
  gtk_widget_set_halign((GtkWidget*)grid, GTK_ALIGN_CENTER);
  gtk_widget_set_valign((GtkWidget*)grid, GTK_ALIGN_CENTER);
  
    
  button = gtk_builder_get_object (builder, "btnGpioOn");
  g_signal_connect (button, "clicked", G_CALLBACK (gpio_testing), (gpointer)true);
  
  button = gtk_builder_get_object (builder, "btnGpio16");
  g_signal_connect (button, "clicked", G_CALLBACK (toggle_gpio_16), (gpointer)true);
  //--- Capture Image
  captureImage = (GtkImage*)gtk_builder_get_object (builder, "captureImage");
  gtk_image_set_from_file (captureImage, "demo_bg_small.png");
  //gtk_widget_set_halign ((GtkWidget*)captureImage, GTK_ALIGN_CENTER);
  button = gtk_builder_get_object (builder, "btnCaptureFrame");
  g_signal_connect (button, "clicked", G_CALLBACK (capture_image), captureImage);
  
  // --- Video Area
  videoArea = (GtkImage*)gtk_builder_get_object (builder, "videoArea");
  gtk_widget_set_size_request ((GtkWidget*)videoArea, 500, 380);
  g_signal_connect (videoArea, "draw", G_CALLBACK (video_area_expose), NULL);

  
  //--- Monitor toggle & status
  statusLabel = (GtkLabel*)gtk_builder_get_object (builder, "statuslabel");
  monitorButton = (GtkButton*)gtk_builder_get_object (builder, "btnStartMonitor");
  g_signal_connect (monitorButton, "clicked", G_CALLBACK (toggle_monitor), NULL);
  
  
  //--- Settings
  button = gtk_builder_get_object (builder, "btnShowSettings");
  g_signal_connect (button, "clicked", G_CALLBACK (show_settings), NULL);
  //--- Quit
  button = gtk_builder_get_object (builder, "quit");
  g_signal_connect (button, "clicked", G_CALLBACK (gtk_main_quit), NULL);


  
  
  
  gtk_main ();
 
  return 0;
}
