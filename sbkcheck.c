// Reversed from rayman's sbkdetectv2 at https://forum.xda-developers.com/showthread.php?t=1290503
// I got a SIGBUS on Ubuntu 18.04 and this should compile for any host with libusb-1.0

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <libusb-1.0/libusb.h>

#include "getvercheckcmd.h"

#define NVIDIA_VID 0x0955
#define APX_PID 0x7820
#define INTERFACE_NUM 0
#define BCDDEVICE_SBKV2 0x104
#define VERSION_SBKV2 0x20001

typedef struct chipuid {
	uint32_t uid;
	uint32_t unk;
} chipuid_t;

static int perr(char const *format, ...)
{
	va_list args;
	int r;

	va_start (args, format);
	r = vfprintf(stderr, format, args);
	va_end(args);

	return r;
}

#define ERR_EXIT(errcode) ({ perr("   %s\n", libusb_strerror(errcode)); exit(-1); })
#define CALL_CHECK(fcall) ({ enum libusb_error r=(enum libusb_error)fcall; if (r < 0) ERR_EXIT(r); })

static libusb_device_handle *find_device(libusb_context *ctx) {
	libusb_device **list;
	libusb_device *apx_dev = NULL;
	libusb_device_handle *apx_handle = NULL;
	CALL_CHECK(libusb_get_device_list(ctx, &list));
	for (libusb_device **dev = list; *dev; ++dev) {
		struct libusb_device_descriptor desc;
		CALL_CHECK(libusb_get_device_descriptor(*dev, &desc));
		if (desc.idVendor == NVIDIA_VID && desc.idProduct == APX_PID) {
			puts("Found APX mode device");
			apx_dev = *dev;
			break;
		}
	}
	if (apx_dev) {
		CALL_CHECK(libusb_open(apx_dev, &apx_handle));
	}
	libusb_free_device_list(list, 1);
	return apx_handle;
}

// taken from https://github.com/AndroidRoot/wheelie/blob/master/usb/linux/libnvusb.c
static int32_t getEndpoints(struct libusb_config_descriptor *config, uint8_t *ep_in, uint8_t *ep_out) {
	int32_t i;
	int32_t ret=-2;
	for(i=0; i<config->interface[0].altsetting[0].bNumEndpoints; i++){
		if((config->interface[0].altsetting[0].endpoint[i].bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN){
			*ep_in = config->interface[0].altsetting[0].endpoint[i].bEndpointAddress;
			ret++;
		}else{
			*ep_out = config->interface[0].altsetting[0].endpoint[i].bEndpointAddress;
			ret++;
		}
	}
	return ret;
}

int main(void) {
	int ret = 0;
	libusb_context *ctx;
	CALL_CHECK(libusb_init(&ctx));
	libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
	libusb_device_handle *apx_handle = find_device(ctx);
	if (apx_handle) {
		libusb_device *apx_dev = libusb_get_device(apx_handle);
		if (!apx_dev) {
			puts("Couldn't get APX device from handle.");
			ret = -2;
			goto usb_exit;
		}
		struct libusb_config_descriptor *apx_config_desc;
		CALL_CHECK(libusb_get_active_config_descriptor(apx_dev, &apx_config_desc));
		struct libusb_device_descriptor apx_dev_desc;
		CALL_CHECK(libusb_get_device_descriptor(apx_dev, &apx_dev_desc));
		if (apx_dev_desc.bcdDevice == BCDDEVICE_SBKV2) {
			CALL_CHECK(libusb_claim_interface(apx_handle, INTERFACE_NUM));
			uint8_t ep_in, ep_out;
			if (getEndpoints(apx_config_desc, &ep_in, &ep_out)) {
				puts("Error retreiving endpoints");
				return -1;
			}
			chipuid_t chipuid = { .uid = 0xDEADBEEF, .unk = 0xBAADF00D };
			int num_transfered = 0;
			CALL_CHECK(libusb_bulk_transfer(apx_handle, ep_in, (uint8_t *)&chipuid, sizeof(chipuid), &num_transfered, 0));
			if (num_transfered != sizeof(chipuid)) {
				puts("Couldn't get chipuid.");
				ret = -3;
				goto usb_exit;
			}
			printf("chipuid uid: 0x%08x unk: 0x%08x\n", chipuid.uid, chipuid.unk);
			num_transfered = 0;
			CALL_CHECK(libusb_bulk_transfer(apx_handle, ep_out, getvercheckcmd_bin, sizeof(getvercheckcmd_bin), &num_transfered, 0));
			if (num_transfered != sizeof(getvercheckcmd_bin)) {
				puts("Couldn't send getvercheck command.");
				ret = -4;
				goto usb_exit;
			}
			uint32_t version = 0xFACEFEED;
			num_transfered = 0;
			CALL_CHECK(libusb_bulk_transfer(apx_handle, ep_in, (uint8_t *)&version, sizeof(version), &num_transfered, 0));
			if (num_transfered != sizeof(version)) {
				puts("Couldn't get getvercheck command result.");
				ret = -5;
				goto usb_exit;
			}
			libusb_release_interface(apx_handle, INTERFACE_NUM);
			printf("version: 0x%08x\n", version);
			if (version == VERSION_SBKV2) {
				puts("Detected SBKv2 via version.");
			} else {
				puts("Detected SBKv1 via version.");
			}
		} else {
			puts("Detected SBKv1 via bcdDevice.");
		}
	} else {
		puts("couldn't get handle to APX device");
	}
usb_exit:
	libusb_exit(ctx);
	return ret;
}
