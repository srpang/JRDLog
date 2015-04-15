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

#ifndef _COMMANDLISTENER_H__
#define _COMMANDLISTENER_H__

#include <sysutils/FrameworkListener.h>
#include "MobileLogController.h"

class CommandListener : public FrameworkListener {

public:
    static MobileLogController *sMobileLogCtrl;

    CommandListener();
    virtual ~CommandListener() {}

private:
                  
    class MobileLogCommand : public FrameworkCommand {
    public:
        MobileLogCommand(const char *cmd);
        virtual ~MobileLogCommand() {}
        int runCommand(SocketClient *c, int argc, char ** argv);
    };

};

#endif
