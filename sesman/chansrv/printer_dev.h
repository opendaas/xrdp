/**
 * Copyright (C) 2010 Ulteo SAS
 * http://www.ulteo.com
 * Author David Lechevalier <david@ulteo.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/

#ifndef PRINTER_DEV_H_
#define PRINTER_DEV_H_

#include "log.h"
#include "arch.h"
#include "os_calls.h"
#include "parse.h"

#define PPD_FILE 		"/usr/share/ppd/PostscriptColor.ppd"
#define DEVICE_URI	"xrdp_printer:"
#define SPOOL_DIR		"/var/spool/xrdp_printer/"

int DEFAULT_CC
printer_dev_add(struct stream* s, int device_data_length, int device_id, char* dos_name);

int DEFAULT_CC
printer_dev_get_next_job();


#define RDPDR_PRINTER_ANNOUNCE_FLAG_ASCII				0x00000001
#define RDPDR_PRINTER_ANNOUNCE_FLAG_DEFAULTPRINTER		0x00000002
#define RDPDR_PRINTER_ANNOUNCE_FLAG_NETWORKPRINTER		0x00000004
#define RDPDR_PRINTER_ANNOUNCE_FLAG_TSPRINTER			0x00000008
#define RDPDR_PRINTER_ANNOUNCE_FLAG_XPSFORMAT			0x00000010

struct printer_device
{
	int device_id;
	char printer_name[256];
};

#endif /* PRINTER_DEV_H_ */
