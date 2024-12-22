#pragma once

#include <unordered_map>
#include <functional>

// A parancsok típusát határozza meg
class CommandProcessor {
public:
    
    // Parancsok kimenete (függvény pointer)
    using OutConsumer = std::function<void(const std::string& text)>;

    // Parancsok típusú alias (függvény pointer)
    using Command = std::function<void(OutConsumer consumer) >;

    
    // Parancs regisztrálása
    void registerCommand(const std::string& commandName, Command command);

    // Parancs végrehajtása
    void execute(const std::string& commandName, OutConsumer consumer)  const;

private:
    // Parancsokat tároló szótár
    std::unordered_map<std::string, Command> commands_;
};

