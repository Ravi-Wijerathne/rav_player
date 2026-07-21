#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "plugins/PluginManager.h"
#include "plugins/Plugin.h"

using namespace testing;

class MockPlugin : public Plugin {
public:
    MOCK_METHOD(std::string_view, name, (), (const, override));
    MOCK_METHOD(std::string_view, version, (), (const, override));
    MOCK_METHOD(std::string_view, description, (), (const, override));
    MOCK_METHOD(bool, initialize, (), (override));
    MOCK_METHOD(void, shutdown, (), (override));
};

TEST(PluginManagerTest, RegisterPlugin) {
    PluginManager manager;
    auto plugin = std::make_unique<MockPlugin>();
    EXPECT_CALL(*plugin, name()).WillRepeatedly(Return("TestPlugin"));
    
    EXPECT_TRUE(manager.register_plugin(std::move(plugin)));
    EXPECT_EQ(manager.plugins().size(), 1);
    EXPECT_NE(manager.find("TestPlugin"), nullptr);
}

TEST(PluginManagerTest, InitializeAllSuccess) {
    PluginManager manager;
    auto plugin1 = std::make_unique<MockPlugin>();
    auto plugin2 = std::make_unique<MockPlugin>();
    
    EXPECT_CALL(*plugin1, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*plugin2, initialize()).WillOnce(Return(true));
    
    manager.register_plugin(std::move(plugin1));
    manager.register_plugin(std::move(plugin2));
    
    EXPECT_TRUE(manager.initialize_all());
}

TEST(PluginManagerTest, InitializeAllFailure) {
    PluginManager manager;
    auto plugin1 = std::make_unique<MockPlugin>();
    auto plugin2 = std::make_unique<MockPlugin>();
    
    EXPECT_CALL(*plugin1, initialize()).WillOnce(Return(true));
    EXPECT_CALL(*plugin2, initialize()).WillOnce(Return(false));
    
    manager.register_plugin(std::move(plugin1));
    manager.register_plugin(std::move(plugin2));
    
    EXPECT_FALSE(manager.initialize_all());
}

TEST(PluginManagerTest, ShutdownAll) {
    PluginManager manager;
    auto plugin = std::make_unique<MockPlugin>();
    
    EXPECT_CALL(*plugin, shutdown()).Times(1);
    
    manager.register_plugin(std::move(plugin));
    manager.shutdown_all();
    EXPECT_EQ(manager.plugins().size(), 0);
}
