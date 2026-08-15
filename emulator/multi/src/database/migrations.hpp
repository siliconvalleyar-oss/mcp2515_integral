#pragma once

#include <string>
#include <vector>

namespace ecumult {

class Migrations {
public:
    struct Migration {
        int version;
        std::string description;
        std::string sql;
    };

    static std::vector<Migration> getAll();
    static int currentVersion();

private:
    static std::vector<Migration> initMigrations();
};

} // namespace ecumult
