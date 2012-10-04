#include <stdio.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

#define USB_MINIMAG_VENDOR_ID      0x0acd
#define USB_MINIMAG_PRODUCT_ID     0x0200
#define UDELAY                     500000

// Configuration utility for ID Tech MiniMag IDT-3331 card swipers
// Based on code from http://web.mit.edu/ecommerce/src/idt-minimag.c
//
// Compile: gcc -lusb-1.0 minimag.c -o minimag
//

// sends a command to the minimag swiper
int minimag_send(libusb_device_handle* devh, char* str, int len) {
	int wrote = 0;
	int r;
	int i;

	for(i = 0; i < len; i++) {
		char c = str[i];

		if(c == '\r' || c == '\n')
			c = '\r';

		fprintf(stderr, "%c", c);

		r = libusb_control_transfer(
			devh,
			LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
			9,
			0x300,  /* value == report type FEATURE, id 0 */
			0,      /* index */
			&c,
			1,
			5000); /* timeout */

		if(r < 0) {
			fprintf(stderr, "write error: %s\n", libusb_error_name(r));
			break;
		}

		wrote += r;

		usleep(UDELAY);

		if(c == '\r') break;
	}

	fprintf(stderr, "\nwrote %d bytes\n", wrote);

	return r;
}

int main() {
	libusb_context* ctx = NULL;
	libusb_device_handle* devh = NULL;
	char buf[4096];
	int r;

	// start libusb
	r = libusb_init(&ctx);

	if(r < 0) {
		perror("unable to initialize libusb");
		return 1;
	}

	// more verbosity
	libusb_set_debug(ctx, 3);

	// find the minimag swiper
	devh = libusb_open_device_with_vid_pid(ctx, USB_MINIMAG_VENDOR_ID, USB_MINIMAG_PRODUCT_ID);

	if(devh == NULL) {
		fprintf(stderr, "unable to open device\n");
		return 1;
	}

	// detach the driver from the kernel
	r = libusb_kernel_driver_active(devh, 0);

	if(r == 1)
		r = libusb_detach_kernel_driver(devh, 0);

	if(r < 0) {
		fprintf(stderr, "usb_kernel_driver_detach error %s\n", libusb_error_name(r));
		return 1;
	}

	// reset configuration
	r = libusb_set_configuration(devh, 1);
	if(r < 0) {
		fprintf(stderr, "usb_set_configuration error %s\n", libusb_error_name(r));
		return 1;
	}

	// claim
	r = libusb_claim_interface(devh, 0);

	if (r < 0) {
		fprintf(stderr, "usb_claim_interface error %s\n", libusb_error_name(r));
		return 1;
	}

	// prompt the user for commands
	fprintf(stdout, "ready\n");

	while(1) {
		if(fgets(buf, 4096, stdin) != NULL) {
			minimag_send(devh, buf, 4096);
		}
	}

	libusb_release_interface(devh, 0);
	libusb_attach_kernel_driver(devh, 0);
	libusb_close(devh);
	libusb_exit(ctx);
}

