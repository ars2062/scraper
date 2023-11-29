#include "structs.h"
#include "combination.h"
#include <map>
#include <chrono>

#if !defined(MY_GENERATORS_HEADER)
#define MY_GENERATORS_HEADER
namespace Generators
{

    vector<string> urls_from_paginations(string url, const vector<Config::Pagination> &v)
    {
        std::map<string, vector<string>> values;

        for (int i = 0; i < v.size(); i++)
        {
            const auto *p = &v[i];
            if (p->values.size() > 0)
            {
                for (const auto &it : p->values)
                {
                    values["{" + p->key + "}"].push_back(it);
                }
            }
            else
            {
                for (int j = p->min; j <= p->max; j++)
                {
                    values["{" + p->key + "}"].push_back(to_string(j));
                }
            }
        }

        ThreadPool pool = ThreadPool(thread::hardware_concurrency());
        Combination::CombinationArray posibilities = Combination::combine_vectors(url, values, pool);

        vector<string> result;
        for (int i = 0; i < posibilities.size(); i++)
        {
            auto *posibility = &posibilities[i];
            string newUrl = url;
            for (int j = 0; j < posibility->size(); j++)
            {
                auto key = "{" + v.at(j).key + "}";
                auto value = posibility->at(j);
                size_t start_pos = newUrl.find(key);
                newUrl.replace(start_pos, key.length(), value);
            }
            result.push_back(newUrl);
        }
        return result;
    }
} // namespace Generators
#endif