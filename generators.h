#include "structs.h"
#include "combination.h"
#include <map>

#if !defined(MY_GENERATORS_HEADER)
#define MY_GENERATORS_HEADER
namespace Generators
{

    vector<string> urls_from_pagination(const string& url, const vector<Config::Pagination> &v)
    {
        std::map<string, vector<string>> values;

        for (const auto & i : v)
        {
            const auto *p = &i;
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

        Combination::CombinationArray possibilities = Combination::combine_vectors(values);

        vector<string> result;
        for (auto & p : possibilities)
        {
            auto *possibility = &p;
            string newUrl = url;
            for (int j = 0; j < possibility->size(); j++)
            {
                auto key = "{" + v.at(j).key + "}";
                auto value = possibility->at(j);
                size_t start_pos = newUrl.find(key);
                newUrl.replace(start_pos, key.length(), value);
            }
            result.push_back(newUrl);
        }
        return result;
    }
} // namespace Generators
#endif