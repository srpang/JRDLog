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
    int rc = 0;

    ALOGD("MobileLogCommand runCommand argc %d, argv1 %s", argc, argv[1]);
    ResponseCode::resMobileLogStatus = ResponseCode::ActionInitiated;
 
    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
        return 0;
    }

    if (!strcmp(argv[1], "stop")) {    
        rc = sMobileLogCtrl->stopMobileLogging();
        if (!rc) {
            cli->sendMsg(ResponseCode::CommandOkay, "Mobilelog stop succeeded", false);
        } else {
            cli->sendMsg(ResponseCode::resMobileLogStatus, "Mobilelog stop failed", true);
        }
    } else if (!strcmp(argv[1], "start")) {
        rc = sMobileLogCtrl->startMobileLogging();
        if (!rc) {
            cli->sendMsg(ResponseCode::CommandOkay, "Mobilelog start succeeded", false);
        } else {
            cli->sendMsg(ResponseCode::resMobileLogStatus, "Mobilelog start failed", true);
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
    int rc = 0;

    ALOGD("ModemLogCommand runCommand argc %d, argv1 %s", argc, argv[1]);
    ResponseCode::resModemLogStatus = ResponseCode::ActionInitiated;
 
    if (!strcmp(argv[1], "stop")) {    
        rc = sModemLogCtrl->stopModemLogging();
        if (!rc) {
            cli->sendMsg(ResponseCode::CommandOkay, "Modemlog stop succeeded", false);
        } else {
            cli->sendMsg(ResponseCode::resModemLogStatus, "Modemlog stop failed", true);
        }
    } else if (!strcmp(argv[1], "start")) {
        rc = sModemLogCtrl->startModemLogging();
        if (!rc) {
            cli->sendMsg(ResponseCode::CommandOkay, "Modemlog start succeeded", false);
        } else {
            cli->sendMsg(ResponseCode::resModemLogStatus, "Modemlog start failed", true);
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
    int rc = 0;

    ALOGD("NetLogCommand runCommand argc %d, argv1 %s", argc, argv[1]);
    ResponseCode::resNetLogStatus = ResponseCode::ActionInitiated;
 
    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
        return 0;
    }

    if (!strcmp(argv[1], "stop")) {    
        rc = sNetLogCtrl->stopNetLogging();
        if (!rc) {
            cli->sendMsg(ResponseCode::CommandOkay, "Netlog stop succeeded", false);
        } else {
            cli->sendMsg(ResponseCode::resNetLogStatus, "Netlog stop failed", true);
        }
    } else if (!strcmp(argv[1], "start")) {
        rc = sNetLogCtrl->startNetLogging();
        if (!rc) {
            cli->sendMsg(ResponseCode::CommandOkay, "Netlog start succeeded", false);
        } else {
            cli->sendMsg(ResponseCode::resModemLogStatus, "Netlog start failed", true);
        }        
    } else {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown Netlog cmd", false);
        return 0;
    }


    return 0;
} 


