#include <string>
#include <iostream>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;
#if !defined(MY_STRUCTS_HEADER)
#define MY_STRUCTS_HEADER
namespace Config
{

    typedef struct Pagination
    {
        string key;
        int min;
        int max;
        vector<string> values;
    };

    typedef struct Data;

    typedef struct SubURL
    {
        vector<Data> data;
        bool clientside = false;
        vector<Pagination> paginations;
        string name;
        string urlSelector;
    };

    typedef struct Data
    {
        string selector;
        bool multiple;
        string name;
        vector<Data> children;
        SubURL subUrl;
    };

    typedef struct Website
    {
        string url;
        vector<Data> data;
        bool clientside = false;
        vector<Pagination> paginations;
    };

    typedef struct Pipe
    {
        string name;
        string url;
        vector<Data> data;
        bool clientside = false;
    };

    void json_to_paginations(const json &j, vector<Pagination> &v)
    {
        const auto size = j.size();
        for (auto i = 0; i < size; i++)
        {
            const auto *pagination = &j.at(i);
            Pagination p;

            p.key = pagination->at("key");
            if (pagination->contains("min"))
                p.min = pagination->at("min");
            if (pagination->contains("max"))
                p.max = pagination->at("max");
            if (pagination->contains("values"))
            {
                auto values = pagination->at("values");
                const auto value_size = values.size();
                vector<string> new_values;
                for (auto j = 0; j < value_size; j++)
                {
                    p.values.push_back(values.at(j).dump());
                }
            }
            v.push_back(p);
        }
    }

    void json_to_data(const json &j, Data &d)
    {
        if (j.contains("selector"))
            j.at("selector").get_to(d.selector);
        if (j.contains("multiple"))
            j.at("multiple").get_to(d.multiple);
        if (j.contains("name"))
            j.at("name").get_to(d.name);
        if (j.contains("children"))
        {
            const auto *children = &j.at("children");
            for (int i = 0; i < children->size(); i++)
            {
                Data data;
                json_to_data(children->at(i), data);
                d.children.push_back(data);
            }
        }
        if (j.contains("subUrl"))
        {
            const auto *subUrl = &j.at("subUrl");
            if (subUrl->contains("name"))
                subUrl->at("name").get_to(d.subUrl.name);
            if (subUrl->contains("urlSelector"))
                subUrl->at("urlSelector").get_to(d.subUrl.urlSelector);
            if (subUrl->contains("clientside"))
                subUrl->at("clientside").get_to(d.subUrl.clientside);
            if (subUrl->contains("paginations"))
            {
                const auto paginations = subUrl->at("paginations");
                json_to_paginations(subUrl->at("paginations"), d.subUrl.paginations);
            }
            if (subUrl->contains("data"))
            {
                const auto *data_array = &subUrl->at("data");
                for (int i = 0; i < data_array->size(); i++)
                {
                    auto *jdata = &data_array->at(i);
                    Data data;
                    json_to_data(*jdata, data);
                    d.subUrl.data.push_back(data);
                }
            }
        } else {
            d.subUrl = {};
        }
    }

    void json_to_website(const json &j, Website &w)
    {
        j.at("url").get_to(w.url);
        if (j.contains("clientside"))
            j.at("clientside").get_to(w.clientside);
        if (j.contains("paginations"))
        {
            const auto paginations = j.at("paginations");
            json_to_paginations(j.at("paginations"), w.paginations);
        }
        if (j.contains("data"))
        {
            const auto *data_array = &j.at("data");
            for (int i = 0; i < data_array->size(); i++)
            {
                auto *jdata = &data_array->at(i);
                Data data;
                json_to_data(*jdata, data);
                w.data.push_back(data);
            }
        }
    }

    void json_to_websites(const json &j, vector<Website> &v)
    {
        for (short i = 0; i < j.size(); i++)
        {
            Config::Website w;
            Config::json_to_website(j.at(i), w);
            v.push_back(w);
        }
    }
} // namespace config
#endif