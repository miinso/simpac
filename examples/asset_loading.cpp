#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#define GetCurrentDir getcwd
#endif

std::string get_current_dir() {
    char buff[FILENAME_MAX];
    GetCurrentDir(buff, FILENAME_MAX);
    std::string current_working_dir(buff);
    return current_working_dir;
}

bool is_directory(const std::string& path) {
#ifdef _WIN32
    DWORD ftyp = GetFileAttributesA(path.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false;
    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true;
#else
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) != 0)
        return false;
    return S_ISDIR(statbuf.st_mode);
#endif
    return false;
}

std::vector<std::string> list_directory(const std::string& path) {
    std::vector<std::string> result;
#ifdef _WIN32
    WIN32_FIND_DATA find_data;
    HANDLE hFind = FindFirstFile((path + "\\*").c_str(), &find_data);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            result.push_back(find_data.cFileName);
        } while (FindNextFile(hFind, &find_data) != 0);
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(path.c_str());
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            result.push_back(entry->d_name);
        }
        closedir(dir);
    }
#endif
    return result;
}

void print_directory_tree(const std::string& path, const std::string& prefix = "") {
    std::vector<std::string> items = list_directory(path);
    std::sort(items.begin(), items.end());

    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i] == "." || items[i] == "..") continue;

        bool is_last_item = (i == items.size() - 1);
        std::string new_prefix = prefix + (is_last_item ? "`-- " : "|-- ");
        std::cout << new_prefix << items[i] << std::endl;

        if (is_directory(path + "/" + items[i])) {
            print_directory_tree(path + "/" + items[i], 
                                 prefix + (is_last_item ? "    " : "|   "));
        }
    }
}

int main() {
    std::string current_dir = get_current_dir();
    std::cout << "Current working directory: " << current_dir << std::endl;

    std::cout << "Directory tree:" << std::endl;
    print_directory_tree(current_dir);

    std::string file_path = "resources/hello.txt";
    std::cout << "\nAttempting to open file: " << file_path << std::endl;

    std::ifstream file(file_path);
    if (file.is_open()) {
        std::cout << "File contents:" << std::endl;
        std::string line;
        while (std::getline(file, line)) {
            std::cout << line << std::endl;
        }
        file.close();
    } else {
        std::cerr << "Unable to open file: " << file_path << std::endl;
    }
    return 0;
}