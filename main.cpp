#include <iostream>
#include <string>
#include <regex>
#include <functional>
#include <stack>

#if defined(_WIN32)
// desktop win32 software
#include "Windows.h"


#if defined(UNICODE)
std::string wide_to_string(LPCWSTR t)
{
    char utf8buffer[1024];

    WideCharToMultiByte(
        CP_UTF8,
        0,
        t,
        -1,
        utf8buffer,
        1024,
        nullptr,
        FALSE
    );
    std::string f = utf8buffer;
    return f;
}

void string_to_wide(std::string t, WCHAR *output)
{
    MultiByteToWideChar(
        CP_UTF8,
        0,
        t.c_str(),
        -1,
        output,
        MAX_PATH
        );
}

#endif

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
#if defined(UNICODE)
    WCHAR output_wide[MAX_PATH];
    string_to_wide(path.c_str(), output_wide);
    fileHandle = FindFirstFile(output_wide, &ffd);
#else
    fileHandle = FindFirstFile(path.c_str(), &ffd);
#endif    

    if (INVALID_HANDLE_VALUE == fileHandle) {
        printf("Invalid File Handle Value \n");
        return result;
    }

    do
    {
#if defined(UNICODE)
        std::string f = wide_to_string(ffd.cFileName);
#else
        std::string f = ffd.cFileName;
#endif        
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
            stack.emplace_back(current_dir + "\\" + dir, relative_path + dir + "\\");
        }
    }

    return allfiles;
}

#endif




int main(int argc, char *argv[]) {
    std::string path = ".";
    if (argc > 1) { path = argv[1]; }

    std::vector<std::string> files = get_all_files(path);

    for(std::string file : files)
    {
        printf("%s\n", file.c_str());
    }

    std::cout << "Reads 'filematch.txt' and outputs all files from directory that matches description or nothing if no 'filematch.txt' is found." << std::endl;
    return 0;
}
