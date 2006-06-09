/*
 *    This file is part of LiSA Command Line Interface.
 *
 *    LiSA Command Line Interface is free software; you can redistribute it 
 *    and/or modify it under the terms of the GNU General Public License 
 *    as published by the Free Software Foundation; either version 2 of the 
 *    License, or (at your option) any later version.
 *
 *    LiSA Command Line Interface is distributed in the hope that it will be 
 *    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with LiSA Command Line Interface; if not, write to the Free 
 *    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *    MA  02111-1307  USA
 */

#ifndef __FILTER_H
#define __FILTER_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>

#define MAX_LINE_WIDTH 1024
#define GREP_PATH "/bin/grep"
/* 
Output modifier operating modes: 
 
MODE_BEGIN: search for the first occurence of pattern, and then
    simply echo remaining lines
MODE_INCLUDE: list only lines matching the given pattern
MODE_EXCLUDE: list only lines _not_ matching the given pattern
MODE_GREP: this is the feature to have grep "full option" as our
	output modifier, because we're a linux switch, not just a stupid 
	ignorant cisco switch ;-)
*/

#define MODE_INCLUDE 	0x0000
#define MODE_EXCLUDE 	0x0001
#define MODE_BEGIN		0x0002
#define MODE_GREP		0x0003
#define MODE_MASK		0x000f

#endif
