#include "CommandProcessor.h"

// A parancsok regisztrálása
void CommandProcessor::registerCommand(const std::string& commandName, Command command) {
    commands_[commandName] = command;
}

// A parancs végrehajtása
void CommandProcessor::execute(const std::string& commandName  ,OutConsumer consumer ) const {
    auto it = commands_.find(commandName);
    if (it != commands_.end()) {
        // A parancs megtalálva, végrehajtjuk
        it->second(consumer);
    } 
}