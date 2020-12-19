//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------
#pragma once

extern bool log_updated;
extern char log_entry[256];

void write_log(const char *text);
void write_log(int number);