/*
 * Copyright (c) 2005  Gregor Richards
 *
 * This file is part of Glass.
 * 
 * Glass is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Glass is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Glass; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef KLASSI_H
#define KLASSI_H

#include <map>
using namespace std;

class Klass;
class Variable;

class KlassI {
    public:
    Klass *of;
    map<string,Variable *> vars;
};

#endif