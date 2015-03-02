/*
* libefivar - library for the manipulation of EFI variables
* Copyright 2012-2015 Red Hat, Inc.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _EFIVAR_INTERNAL_DP_H
#define _EFIVAR_INTERNAL_DP_H

#include <alloca.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "ucs2.h"

#define format(buf, size, off, fmt, args...) ({				\
		ssize_t _x;						\
		_x = snprintf(((buf)+(off)),				\
			       ((size)?((size)-(off)):0),		\
			       fmt, ## args);				\
		if (_x < 0)						\
			return _x;					\
		_x;							\
	})

#define format_helper(fn, buf, size, off, args...) ({			\
		ssize_t _x;						\
		_x = (fn)(((buf)+(off)),				\
			  ((size)?((size)-(off)):0),			\
			  ## args);					\
		if (_x < 0)						\
			return _x;					\
		_x;							\
	})

#define onstack(buf, len) ({						\
		char *__newbuf = alloca(len);				\
		memcpy(__newbuf, buf, len);				\
		free(buf);						\
		__newbuf;						\
	})

#define format_guid(buf, size, off, guid) ({				\
		int _rc;						\
		char *_guidstr = NULL;					\
									\
		_rc = efi_guid_to_str(guid, &_guidstr);			\
		if (_rc < 0)						\
			return _rc;					\
		_guidstr = onstack(_guidstr, strlen(_guidstr)+1);	\
		format(buf, size, off, "%s", _guidstr);			\
	})

static inline ssize_t
__attribute__((__unused__))
format_hex_helper(char *buf, size_t size, const void const *addr,
		  const size_t len)
{
	ssize_t off = 0;
	ssize_t sz;
	for (size_t i = 0; i < len; i++) {
		sz = format(buf, size, off, "%02x",
			     *((const unsigned char const *)addr+i));
		if (sz < 0)
			return -1;
		off += sz;
	}
	return off;
}

#define format_hex(buf, size, off, addr, len)				\
	format_helper(format_hex_helper, buf, size, off, addr, len)

static inline ssize_t
__attribute__((__unused__))
format_vendor_helper(char *buf, size_t size, char *label, const_efidp dp)
{
	ssize_t off = 0;
	size_t bytes = efidp_node_size(dp)
			- sizeof (efidp_header)
			- sizeof (efi_guid_t);

	off = format(buf, size, off, "%s(", label);
	off += format_guid(buf, size, off, &dp->hw_vendor.vendor_guid);
	if (bytes) {
		off += format(buf, size, off, ",");
		off += format_hex(buf, size, off,
				  dp->hw_vendor.vendor_data, bytes);
	}
	off += format(buf, size, off, ")");
	return off;
}

#define format_vendor(buf, size, off, label, dp)			\
	format_helper(format_vendor_helper, buf, size, off, label, dp)

#define format_ucs2(buf, size, off, str, len) ({			\
		uint16_t _ucs2buf[(len)];				\
		memset(_ucs2buf, '\0', sizeof (_ucs2buf));		\
		memcpy(_ucs2buf, str, sizeof (_ucs2buf)			\
				      - sizeof (_ucs2buf[0]));		\
		char *_asciibuf = ucs2_to_utf8(_ucs2buf, (len) - 1);	\
		if (_asciibuf == NULL)					\
			return -1;					\
		_asciibuf = onstack(_asciibuf, (len));			\
		format(buf, size, off, "%s", _asciibuf);		\
       })

static inline int
__attribute__((__unused__))
peek_dn_type(const_efidp dp, uint8_t type, uint8_t subtype)
{
	if (efidp_type(dp) == EFIDP_END_TYPE &&
			efidp_subtype(dp) == EFIDP_END_ENTIRE)
		return 0;

	dp = (const_efidp)(const efidp_header const *)((uint8_t *)dp + dp->length);

	if (efidp_type(dp) == type && efidp_subtype(dp) == subtype)
		return 1;
	return 0;
}

extern ssize_t format_hw_dn(char *buf, size_t size, const_efidp dp);
extern ssize_t format_acpi_dn(char *buf, size_t size, const_efidp dp);
extern ssize_t format_message_dn(char *buf, size_t size, const_efidp dp);
extern ssize_t format_media_dn(char *buf, size_t size, const_efidp dp);
extern ssize_t format_bios_boot_dn(char *buf, size_t size, const_efidp dp);

#endif /* _EFIVAR_INTERNAL_DP_H */
