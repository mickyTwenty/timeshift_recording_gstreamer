/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   myMosq.h
 * Author: micky
 *
 * Created on February 20, 2020, 8:44 AM
 */

#ifndef MYMOSQ_H
#define MYMOSQ_H

#include <mosquittopp.h>
#include <mosquitto.h>

#include "common.h"

class myMosq : public mosqpp::mosquittopp
{
private:
    gchar           id[128];
    int             port;
    int             keepalive;

    void            on_connect(int rc);
    void            on_disconnect(int rc);
    
public:
    myMosq();
    ~myMosq();
    
    void            on_message(const mosquitto_message* _message);
};

#endif /* MYMOSQ_H */

