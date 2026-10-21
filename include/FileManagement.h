#pragma once
#include <filesystem>
#include <vector>
#include <tuple>
#include <string>

using namespace std;
using namespace std::filesystem;

std::vector<std::tuple<std::string, std::string>> GetModelPaths()
{
	std::vector<std::tuple<std::string, std::string>> modelPaths;
	std::string target_ext = ".obj";
	path workingDirectory(std::string(SDL_GetBasePath()) + "assets/obj");

	for (const auto& entry : recursive_directory_iterator(workingDirectory))
	{
		if (entry.is_regular_file() && entry.path().extension() == target_ext)
		{
			std::string filename = entry.path().string();
			std::string parentPath = entry.path().parent_path().string();
			size_t pos = filename.find(parentPath);
			if (pos != std::string::npos)
				filename.erase(pos, parentPath.length());
			modelPaths.emplace_back(std::tuple<std::string, std::string>(filename, entry.path().string()));
		}
	}

	return modelPaths;
}