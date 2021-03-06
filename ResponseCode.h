/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef _RESPONSECODE_H
#define _RESPONSECODE_H

class ResponseCode {
    // Keep in sync with
    // frameworks/base/services/java/com/android/server/NetworkManagementService.java
public:
    // 100 series - Requestion action was initiated; expect another reply
    // before proceeding with a new command.
    static const int ActionInitiated           = 100;

    // 200 series - Requested action has been successfully completed
    static const int CommandOkay               = 200;
    static const int StartOkay                 = 201;
    static const int StopOkay                  = 202;
    static const int GetRunningStatusRsp       = 203;

    // 400 series - The command was accepted but the requested action
    // did not take place.
    static const int OperationFailed           = 400;
    static const int StartFailed               = 401;
    static const int StopFailed                = 402;

    // 500 series - The command was not accepted and the requested
    // action did not take place.
    static const int CommandSyntaxError        = 500;
    static const int CommandParameterError     = 501;

    // 600 series - self defined errors
    static const int FileAccessFailed          = 600;
    static const int FileOpenFailed            = 601;
    static const int ThreadStartFailed         = 602;
    static const int ThreadStopFailed          = 602;
    static const int LogAlreadyStartFailed     = 603;

    static const int ImpossibleFailed          = 700;
    static int resMobileLogStatus;
    static int resModemLogStatus;
    static int resNetLogStatus;
};
#endif
