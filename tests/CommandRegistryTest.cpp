#include "CommandRegistry.h"

#include <gtest/gtest.h>

using scribe::core::Command;
using scribe::core::CommandRegistry;

namespace
{

Command makeCommand(const QString& id, std::function<void()> callback = [] {})
{
    return Command{.id = id,
                   .title = QStringLiteral("Title of ") + id,
                   .shortcut = QString(),
                   .callback = std::move(callback),
                   .enabled = true};
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

TEST(CommandRegistry, LiveConditionDecidesWhetherACommandRuns)
{
    CommandRegistry registry;
    int calls = 0;
    bool allowed = false;
    Command command = makeCommand(QStringLiteral("x"), [&] { ++calls; });
    command.enabledWhen = [&] { return allowed; };
    ASSERT_TRUE(registry.registerCommand(command));

    EXPECT_FALSE(registry.isEnabled(QStringLiteral("x")));
    EXPECT_FALSE(registry.run(QStringLiteral("x")));
    EXPECT_EQ(calls, 0);

    allowed = true; // the condition is read again each time, not remembered
    EXPECT_TRUE(registry.isEnabled(QStringLiteral("x")));
    EXPECT_TRUE(registry.run(QStringLiteral("x")));
    EXPECT_EQ(calls, 1);
}

TEST(CommandRegistry, SwitchedOffCommandIgnoresItsCondition)
{
    CommandRegistry registry;
    int conditionChecks = 0;
    Command command = makeCommand(QStringLiteral("x"));
    command.enabledWhen = [&] {
        ++conditionChecks;
        return true;
    };
    registry.registerCommand(command);

    ASSERT_TRUE(registry.setEnabled(QStringLiteral("x"), false));
    EXPECT_FALSE(registry.isEnabled(QStringLiteral("x")));
    EXPECT_EQ(conditionChecks, 0); // the cheap switch is checked first
}

TEST(CommandRegistry, CommandWithoutAConditionIsEnabledByItsSwitchAlone)
{
    CommandRegistry registry;
    registry.registerCommand(makeCommand(QStringLiteral("x")));
    EXPECT_TRUE(registry.isEnabled(QStringLiteral("x")));
    registry.setEnabled(QStringLiteral("x"), false);
    EXPECT_FALSE(registry.isEnabled(QStringLiteral("x")));
}

TEST(CommandRegistry, UnknownIdIsNotEnabled)
{
    const CommandRegistry registry;
    EXPECT_FALSE(registry.isEnabled(QStringLiteral("nope")));
}

TEST(CommandRegistry, ToggleCommandReportsItsLiveCheckState)
{
    CommandRegistry registry;
    bool on = false;
    Command command = makeCommand(QStringLiteral("x"), [&] { on = !on; });
    command.checkedWhen = [&] { return on; };
    ASSERT_TRUE(registry.registerCommand(command));

    EXPECT_FALSE(registry.isChecked(QStringLiteral("x")));
    registry.run(QStringLiteral("x"));
    EXPECT_TRUE(registry.isChecked(QStringLiteral("x"))); // read again each time
    registry.run(QStringLiteral("x"));
    EXPECT_FALSE(registry.isChecked(QStringLiteral("x")));
}

TEST(CommandRegistry, OrdinaryAndUnknownCommandsAreNeverChecked)
{
    CommandRegistry registry;
    registry.registerCommand(makeCommand(QStringLiteral("plain")));

    EXPECT_FALSE(registry.isChecked(QStringLiteral("plain")));
    EXPECT_FALSE(registry.isChecked(QStringLiteral("nope")));
}

TEST(CommandRegistry, KeepsAnAlternateShortcut)
{
    CommandRegistry registry;
    Command command = makeCommand(QStringLiteral("zoomIn"));
    command.shortcut = QStringLiteral("Ctrl++");
    command.alternateShortcut = QStringLiteral("Ctrl+=");
    ASSERT_TRUE(registry.registerCommand(command));

    EXPECT_EQ(registry.find(QStringLiteral("zoomIn"))->alternateShortcut, QStringLiteral("Ctrl+="));
}
