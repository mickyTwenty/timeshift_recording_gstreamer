/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MyConfig.h
 * Author: micky
 *
 * Created on February 21, 2020, 12:15 PM
 */

#ifndef MYCONFIG_H
#define MYCONFIG_H

#include <glib.h>


class MyConfig {
public:
    MyConfig();    
    virtual ~MyConfig();
    
public:
    int         load_config(gchar *file);
    
private:

};

#endif /* MYCONFIG_H */

