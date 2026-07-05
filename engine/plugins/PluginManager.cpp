#include "PluginManager.h"

PluginManager::~PluginManager() {
	shutdown_all();
}

bool PluginManager::register_plugin(std::unique_ptr<Plugin> plugin) {
	if (!plugin) return false;
	plugins_.push_back(std::move(plugin));
	return true;
}

bool PluginManager::initialize_all() {
	for (auto& plugin : plugins_) {
		if (!plugin->initialize()) {
			return false;
		}
	}
	return true;
}

void PluginManager::shutdown_all() {
	for (auto& plugin : plugins_) {
		plugin->shutdown();
	}
	plugins_.clear();
}

Plugin* PluginManager::find(const std::string& name) {
	for (auto& plugin : plugins_) {
		if (plugin->name() == name) return plugin.get();
	}
	return nullptr;
}

const std::vector<Plugin*>& PluginManager::plugins() const {
	static std::vector<Plugin*> raw;
	raw.clear();
	for (const auto& p : plugins_) raw.push_back(p.get());
	return raw;
}
