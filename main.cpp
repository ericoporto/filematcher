#include <iostream>
#include <string>
#include <utility>
#include <regex>
#include <functional>
#include <vector>

#if defined(_WIN32)
const std::string path_sep = "\\";
#else
const std::string path_sep = "/";
#endif

#if defined(_WIN32)
// desktop win32 software
#include "Windows.h"


std::pair<std::vector<std::string>,std::vector<std::string>> get_files_and_dirs(const std::string &dirname) {
    std::pair<std::vector<std::string>,std::vector<std::string>> result;
    std::vector<std::string> files;
    std::vector<std::string> dirs;
    std::string path = dirname;

    if(dirname.back() == '/' || dirname.back() == '\\') {
        path = dirname + "*";
    } else if(dirname.back() != '*' && dirname.back() != '/') {
        path = dirname + "\\*";
    }

    HANDLE fileHandle;
    WIN32_FIND_DATA ffd;
    LARGE_INTEGER szDir;
    WIN32_FIND_DATA fileData;
    fileHandle = FindFirstFile(path.c_str(), &ffd);

    if (INVALID_HANDLE_VALUE == fileHandle) {
        printf("Invalid File Handle Value \n");
        return result;
    }

    do
    {
        std::string f = ffd.cFileName;
        bool is_valid_name = f != "." && f != "..";
        bool is_dir = ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        if(!is_valid_name)
            continue;
        if(is_dir)
        {
            dirs.emplace_back(f);
        }
        else
        {
            files.emplace_back(f);
        }
    } while (FindNextFile(fileHandle, &ffd) != 0);

    result.first = files;
    result.second = dirs;

    return result;
}
#else
#include <sys/types.h>
#include <dirent.h>
#include <fstream>

std::pair<std::vector<std::string>, std::vector<std::string>> get_files_and_dirs(const std::string &dirname) {
    std::pair<std::vector<std::string>, std::vector<std::string>> result;
    std::vector<std::string> files;
    std::vector<std::string> dirs;

    std::string path = dirname;
    if (!path.empty() && path.back() != '/') {
        path += '/';
    }

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(path.c_str())) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            std::string name = ent->d_name;
            if (name != "." && name != "..") {
                if (ent->d_type == DT_DIR) {
                    dirs.push_back(name);
                } else {
                    files.push_back(name);
                }
            }
        }
        closedir(dir);
    } else {
        std::cerr << "Error opening directory " << path << std::endl;
    }

    result.first = files;
    result.second = dirs;

    return result;
}
#endif

std::vector<std::string> get_all_files(const std::string &dirname)
{
    std::vector<std::string> allfiles;
    std::vector<std::pair<std::string, std::string>> stack;
    stack.emplace_back(dirname, "");

    while (!stack.empty()) {
        auto current = stack.back();
        stack.pop_back();
        std::string current_dir = current.first;
        std::string relative_path = current.second;

        auto result = get_files_and_dirs(current_dir);
        auto const& files = result.first;
        auto const& dirs = result.second;

        for (const auto &file : files) {
            allfiles.push_back(relative_path + file);
        }

        for (const auto &dir : dirs) {
            stack.emplace_back(current_dir + path_sep + dir, relative_path + dir + path_sep);
        }
    }

    return allfiles;
}

std::vector<std::string> read_file(const std::string& filename) {
    std::vector<std::string> lines;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Unable to open file: " << filename << std::endl;
        return lines;
    }

    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    file.close();
    return lines;
}

int main(int argc, char *argv[]) {
    std::string path = ".";
    if (argc > 1) { path = argv[1]; }

    std::cout << "Reads 'filematch.txt' and outputs all files from directory that matches description or all files if no 'filematch.txt' is found." << std::endl;

    std::vector<std::string> files = get_all_files(path);
    std::vector<std::string> patterns_description = read_file(path + path_sep + "filematch.txt");

    for(std::string file : files)
    {
        printf("%s\n", file.c_str());
    }

    return 0;
}
