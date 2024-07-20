#include <iostream>
#include <fstream>
#include <string>
#include <utility>
#include <regex>
#include <functional>
#include <algorithm>
#include <cctype>
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

enum PatternType {
    eInclude = 0,
    eExclude
};

struct Pattern {
    PatternType Type;
    std::regex Regex;
    std::string TextualTranslatedPattern; // for debug purposes
    std::string TextualOriginalPattern; // for debug purposes
};

// -- glob to regex translator --
// from : https://gist.github.com/alco/1869512
/*
 * Return a new string with all occurrences of 'from' replaced with 'to'
 */
std::string replace_all(const std::string &str, const char *from, const char *to)
{
    std::string result(str);
    std::string::size_type
            index = 0,
            from_len = strlen(from),
            to_len = strlen(to);
    while ((index = result.find(from, index)) != std::string::npos) {
        result.replace(index, from_len, to);
        index += to_len;
    }
    return result;
}

/*
 * Translate a shell pattern into a regular expression
 * This is a direct translation of the algorithm defined in fnmatch.py.
 */
static std::string translate_to_regex_string(const std::string &pattern)
{
    int i = 0;
    int n = (int)pattern.length();
    std::string result;

    while (i < n) {
        char c = pattern[i];
        ++i;

        if (c == '*') {
            result += ".*";
        } else if (c == '?') {
            result += '.';
        } else if (c == '[') {
            int j = i;
            /*
             * The following two statements check if the sequence we stumbled
             * upon is '[]' or '[!]' because those are not valid character
             * classes.
             */
            if (j < n && pattern[j] == '!')
                ++j;
            if (j < n && pattern[j] == ']')
                ++j;
            /*
             * Look for the closing ']' right off the bat. If one is not found,
             * escape the opening '[' and continue.  If it is found, process
             * the contents of '[...]'.
             */
            while (j < n && pattern[j] != ']')
                ++j;
            if (j >= n) {
                result += "\\[";
            } else {
                std::string stuff = replace_all(std::string(&pattern[i], j - i), "\\", "\\\\");
                char first_char = pattern[i];
                i = j + 1;
                result += "[";
                if (first_char == '!') {
                    result += "^" + stuff.substr(1);
                } else if (first_char == '^') {
                    result += "\\" + stuff;
                } else {
                    result += stuff;
                }
                result += "]";
            }
        } else {
            if (isalnum(c)) {
                result += c;
            } else {
                result += "\\";
                result += c;
            }
        }
    }
    return result;
}
// -- end of glob to regex translator --

std::string to_lower(std::string sentence)
{
    std::string out = sentence;
    std::transform(sentence.begin(), sentence.end(), out.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return out;
}

std::vector<Pattern> description_to_patterns(const std::vector<std::string>& description)
{
    std::vector<Pattern> patterns;
    for(std::string line : description)
    {
        if(line.empty())
            continue;

        Pattern p;
        std::string l = line;
        p.Type = eExclude;
        if(line[0] == '!')
        {
            p.Type = eInclude;
            l = line.substr(1);
        }
        l = to_lower(l); // for case insensitivity

        std::string regex_txt = translate_to_regex_string(l);
        p.TextualOriginalPattern = l;
        p.TextualTranslatedPattern = regex_txt;
        p.Regex = regex_txt;
        patterns.emplace_back(p);
    }
    return  patterns;
}

std::vector<std::string> matched_files(std::vector<std::string> files, std::vector<Pattern> patterns)
{
    std::vector<std::string> matches;
    // if a file entry matches no pattern of the exclude type it should be included in the list
    // if a file entry matches a pattern of the exclude type it should not be included unless a pattern of include type will match it after
    // if a pattern of include type exists but a later pattern causes it to be excluded, it should be excluded.

    for (const auto& file : files) {
        bool include = true;
        std::string lower_case_file = to_lower(file); // for case insensitivity

        for (const auto& pattern : patterns) {
            if (std::regex_match(lower_case_file, pattern.Regex)) {
                if (pattern.Type == eExclude) {
                    include = false;
                }
                if (pattern.Type == eInclude) {
                    include = true;
                }
            }
        }

        if (include) {
            matches.push_back(file);
        }
    }

    return matches;
}

int main(int argc, char *argv[]) {
    std::string path = ".";
    if (argc > 1) { path = argv[1]; }

    std::cout << "Reads 'filematch.txt' and outputs all files from directory that matches description or all files if no 'filematch.txt' is found." << std::endl;

    std::vector<std::string> files = get_all_files(path);
    std::vector<std::string> patterns_description = read_file(path + path_sep + "filematch.txt");
    std::vector<Pattern> patterns = description_to_patterns(patterns_description);
    std::vector<std::string> matches = matched_files(files, patterns);

    for(const std::string& file : matches)
    {
        printf("%s\n", file.c_str());
    }

    return 0;
}
