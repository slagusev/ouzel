// Copyright (C) 2017 Elviss Strazdins
// This file is part of the Ouzel engine.

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "utils/Noncopyable.h"

namespace ouzel
{
    class Application;

    class FileSystem: public Noncopyable
    {
        friend Application;
    public:
        static const std::string DIRECTORY_SEPARATOR;

        static std::string getHomeDirectory();
        static std::string getStorageDirectory(const std::string& developer, const std::string& app);
        static std::string getTempDirectory();

        bool readFile(const std::string& filename, std::vector<uint8_t>& data) const;
        bool writeFile(const std::string& filename, const std::vector<uint8_t>& data) const;

        std::string getPath(const std::string& filename) const;
        void addResourcePath(const std::string& path);

        static std::string getExtensionPart(const std::string& path);
        static std::string getFilenamePart(const std::string& path);
        static std::string getDirectoryPart(const std::string& path);
        static bool isAbsolutePath(const std::string& path);

        static bool directoryExists(const std::string& filename);
        static bool fileExists(const std::string& filename);

    protected:
        FileSystem();

        std::string appPath;
        std::vector<std::string> resourcePaths;
    };
}
