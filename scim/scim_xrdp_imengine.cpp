/**
 * Copyright (C) 2012 Ulteo SAS
 * http://www.ulteo.com
 * Author Alexandre CONFIANT-LATOUR <a.confiant@ulteo.com> 2012
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

#define Uses_SCIM_UTILITY
#define Uses_SCIM_IMENGINE
#define Uses_SCIM_CONFIG_BASE

#include <scim.h>
#include "scim_xrdp_imengine_factory.h"
#include "scim_xrdp_imengine.h"

XrdpInstance::XrdpInstance(XrdpFactory *factory, const String &encoding, int id)
:IMEngineInstanceBase(factory, encoding, id), m_factory(factory) { }

XrdpInstance::~XrdpInstance() { }

bool XrdpInstance::process_key_event(const KeyEvent& key) {
	if (key.is_key_release()) return true;
	ucs4_t code = key.code & ((ucs4_t) 0x001FFFFF);

	if (code == key.code) {
		return false; /* Keysym */
	}

	if (key.mask == 0 || key.is_shift_down()) {
		WideString str;
		str.push_back(code);
		commit_string(str);
		return true;
	}

	return false;
}

void XrdpInstance::move_preedit_caret(unsigned int pos) { }
void XrdpInstance::select_candidate(unsigned int item) { }
void XrdpInstance::update_lookup_table_page_size(unsigned int page_size) { }
void XrdpInstance::lookup_table_page_up() { }
void XrdpInstance::lookup_table_page_down() { }
void XrdpInstance::reset() { }
void XrdpInstance::focus_in() { }
void XrdpInstance::focus_out() { }
void XrdpInstance::trigger_property(const String &property) { }
