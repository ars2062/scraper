#include <vector>
#include "structs.h"
#include <thread>
#include <set>
#include <map>
#include <regex>

#include "pool.h"
#include "json.hpp"
// #include "libxml2/libxml/parser.h"
#include <curl/curl.h>

#include <libxml/xpath.h>
#include <libxml/HTMLparser.h>

namespace Crawler
{
    using namespace nlohmann;
    using namespace Config;

    set<string> processed;
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
    {
        ((std::string *)userp)->append((char *)contents, size * nmemb);
        return size * nmemb;
    }

    std::string get_request(std::string word)
    {
        CURLcode res_code = CURLE_FAILED_INIT;
        CURL *curl = curl_easy_init();
        std::string result;
        std::string url = "https://www.merriam-webster.com/dictionary/" + word;

        curl_global_init(CURL_GLOBAL_ALL);

        if (curl)
        {
            curl_easy_setopt(curl,
                             CURLOPT_WRITEFUNCTION,
                             static_cast<curl_write>([](char *contents, size_t size,
                                                        size_t nmemb, std::string *data) -> size_t
                                                     {
        size_t new_size = size * nmemb;
        if (data == NULL) {
          return 0;
        }
        data -> append(contents, new_size);
        return new_size; }));
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "simple scraper");

            res_code = curl_easy_perform(curl);

            if (res_code != CURLE_OK)
            {
                return curl_easy_strerror(res_code);
            }

            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();

        return result;
    }
    vector<xmlNodePtr> select_xpath(xmlDocPtr html,
                                    xmlNodePtr node,
                                    string selector)
    {
        vector<xmlNodePtr> res;
        xmlXPathContextPtr context = xmlXPathNewContext(html);

        if (node != NULL)
            context->node = node;
        xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(reinterpret_cast<const xmlChar *>(selector.c_str()), context);

        if (xpathObj)
        {
            for (int i = 0; i < xpathObj->nodesetval->nodeNr; ++i)
            {
                xmlNodePtr node = xpathObj->nodesetval->nodeTab[i];
                res.push_back(node);
            }

            // Free the XPath object
            xmlXPathFreeObject(xpathObj);
        }
        xmlXPathFreeContext(context);
        return res;
    }

    string getDataValue(xmlDocPtr html,
                        xmlNodePtr node,
                        Data data)
    {
        string res;

        auto &[selector,
               multiple,
               name,
               children,
               subUrl] = data;
        for (auto &node : select_xpath(html, node, selector))
        {
            res += string(reinterpret_cast<char *>(node->content));
        }
        return res;
    }

    void serverside(const Pipe &pipe, json &parent, bool noParentProperty);

    void handleSubURL(xmlDocPtr html,
                      xmlNodePtr node,
                      Data data,
                      string url,
                      json parent)
    {
        auto selector = data.subUrl.urlSelector;
        auto anchor = selector == "self" ? node : select_xpath(html, node, selector)[0];

        string href = string(reinterpret_cast<char *>(xmlGetProp(anchor, (const xmlChar *)"href")));
        if (href.size() == 0)
        {
            throw "subUrl link not found for" + data.name;
        }
        const regex e("^http");

        if (!std::regex_search(href, e))
        {
            std::smatch match;
            std::regex originRegex("^(https?://[^/]+)");

            if (std::regex_search(url, match, originRegex))
            {
                std::string origin = match[1].str();
                href = origin + href;
            }
        }

        if (parent[data.name].is_null())
        {
            parent[data.name] = json::value_t::object;
        }
        else if (parent[data.name].is_string())
        {
            auto value = parent[data.name];
            parent[data.name] = json::value_t::object;
            parent[data.name]["value"] = value;
        }
        Pipe p;
        p.url = href;
        p.name = url;
        p.clientside = data.subUrl.clientside;
        move(data.subUrl.data.begin(), data.subUrl.data.end(), back_inserter(p.data));
        serverside(p, parent[data.name], true);
    }

    void resolveData(xmlDocPtr html,
                     xmlNodePtr node,
                     vector<Data> data,
                     string url,
                     json parent)
    {
        for (auto &d : data)
        {
            auto [selector,
                  multiple,
                  name,
                  children,
                  subUrl] = d;
            if (children.size() > 0)
            {
                if (multiple)
                {
                    for (auto &node : select_xpath(html, node, selector))
                    {
                        json o = json::value_t::object;
                        parent[name].push_back(o);
                        resolveData(html, node, children, url, o);
                    }
                }
                else
                {
                    if (parent[name].is_null())
                        parent[name] = json::value_t::object;
                    resolveData(html, node, children, url, parent);
                    if (subUrl.data.size() > 0)
                        handleSubURL(html, node, d, url, parent);
                }
            }
            else
            {
                parent[name] = getDataValue(html, node, d);
            }
        }
    }

    void clientside(const Pipe &pipe)
    {
        throw "client side not integrated";
    }

    void serverside(const Pipe &pipe, json &parent, bool noParentProperty = false)
    {
        auto [name,
              url,
              data,
              clientside] = pipe;

        string html_document = get_request(string(url));

        htmlDocPtr html = htmlReadMemory(html_document.c_str(), html_document.length(), nullptr, nullptr, HTML_PARSE_NOERROR);

        if (!noParentProperty && parent[name].is_null())
            parent[name] = json::value_t::object;
        resolveData(html, NULL, data, url, noParentProperty ? parent : parent[name]);
        processed.insert(url);
    }

    json crawl(vector<Pipe> &pipes)
    {
        // ThreadPool pool(thread::hardware_concurrency());
        ThreadPool pool(1);
        json result = json::value_t::object;

        auto *resultPtr = &result;
        int counter = 0;
        for (const auto &pipe : pipes)
        {

            pool.enqueue([pipe, resultPtr, &counter]
                         {
            if (pipe.clientside)
            {
                clientside(pipe);
            }
            else
            {
                try
                {
                    serverside(pipe, *resultPtr);
                }
                catch(const std::exception& e)
                {
                    std::cerr << e.what() << '\n';
                    throw e;
                }
            }
            cout << ++counter << endl; });
        }
        return result;
    }
} // namespace Crawler
