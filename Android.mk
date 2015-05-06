# Copyright 2006-2014 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
             jrdlogd.cpp \
             CommandListener.cpp \
             MobileLogController.cpp \
             ModemLogController.cpp \
             NetLogController.cpp \
             event.logtags \

LOCAL_SHARED_LIBRARIES := \
                  liblog \
                  libsysutils \

LOCAL_MODULE := jrd_log_d

LOCAL_CPPFLAGS := -std=c++11 -Wall -Werror

include $(BUILD_EXECUTABLE)

include $(call first-makefiles-under,$(LOCAL_PATH))
