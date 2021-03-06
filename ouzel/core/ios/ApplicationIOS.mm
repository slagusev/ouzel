// Copyright (C) 2017 Elviss Strazdins
// This file is part of the Ouzel engine.

#import <UIKit/UIKit.h>
#include "ApplicationIOS.h"
#import "AppDelegate.h"

namespace ouzel
{
    ApplicationIOS::ApplicationIOS(int aArgc, char* aArgv[]):
        Application(aArgc, aArgv)
    {
        mainQueue = dispatch_get_main_queue();
    }

    int ApplicationIOS::run()
    {
        @autoreleasepool
        {
            return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
        }
    }

    void ApplicationIOS::execute(const std::function<void(void)>& func)
    {
        std::function<void(void)> localFunction = func;

        dispatch_async(mainQueue, ^{
            localFunction();
        });
    }

    bool ApplicationIOS::openURL(const std::string& url)
    {
        NSString* nsStringURL = [NSString stringWithUTF8String:url.c_str()];
        
        return [[UIApplication sharedApplication] openURL:[NSURL URLWithString:nsStringURL]] == YES;
    }
}
