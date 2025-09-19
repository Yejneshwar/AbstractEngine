#include "FileUtils.h"
#import <Foundation/Foundation.h>

std::string GUI::Utils::getResourcePath(const std::string& inputPath) {
    std::string path = inputPath;
    
    // 1. Remove leading "./", "/", or "Resource/"
    if (path.rfind("./", 0) == 0) {
        path = path.substr(2);
    }
    if (path.rfind("/", 0) == 0) {
        path = path.substr(1);
    }
    
    // 2. Separate directory and filename
    std::string directory;
    std::string filename = path;
    
    size_t slashPos = path.find_last_of('/');
    if (slashPos != std::string::npos) {
        directory = path.substr(0, slashPos);
        filename = path.substr(slashPos + 1);
    }
    
    // 3. Split filename into name and extension
    std::string name = filename;
    std::string extension;
    
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos) {
        name = filename.substr(0, dotPos);
        extension = filename.substr(dotPos + 1);
    }
    
    // 4. Use NSBundle to find the full path
    NSString *nsName = [NSString stringWithUTF8String:name.c_str()];
    NSString *nsExt = extension.empty() ? nil : [NSString stringWithUTF8String:extension.c_str()];
    NSString *nsDir = directory.empty() ? nil : [NSString stringWithUTF8String:directory.c_str()];
    
    NSString *fullPath = [[NSBundle mainBundle] pathForResource:nsName
                                                         ofType:nsExt
                                                    inDirectory:nsDir];
    
    if (!fullPath) {
        NSLog(@"[getResourcePath] Resource not found: %@", [NSString stringWithUTF8String:inputPath.c_str()]);
        return "";
    }
    
    return std::string([fullPath UTF8String]);
}
