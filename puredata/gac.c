// ==============================================================================
//	gac.c
//	
//	pd-Interface to [ 11h11 | Gac ]
//  Adapted by: Patrick Sebastien Coulombe
//	Website:	http://www.workinprogress.ca/guitare-a-crayon
//
//	Original Author:	Michael Egger
//	Copyright:	2007 [ a n y m a ]
//	Website:	http://gnusb.sourceforge.net/
//	
//	License:	GNU GPL 2.0 www.gnu.org
//	Version:	2009-04-11
// ==============================================================================
// ==============================================================================

#include "m_pd.h"
#include <usb.h> //http://libusb-win32.sourceforge.net
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pthread.h"


// ==============================================================================
// Constants
// ------------------------------------------------------------------------------
#define USBDEV_SHARED_VENDOR    	0x16c0  /* VOTI */
#define USBDEV_SHARED_PRODUCT   	0x05dc  /* Obdev's free shared PID */
#define DEFAULT_CLOCK_INTERVAL		10      /* ms */
#define OUTLETS 					17
#define USBREPLYBUFFER 				21

unsigned char buffer[USBREPLYBUFFER]; //accessible everywhere

// ==============================================================================
// Our External's Memory structure
// ------------------------------------------------------------------------------
typedef struct _gac				// defines our object's internal variables for each instance in a patch
{
	t_object 		p_ob;					// object header - ALL pd external MUST begin with this...
	usb_dev_handle	*dev_handle;			// handle to the gac usb device
	void			*m_clock;				// handle to our clock
	double 			m_interval;				// clock interval for polling edubeat
	double 			m_interval_bak;			// backup clock interval for polling edubeat
	int				is_running;				// is our clock ticking?
	void 			*outlets[OUTLETS];		// handle to the objects outlets
	int				x_verbose;
	pthread_attr_t 	gac_thread_attr;
	pthread_t    	x_threadid;
} t_gac;

void *gac_class;					// global pointer to the object class - so pd can reference the object 


// ==============================================================================
// Function Prototypes
// ------------------------------------------------------------------------------
void *gac_new(t_symbol *s);
void gac_assist(t_gac *x, void *b, long m, long a, char *s);
void gac_bang(t_gac *x);				
static int usbGetStringAscii(usb_dev_handle *dev, int ndex, int langid, char *buf, int buflen);
void find_device(t_gac *x);

// =============================================================================
// Threading
// ------------------------------------------------------------------------------
static void *usb_thread_read(void *w)
{
	t_gac *x = (t_gac*) w;
	int nBytes;
	
	while(1) {
		pthread_testcancel();
			if (!(x->dev_handle)) find_device(x);
			else {
					nBytes = usb_control_msg(x->dev_handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, 
										0, 0, 0, (char *)buffer, sizeof(buffer), DEFAULT_CLOCK_INTERVAL);
					if(x->x_verbose)post("thread read %i bytes", nBytes);
			}
	}
	return 0;
}

static void usb_thread_start(t_gac *x) {
	// create the worker thread
    if(pthread_attr_init(&x->gac_thread_attr) < 0)
	{
       error("gac: could not launch receive thread");
       return;
    }
    if(pthread_attr_setdetachstate(&x->gac_thread_attr, PTHREAD_CREATE_DETACHED) < 0)
	{
       error("gac: could not launch receive thread");
       return;
    }
    if(pthread_create(&x->x_threadid, &x->gac_thread_attr, usb_thread_read, x) < 0)
	{
       error("gac: could not launch receive thread");
       return;
    }
    else
    {
       if(x->x_verbose)post("gac: thread %d launched", (int)x->x_threadid );
    }
}

