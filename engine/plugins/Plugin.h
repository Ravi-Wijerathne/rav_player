#pragma once
#include <string>
#include <string_view>

class Plugin {
public:
	virtual ~Plugin() = default;

	virtual std::string_view name() const = 0;
	virtual std::string_view version() const = 0;
	virtual std::string_view description() const = 0;

	virtual bool initialize() = 0;
	virtual void shutdown() = 0;
};
