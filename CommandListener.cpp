/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
 *
 * Not a Contribution.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// #define LOG_NDEBUG 0

#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <linux/if.h>
#include <inttypes.h>
#include <cutils/log.h>
#include <netutils/ifc.h>
#include <sysutils/SocketClient.h>

#include "CommandListener.h"
#include "ResponseCode.h"

MobileLogController *CommandListener::sMobileLogCtrl = NULL;
ModemLogController *CommandListener::sModemLogCtrl = NULL;
NetLogController *CommandListener::sNetLogCtrl = NULL;

int ResponseCode::resMobileLogStatus = ResponseCode::ActionInitiated;
int ResponseCode::resModemLogStatus = ResponseCode::ActionInitiated;
int ResponseCode::resNetLogStatus = ResponseCode::ActionInitiated;

CommandListener::CommandListener() :
                 FrameworkListener("jrdlogd", true) {
    registerCmd(new MobileLogCommand("mobilelog"));
    registerCmd(new ModemLogCommand("modemlog"));
    registerCmd(new NetLogCommand("netlog"));
    if (!sMobileLogCtrl)
        sMobileLogCtrl = new MobileLogController();

    if (!sModemLogCtrl)
        sModemLogCtrl = new ModemLogController();   

    if (!sNetLogCtrl)
        sNetLogCtrl = new NetLogController();   
}

CommandListener::MobileLogCommand::MobileLogCommand(const char *cmd) :
                 FrameworkCommand(cmd) {
}

int CommandListener::MobileLogCommand::runCommand(SocketClient *cli,
                                                      int argc, char **argv) {
    bool rc = 0;

    ALOGD("MobileLogCommand runCommand argc %d, argv1 %s", argc, argv[1]);
    ResponseCode::resMobileLogStatus = ResponseCode::ActionInitiated;
 
    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
        return 0;
    }

    if (!strcmp(argv[1], "stop")) {    
        rc = sMobileLogCtrl->stopMobileLogging();
        if (rc) {
            ALOGD("Mobilelog stop succeeded");  
            cli->sendMsg(ResponseCode::StopOkay, "Mobilelog stop succeeded", false);
        } else {
            char *msg = NULL;
            ALOGD("Mobilelog stop failed");  
            asprintf(&msg, "Mobilelog stop failed, reason: %d", ResponseCode::resMobileLogStatus);
            cli->sendMsg(ResponseCode::StopFailed, msg, true);
            free(msg);
        }
    } else if (!strcmp(argv[1], "start")) {
        rc = sMobileLogCtrl->startMobileLogging();
        if (rc) {
            ALOGD("Mobilelog start succeeded");  
            cli->sendMsg(ResponseCode::StartOkay, "Mobilelog start succeeded", false);
        } else {
            char *msg = NULL;
            ALOGD("Mobilelog start failed");            
            asprintf(&msg, "Mobilelog start failed, reason: %d", ResponseCode::resMobileLogStatus);
            cli->sendMsg(ResponseCode::StartFailed, msg, true);
            free(msg);
        }        
    } else {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown mobilelog cmd", false);
        return 0;
    }


    return 0;
} 

CommandListener::ModemLogCommand::ModemLogCommand(const char *cmd) :
                 FrameworkCommand(cmd) {
}

int CommandListener::ModemLogCommand::runCommand(SocketClient *cli,
                                                      int argc, char **argv) {
    bool rc = 0;

    ALOGD("ModemLogCommand runCommand argc %d, argv1 %s", argc, argv[1]);
    ResponseCode::resModemLogStatus = ResponseCode::ActionInitiated;
 
    if (!strcmp(argv[1], "stop")) {    
        rc = sModemLogCtrl->stopModemLogging();
        if (rc) {
            ALOGD("Modemlog stop succeeded");
            cli->sendMsg(ResponseCode::StopOkay, "Modemlog stop succeeded", false);
        } else {
            char *msg = NULL;
            ALOGD("Modemlog stop failed");            
            asprintf(&msg, "Modemlog stop failed, reason: %d", ResponseCode::resModemLogStatus);
            cli->sendMsg(ResponseCode::StopFailed, msg, true);
            free(msg);        
        }
    } else if (!strcmp(argv[1], "start")) {
        rc = sModemLogCtrl->startModemLogging();
        if (rc) {
            ALOGD("Modemlog start succeeded");
            cli->sendMsg(ResponseCode::StartOkay, "Modemlog start succeeded", false);
        } else {
            char *msg = NULL;
            ALOGD("Modemlog start failed");
            asprintf(&msg, "Modemlog start failed, reason: %d", ResponseCode::resModemLogStatus);
            cli->sendMsg(ResponseCode::StartFailed, msg, true);
            free(msg);           
        }        
    } else {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown Modemlog cmd", false);
        return 0;
    }

    return 0;
} 

CommandListener::NetLogCommand::NetLogCommand(const char *cmd) :
                 FrameworkCommand(cmd) {
}

int CommandListener::NetLogCommand::runCommand(SocketClient *cli,
                                                      int argc, char **argv) {
    bool rc = 0;

    ALOGD("NetLogCommand runCommand argc %d, argv1 %s", argc, argv[1]);
    ResponseCode::resNetLogStatus = ResponseCode::ActionInitiated;
 
    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
        return 0;
    }

    if (!strcmp(argv[1], "stop")) {    
        rc = sNetLogCtrl->stopNetLogging();
        if (rc) {
            ALOGD("Netlog stop succeeded");
            cli->sendMsg(ResponseCode::StopOkay, "Netlog stop succeeded", false);
        } else {
            char *msg = NULL;
            ALOGD("Netlog stop failed");
            asprintf(&msg, "Netlog stop failed, reason: %d", ResponseCode::resNetLogStatus);
            cli->sendMsg(ResponseCode::StopFailed, msg, true);
            free(msg);   
        }
    } else if (!strcmp(argv[1], "start")) {
        rc = sNetLogCtrl->startNetLogging();
        if (rc) {
            ALOGD("Netlog start succeeded");
            cli->sendMsg(ResponseCode::StartOkay, "Netlog start succeeded", false);
        } else {
            char *msg = NULL;
            ALOGD("Netlog start failed");
            asprintf(&msg, "Netlog start failed, reason: %d", ResponseCode::resNetLogStatus);
            cli->sendMsg(ResponseCode::StartFailed, msg, true);
            free(msg); 
        }        
    } else {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown Netlog cmd", false);
        return 0;
    }


    return 0;
} 


