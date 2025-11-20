#include "FileUtility.h"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

//.jsonファイル名の一覧を取得
std::vector<std::string> FileUtility::GetSceneFileNames(const std::string& directory_path = "JSON/")
{
    std::vector<std::string> file_names;

    try
    {
        //directoryPath 内のすべてのエントリを巡回
        for (auto& entry : fs::directory_iterator(directory_path))
        {
            //ファイルであること、かつ拡張子が.jsonであることを確認
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                //ファイル名のみをリストに追加
                file_names.push_back(entry.path().filename().string());
            }
        }
    }
    catch (const fs::filesystem_error& e)
    {
        std::cerr << "Filesystem Error: " << e.what() << std::endl;
    }

    return file_names;
}
