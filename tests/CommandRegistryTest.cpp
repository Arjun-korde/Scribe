#include "CommandRegistry.h"

#include <gtest/gtest.h>

using scribe::core::Command;
using scribe::core::CommandRegistry;

namespace
{

Command makeCommand(const QString& id, std::function<void()> callback = [] {})
{
    return Command{id, QStringLiteral("Title of ") + id, QString(), std::move(callback), true};
}

} // namespace

TEST(CommandRegistry, StartsEmpty)
{
    const CommandRegistry registry;
    EXPECT_TRUE(registry.commands().empty());
    EXPECT_EQ(registry.find(QStringLiteral("file.save")), nullptr);
}

TEST(CommandRegistry, RegistersAndFindsCommands)
{
    CommandRegistry registry;
    Command save = makeCommand(QStringLiteral("file.save"));
    save.shortcut = QStringLiteral("Ctrl+S");
    ASSERT_TRUE(registry.registerCommand(save));

    const Command* found = registry.find(QStringLiteral("file.save"));
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id, QStringLiteral("file.save"));
    EXPECT_EQ(found->title, QStringLiteral("Title of file.save"));
    EXPECT_EQ(found->shortcut, QStringLiteral("Ctrl+S"));
    EXPECT_TRUE(found->enabled);
}

TEST(CommandRegistry, KeepsRegistrationOrder)
{
    CommandRegistry registry;
    registry.registerCommand(makeCommand(QStringLiteral("b")));
    registry.registerCommand(makeCommand(QStringLiteral("a")));
    registry.registerCommand(makeCommand(QStringLiteral("c")));

    ASSERT_EQ(registry.commands().size(), 3u);
    EXPECT_EQ(registry.commands()[0].id, QStringLiteral("b"));
    EXPECT_EQ(registry.commands()[1].id, QStringLiteral("a"));
    EXPECT_EQ(registry.commands()[2].id, QStringLiteral("c"));
}

TEST(CommandRegistry, RejectsDuplicateEmptyIdAndEmptyCallback)
{
    CommandRegistry registry;
    int calls = 0;
    ASSERT_TRUE(registry.registerCommand(makeCommand(QStringLiteral("x"), [&] { ++calls; })));

    EXPECT_FALSE(registry.registerCommand(makeCommand(QStringLiteral("x"), [&] { calls += 100; })));
    EXPECT_FALSE(registry.registerCommand(makeCommand(QString())));
    EXPECT_FALSE(registry.registerCommand(Command{QStringLiteral("y"), {}, {}, nullptr, true}));
    EXPECT_EQ(registry.commands().size(), 1u);

    registry.run(QStringLiteral("x"));
    EXPECT_EQ(calls, 1); // the original callback survived the rejected duplicate
}

TEST(CommandRegistry, RunInvokesTheCallback)
{
    CommandRegistry registry;
    int calls = 0;
    registry.registerCommand(makeCommand(QStringLiteral("x"), [&] { ++calls; }));

    EXPECT_TRUE(registry.run(QStringLiteral("x")));
    EXPECT_TRUE(registry.run(QStringLiteral("x")));
    EXPECT_EQ(calls, 2);
}

TEST(CommandRegistry, RunOfUnknownIdDoesNothing)
{
    const CommandRegistry registry;
    EXPECT_FALSE(registry.run(QStringLiteral("nope")));
}

TEST(CommandRegistry, DisabledCommandsDoNotRun)
{
    CommandRegistry registry;
    int calls = 0;
    registry.registerCommand(makeCommand(QStringLiteral("x"), [&] { ++calls; }));

    ASSERT_TRUE(registry.setEnabled(QStringLiteral("x"), false));
    EXPECT_FALSE(registry.find(QStringLiteral("x"))->enabled);
    EXPECT_FALSE(registry.run(QStringLiteral("x")));
    EXPECT_EQ(calls, 0);

    ASSERT_TRUE(registry.setEnabled(QStringLiteral("x"), true));
    EXPECT_TRUE(registry.run(QStringLiteral("x")));
    EXPECT_EQ(calls, 1);
}

TEST(CommandRegistry, SetEnabledOfUnknownIdFails)
{
    CommandRegistry registry;
    EXPECT_FALSE(registry.setEnabled(QStringLiteral("nope"), false));
}

TEST(CommandRegistry, LookupsStayCorrectAfterManyRegistrations)
{
    // The internal vector reallocates as it grows; ids must still map to the right command.
    CommandRegistry registry;
    for (int i = 0; i < 200; ++i) {
        ASSERT_TRUE(registry.registerCommand(makeCommand(QString::number(i))));
    }
    for (int i = 0; i < 200; ++i) {
        const auto* command = registry.find(QString::number(i));
        ASSERT_NE(command, nullptr);
        EXPECT_EQ(command->id, QString::number(i));
    }
}
