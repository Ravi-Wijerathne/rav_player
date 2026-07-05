#pragma once
#include "Plugin.h"
#include <memory>
#include <string>
#include <vector>

class PluginManager {
public:
	PluginManager() = default;
	~PluginManager();

	PluginManager(const PluginManager&) = delete;
	PluginManager& operator=(const PluginManager&) = delete;

	bool register_plugin(std::unique_ptr<Plugin> plugin);
	bool initialize_all();
	void shutdown_all();

	Plugin* find(const std::string& name);
	const std::vector<Plugin*>& plugins() const;

private:
	std::vector<std::unique_ptr<Plugin>> plugins_;
};
