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

#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "builtins.h"
#include "func.h"
#include "glass.h"
#include "klass.h"
#include "klassi.h"
#include "parseq.h"
#include "variable.h"

vector<Klass *> klasses;
map<string,Variable *> globalVars;

deque<Variable *> mainStack;

bool parseI(char *inp, int &i, ParseQ *pq);
void stringParse(string &toparse);

int main(int argc, char **argv)
{
    int i = 0;
    char *input;
    FILE *fin;
    int fini;
    struct stat sbuf;
    ParseQ *master;
    
    if (argc < 2) {
        fprintf(stderr, "Use: glass <input>\n");
        return 1;
    }
    
    // first read in builtins
    master = new ParseQ();
    i = 0;
    while (parseI(builtinDefinitions, i, master));
    master->parseKlasses();
    delete master;
    
    // open the file ...
    fin = fopen(argv[1], "r");
    if (!fin) { perror(argv[1]); return 1; }
    
    // get the size ...
    fini = fileno(fin);
    if (fstat(fini, &sbuf) == -1) { perror("fstat"); return 1; }
    
    // allocate
    input = (char *) malloc(sbuf.st_size + 2);
    input[sbuf.st_size] = '\0';
    
    // read
    if (fread(input, 1, sbuf.st_size, fin) <= 0) { perror("fread"); return 1; }
    
    // parse
    master = new ParseQ();
    i = 0;
    while (parseI(input, i, master));
    master->parseKlasses();
    delete master;
    
    // now run M.m
    Variable *Mvar;
    Klass *M;
    KlassI *MI;
    if (!globalVars["M"]) {
        cout << "NO M!" << endl;
        return 2;
    }
    Mvar = globalVars["M"];
    
    if (Mvar->type != VAR_KLASS) {
        cout << "M IS NOT A CLASS!" << endl;
        return 2;
    }
    
    M = Mvar->kval;
    MI = new KlassI();
    globalVars["MI"] = new Variable(VAR_KLASSI, MI);
    MI->of = M;
    
    // now make sure M.m exists
    if (M->functions.find("m") == M->functions.end()) {
        cout << "NO M.m!" << endl;
        return 2;
    }
    
    // and run it
    M->functions["m"]->contents->runFunc(MI, M->functions["m"]);
    
    return 0;
}

bool parseI(char *inp, int &i, ParseQ *pq)
{
    int j;
    
    // this function stops iterating at inp[i] == '\0'
    if (!inp[i]) return false;
    
    if (inp[i] >= 'A' && inp[i] <= 'Z') {
        pq->add(new ParseQElement(PQT_GLOBAL, inp[i]));
    } else if (inp[i] >= 'a' && inp[i] <= 'z') {
        pq->add(new ParseQElement(PQT_CLASSWIDE, inp[i]));
    } else if (inp[i] >= '0' && inp[i] <= '9') {
        pq->add(new ParseQElement(PQT_STACK, inp[i]));
    } else if (inp[i] == '(') {
        i++;
        for (j = i; inp[j] && inp[j] != ')'; j++);
        if (!inp[j]) {
            // ERROR
            return false;
        }
        
        // now we're at the )
        inp[j] = '\0';
        // now some sub-parsing
        if (inp[i] >= 'A' && inp[i] <= 'Z') {
            pq->add(new ParseQElement(PQT_GLOBAL, inp + i));
        } else if (inp[i] >= 'a' && inp[i] <= 'z') {
            pq->add(new ParseQElement(PQT_CLASSWIDE, inp + i));
        } else if (inp[i] >= '0' && inp[i] <= '9') {
            pq->add(new ParseQElement(PQT_STACK, inp + i));
        } else if (inp[i] == '_') {
            pq->add(new ParseQElement(PQT_LOCAL, inp + i));
        }
        i = j;
    } else if (inp[i] == '"') {
        // push a string
        i++;
        for (j = i; inp[j] && inp[j] != '"'; j++);
        if (!inp[j]) {
            // ERROR
            return false;
        }
        
        // now we're on the ending "
        inp[j] = '\0';
        string tomk = inp + i;
        
        stringParse(tomk);
        
        pq->add(new ParseQElement(PQT_STRING, tomk));
        i = j;
    } else if (inp[i] == '<') {
        // push a number
        i++;
        for (j = i; inp[j] && inp[j] != '>'; j++);
        if (!inp[j]) {
            // ERROR
            return false;
        }
        
        // now we're on the >
        inp[j] = '\0';
        pq->add(new ParseQElement(PQT_NUMBER, inp + i));
        i = j;
    } else if (inp[i] == '~') {
        // special, builtin
        i++;
        for (j = i; inp[j] && inp[j] != '~'; j++);
        if (!inp[j]) {
            // ERROR
            return false;
        }
        
        // now we're on the ending "
        inp[j] = '\0';
        pq->add(new ParseQElement(PQT_BUILTIN, inp + i));
        i = j;
    } else if (inp[i] == '\'') {
        // comment
        for (; inp[i] && inp[i] != '\''; i++);
        if (!inp[i]) {
            // ERROR
            return false;
        }
    } else if (inp[i] != ' ' && inp[i] != '\t' &&
               inp[i] != '\n' && inp[i] != '\r') {
        // the rest are all commands (or ignorable non-commands)
        pq->add(new ParseQElement(PQT_COMMAND, inp[i]));
    }
    i++;
    return true;
}

void stringParse(string &toparse)
{
    unsigned int i;
    
    for (i = 0; i < toparse.length(); i++) {
        if (toparse[i] == '\\') {
            switch (toparse[i+1]) {
                case 'n':
                    toparse.replace(i, 2, string("\n"));
                    break;
                    
                default:
                    toparse.replace(i, 2, toparse.substr(i + 1, 1));
            }
        }
    }
}

Variable *getVar(KlassI *klass, map<string,Variable *> *locals, string name)
{
    // what type of var?
    char fchar = name[0];
    if (fchar >= 'A' && fchar <= 'Z') {
        // global
        if (globalVars.find(name) == globalVars.end()) {
            globalVars[name] = new Variable();
        }
        return globalVars[name];
    } else if (fchar >= 'a' && fchar <= 'z') {
        // classwide
        if (klass->vars.find(name) == klass->vars.end()) {
            klass->vars[name] = new Variable();
            // check if it's a function that hasn't been allocated in the klassi due to lazy allocation
            if (klass->of->functions.find(name) != klass->of->functions.end()) {
                klass->vars[name]->type = VAR_FUNC;
                klass->vars[name]->fval = klass->of->functions[name];
            }
        }
        return klass->vars[name];
    } else if (fchar == '_') {
        // local
        if (locals->find(name) == locals->end()) {
            (*locals)[name] = new Variable();
        }
        return (*locals)[name];
    } else {
        // ERROR
        exit(2);
    }
}