//--------------------------------------------------------------------------
// - Message: bang  -> poll gac
//--------------------------------------------------------------------------
void gac_bang(t_gac *x) {
	int                 i,n;
	int 				replymask,replyshift,replybyte;
	int					temp;

	for (n = 0; n < OUTLETS; n++) {
		temp = buffer[n];
		
		switch(n) {
			case 0:
				replybyte = buffer[16];
				replyshift = ((0 % 4) * 2);
				replymask = (3 << replyshift);
				temp = temp * 4 + ((replybyte & replymask) >> replyshift);
			break;
			case 1:
				replybyte = buffer[16];
				replyshift = ((1 % 4) * 2);
				replymask = (3 << replyshift);
				temp = temp * 4 + ((replybyte & replymask) >> replyshift);
			break;
			case 2:
				replybyte = buffer[16];
				replyshift = ((2 % 4) * 2);
				replymask = (3 << replyshift);
				temp = temp * 4 + ((replybyte & replymask) >> replyshift);
			break;
			case 3:
				replybyte = buffer[16];
				replyshift = ((3 % 4) * 2);
				replymask = (3 << replyshift);
				temp = temp * 4 + ((replybyte & replymask) >> replyshift);
			break;
			case 4:
				replybyte = buffer[17];
				replyshift = ((0 % 4) * 2);
				replymask = (3 << replyshift);
				temp = temp * 4 + ((replybyte & replymask) >> replyshift);
			break;
			case 5:
				replybyte = buffer[17];
				replyshift = ((1 % 4) * 2);
				replymask = (3 << replyshift);
				temp = temp * 4 + ((replybyte & replymask) >> replyshift);
			break;
			case 6:
				replybyte = buffer[17];
				replyshift = ((2 % 4) * 2);
				replymask = (3 << replyshift);
				temp = temp * 4 + ((replybyte & replymask) >> replyshift);
			break;
			case 7:
				replybyte = buffer[17];
				replyshift = ((3 % 4) * 2);
				replymask = (3 << replyshift);
				temp = temp * 4 + ((replybyte & replymask) >> replyshift);
			break;
			case 8:
				replybyte = buffer[18];
				replyshift = ((0 % 4) * 2);
				replymask = (3 << replyshift);
				temp = temp * 4 + ((replybyte & replymask) >> replyshift);
			break;
			case 9:
				replybyte = buffer[18];
				replyshift = ((1 % 4) * 2);
				replymask = (3 << replyshift);
				temp = temp * 4 + ((replybyte & replymask) >> replyshift);
			break;
			case 10:
				replybyte = buffer[18];
				replyshift = ((2 % 4) * 2);
				replymask = (3 << replyshift);
				temp = temp * 4 + ((replybyte & replymask) >> replyshift);
			break;
			case 11:
				replybyte = buffer[18];
				replyshift = ((3 % 4) * 2);
				replymask = (3 << replyshift);
				temp = temp * 4 + ((replybyte & replymask) >> replyshift);
			break;
			case 12:
				replybyte = buffer[19];
				replyshift = ((0 % 4) * 2);
				replymask = (3 << replyshift);
				temp = temp * 4 + ((replybyte & replymask) >> replyshift);
			break;
			case 13:
				replybyte = buffer[19];
				replyshift = ((1 % 4) * 2);
				replymask = (3 << replyshift);
				temp = temp * 4 + ((replybyte & replymask) >> replyshift);
			break;
			case 14:
				replybyte = buffer[19];
				replyshift = ((2 % 4) * 2);
				replymask = (3 << replyshift);
				temp = temp * 4 + ((replybyte & replymask) >> replyshift);
			break;
			case 15:
				replybyte = buffer[19];
				replyshift = ((3 % 4) * 2);
				replymask = (3 << replyshift);
				temp = temp * 4 + ((replybyte & replymask) >> replyshift);
			break;
			case 16:
				temp = buffer[20];
			break;
		}
		outlet_float(x->outlets[n], temp);
		
		/*
		replybyte = 16 + (i / 4);
		replyshift = ((i % 4) * 2);
		replymask = (3 << replyshift);
		temp = temp * 4 + ((replybyte & replymask) >> replyshift);	// add 2 LSB
		*/
	}
			
}

//--------------------------------------------------------------------------
// - The clock is ticking, tic, tac...
//--------------------------------------------------------------------------
void gac_tick(t_gac *x) { 
	clock_delay(x->m_clock, x->m_interval); 	// schedule another tick
	gac_bang(x); 								// poll the edubeat
} 


//--------------------------------------------------------------------------
// - Object creation and setup
//--------------------------------------------------------------------------
int gac_setup(void)
{
	gac_class = class_new ( gensym("gac"),(t_newmethod)gac_new, 0, sizeof(t_gac), 	CLASS_DEFAULT,0);
	// Add message handlers
	class_addbang(gac_class, (t_method)gac_bang);
	post("bald-approved pdbox version 0.1",0);
	return 1;
}

