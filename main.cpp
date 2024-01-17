#include <iostream>
#include <string>
#include <fstream>
#include "lib/json.hpp"
#include "structs.h"
#include "generators.h"
#include "crawl.h"

using json = nlohmann::json;
using namespace std;
using namespace Config;

void process_for_pipe(Config::Website &w, vector<Config::Pipe> &v)
{
    if (!w.paginations.empty())
    {
        const auto urls = Generators::urls_from_pagination(w.url, w.paginations);
        for (const auto & url : urls)
        {
            Pipe p;
            p.url = url;
            p.name = w.url;
            copy(w.data.begin(), w.data.end(), back_inserter(p.data));
            v.push_back(p);
        }
    }
    else
    {
        Pipe p;
        p.url = w.url;
        p.name = w.url;
        copy(w.data.begin(), w.data.end(), back_inserter(p.data));
        v.push_back(p);
    }
}

int main([[maybe_unused]] int argc, char const *argv[])
{
    ifstream f;
    f.open((string)argv[1]);
    if (!f.is_open())
    {
        return 404;
    }
    json config;
    f >> config;
    f.close();
    vector<Config::Website> websites;
    json_to_websites(config, websites);
    vector<Config::Pipe> pipes;

    for (auto & website : websites)
    {
        process_for_pipe(website, pipes);
    }

    auto result = Crawler::crawl(pipes);
    std::ofstream output("result.json");
    output << result;
    output.close();
    return 0;
}
