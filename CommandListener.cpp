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

CommandListener::CommandListener() :
                 FrameworkListener("jrdlogd", true) {
    registerCmd(new MobileLogCommand("mobilelog"));
    if (!sMobileLogCtrl)
        sMobileLogCtrl = new MobileLogController();
}

CommandListener::MobileLogCommand::MobileLogCommand(const char *cmd) :
                 FrameworkCommand(cmd) {
}

int CommandListener::MobileLogCommand::runCommand(SocketClient *cli,
                                                      int argc, char **argv) {
    int rc = 0;

    ALOGD("MobileLogCommand runCommand argc %d, argv1 %s", argc, argv[1]);

    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
        return 0;
    }

    if (!strcmp(argv[1], "stop")) {    
        rc = sMobileLogCtrl->stopMobileLogging();
    } else if (!strcmp(argv[1], "start")) {
        rc = sMobileLogCtrl->startMobileLogging();
    } else {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown mobilelog cmd", false);
        return 0;
    }

    if (!rc) {
        cli->sendMsg(ResponseCode::CommandOkay, "Mobilelog operation succeeded", false);
    } else {
        cli->sendMsg(ResponseCode::OperationFailed, "Mobilelog operation failed", true);
    }

    return 0;
} 