//--------------------------------------------------------------------------
void *gac_new(t_symbol *s)		// s = optional argument typed into object box (A_SYM) -- defaults to 0 if no args are typed
{
	t_gac *x;									// local variable (pointer to a t_gac data structure)
	x = (t_gac *)pd_new(gac_class);			 // create a new instance of this object
	x->m_clock = clock_new(x,(t_method)gac_tick);
    x->x_verbose = 0;

	x->m_interval = DEFAULT_CLOCK_INTERVAL;
	x->m_interval_bak = DEFAULT_CLOCK_INTERVAL;
	
	x->dev_handle = NULL;
	int i;
	
	// create outlets and assign it to our outlet variable in the instance's data structure
	for (i=0; i < OUTLETS; i++) {
		x->outlets[i] = outlet_new(&x->p_ob, &s_float);
	}	

    usb_thread_start(x); //start polling the device
	clock_delay(x->m_clock,0.); //start reading the buffer

	return x; // return a reference to the object instance 
}

//--------------------------------------------------------------------------
// - Object destruction
//--------------------------------------------------------------------------
void gac_free(t_gac *x)
{
	if (x->dev_handle) usb_close(x->dev_handle);
	freebytes((t_object *)x->m_clock, sizeof(x->m_clock));
		while(pthread_cancel(x->x_threadid) < 0)
			if(x->x_verbose)post("gac: killing thread\n");
		if(x->x_verbose)post("gac: thread canceled\n");
    		
}

//--------------------------------------------------------------------------
// - USB Utility Functions
//--------------------------------------------------------------------------
static int  usbGetStringAscii(usb_dev_handle *dev, int ndex, int langid, char *buf, int buflen)
{
char    asciibuffer[256];
int     rval, i;

    if((rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + ndex, langid, asciibuffer, sizeof(asciibuffer), 1000)) < 0)
        return rval;
    if(asciibuffer[1] != USB_DT_STRING)
        return 0;
    if((unsigned char)asciibuffer[0] < rval)
        rval = (unsigned char)asciibuffer[0];
    rval /= 2;
    /* lossy conversion to ISO Latin1 */
    for(i=1;i<rval;i++){
        if(i > buflen)  /* destination buffer overflow */
            break;
        buf[i-1] = asciibuffer[2 * i];
        if(asciibuffer[2 * i + 1] != 0)  /* outside of ISO Latin1 range */
            buf[i-1] = '?';
    }
    buf[i-1] = 0;
    return i-1;
}

//--------------------------------------------------------------------------
void find_device(t_gac *x) {
	usb_dev_handle      *handle = NULL;
	struct usb_bus      *bus;
	struct usb_device   *dev;
	
	usb_init();
	usb_find_busses();
    usb_find_devices();
	 for(bus=usb_busses; bus; bus=bus->next){
        for(dev=bus->devices; dev; dev=dev->next){
            if(dev->descriptor.idVendor == USBDEV_SHARED_VENDOR && dev->descriptor.idProduct == USBDEV_SHARED_PRODUCT){
                char    string[256];
                int     len;
                handle = usb_open(dev); /* we need to open the device in order to query strings */
                if(!handle){
                    error ("Warning: cannot open USB device: %s", usb_strerror());
                    continue;
                }
                /* now find out whether the device actually is gac */
                len = usbGetStringAscii(handle, dev->descriptor.iManufacturer, 0x0409, string, sizeof(string));
                if(len < 0){
                    post("gac: warning: cannot query manufacturer for device: %s", usb_strerror());
                    goto skipDevice;
                }
                
                if(strcmp(string, "orbitalrobots.com") != 0)
                    goto skipDevice;
                len = usbGetStringAscii(handle, dev->descriptor.iProduct, 0x0409, string, sizeof(string));
                if(len < 0){
                    post("gac: warning: cannot query product for device: %s", usb_strerror());
                    goto skipDevice;
                }
                if(strcmp(string, "EDUBEAT") == 0)
                    break;
				skipDevice:
                usb_close(handle);
                handle = NULL;
            }
        }
        if(handle)
            break;
    }
	
    if(!handle){
        post("Could not find USB device 11h11/gac");
		x->dev_handle = NULL;
	} else {
		 x->dev_handle = handle;
		 post("Found USB device 11h11/gac");
	}
}
