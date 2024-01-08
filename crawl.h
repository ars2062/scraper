#include <vector>
#include "structs.h"
#include <thread>
#include <set>
#include <map>
#include <regex>

#include "lib/json.hpp"
#include "lib/pool.h"
#include <indicators/progress_bar.hpp>
#include <indicators/cursor_control.hpp>

#include "libxml/HTMLparser.h"
#include "libxml/xpath.h"
#include "cpr/cpr.h"

#define LOG(x) cout << '"' << x << '"' << endl

template <typename T, typename F>
void logVector(
    const vector<T> &vector, F T::*field)
{
    // Printing all the elements
    // using <<
    for (auto element : vector)
    {
        LOG(element.*field);
    }
}

namespace Crawler
{
    using namespace nlohmann;
    using namespace Config;
    using namespace indicators;
    static inline void ltrim(std::string &s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
                                        { return !std::isspace(ch); }));
    }

    // trim from end (in place)
    static inline void rtrim(std::string &s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
                             { return !std::isspace(ch); })
                    .base(),
                s.end());
    }

    // trim from both ends (in place)
    static inline void trim(std::string &s)
    {
        rtrim(s);
        ltrim(s);
    }

    set<string> processed;
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
    {
        ((std::string *)userp)->append((char *)contents, size * nmemb);
        return size * nmemb;
    }

    std::string get_request(std::string word)
    {
        cpr::Response response = cpr::Get(cpr::Url{word});
        return response.text;
    }
    vector<xmlNodePtr> select_xpath(xmlXPathContextPtr ctxPtr,
                                    string selector)
    {

        vector<xmlNodePtr> res;
        xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(BAD_CAST selector.c_str(), ctxPtr);
        if (xpathObj == NULL)
            return res;
        if (xmlXPathNodeSetIsEmpty(xpathObj->nodesetval))
        {
            xmlXPathFreeObject(xpathObj);
            return res;
        }
        for (int i = 0; i < xpathObj->nodesetval->nodeNr; ++i)
        {
            xmlNodePtr node = xpathObj->nodesetval->nodeTab[i];
            res.push_back(node);
        }
        xmlXPathFreeObject(xpathObj);
        return res;
    }

    string getDataValue(xmlXPathContextPtr ctxPtr,
                        Data data)
    {
        string res = "";

        auto &[selector,
               multiple,
               name,
               children,
               subUrl] = data;
        auto nodes = select_xpath(ctxPtr, selector);
        for (auto &node : nodes)
        {
            res += string((char *)xmlNodeGetContent(node));
        }
        trim(res);
        return res;
    }

    void serverside(const Pipe &pipe, json *parent, bool noParentProperty);

    void handleSubURL(xmlXPathContextPtr ctxPtr,
                      Data data,
                      string url,
                      json *parent)
    {
        auto selector = data.subUrl.urlSelector;
        auto anchor = select_xpath(ctxPtr, selector)[0];

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

        auto prop = &parent->operator[](data.name);

        if (prop->is_null())
        {
            *prop = json::value_t::object;
        }
        else if (prop->is_string())
        {
            auto value = *prop;
            *prop = json::value_t::object;
            prop->operator[]("value") = value;
        }
        Pipe p;
        p.url = href;
        p.name = url;
        p.clientside = data.subUrl.clientside;
        move(data.subUrl.data.begin(), data.subUrl.data.end(), back_inserter(p.data));
        serverside(p, prop, true);
    }

    void resolveData(xmlXPathContextPtr ctxPtr,
                     vector<Data> data,
                     string url,
                     json *parent)
    {
        for (auto &d : data)
        {
            auto [selector,
                  multiple,
                  name,
                  children,
                  subUrl] = d;
            auto prop = &parent->operator[](name);

            if (children.size() > 0)
            {
                if (multiple)
                {
                    for (auto &node : select_xpath(ctxPtr, selector))
                    {
                        json o = json::value_t::object;
                        if (prop->is_null())
                            *prop = json::value_t::array;
                        auto newCtxPtr = xmlXPathNewContext(node->doc);
                        xmlNodePtr myParent = node->parent;
                        xmlNodePtr originalRootElement = xmlDocGetRootElement(node->doc);
                        xmlDocSetRootElement(node->doc, node);
                        resolveData(newCtxPtr, children, url, (&o));
                        if (subUrl.data.size() > 0)
                        {
                            handleSubURL(newCtxPtr, d, url, (&o));
                        }
                        prop->push_back(o);
                        xmlDocSetRootElement(originalRootElement->doc, originalRootElement);
                        xmlAddChild(myParent, node);
                        xmlXPathFreeContext(newCtxPtr);
                    }
                }
                else
                {
                    if (prop->is_null())
                        *prop = json::value_t::object;
                    resolveData(ctxPtr, children, url, parent);
                    if (subUrl.data.size() > 0)
                        handleSubURL(ctxPtr, d, url, parent);
                }
            }
            else
            {
                *prop = getDataValue(ctxPtr, d);
            }
        }
    }

    void clientside(const Pipe &pipe)
    {
        throw "client side not integrated";
    }

    void serverside(const Pipe &pipe, json *parent, bool noParentProperty = false)
    {
        auto [name,
              url,
              data,
              clientside] = pipe;

        string response = get_request(string(url));

        htmlDocPtr doc = htmlReadMemory(response.c_str(), response.length(), "", nullptr, HTML_PARSE_NOWARNING | HTML_PARSE_NOERROR | HTML_PARSE_RECOVER);
        xmlXPathContextPtr context = xmlXPathNewContext(doc);

        auto o = &parent->operator[](name);

        if (!noParentProperty && o->is_null())
            *o = json::value_t::object;
        resolveData(context, data, url, noParentProperty ? parent : o);
        xmlXPathFreeContext(context);
        xmlFreeDoc(doc);
        processed.insert(url);
    }

    json crawl(vector<Pipe> &pipes)
    {
        show_console_cursor(false);
        ThreadPool pool(thread::hardware_concurrency());
        json result = json::value_t::object;
        indicators::ProgressBar p{
            option::BarWidth{50},
            option::Start{"["},
            option::Fill{"■"},
            option::Lead{"■"},
            option::Remainder{" "},
            option::End{"]"},
            option::ForegroundColor{indicators::Color::green},
            option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}}};

        auto *resultPtr = &result;
        int counter = 0;
        int max = pipes.size();
        mutex m;
        for (const auto &pipe : pipes)
        {
            pool.enqueue([pipe, max, resultPtr, &counter, &p, &m]
                         {
            if (pipe.clientside)
            {
                clientside(pipe);
            }
            else
            {
                // try
                // {
                serverside(pipe, resultPtr);
                // }
                // catch (const std::exception &e)
                // {
                //     std::cerr << e.what() << '\n';
                //     throw e;
                // }
            }
            
            m.lock();
            counter++;
            auto progress = counter / (float)max * 100;
            stringstream s;
            s.precision(3);
            s<<progress;
            auto text = to_string(counter) + "/" + to_string(max) + " " + s.str() + "%";
            p.set_option(option::PostfixText{text});
            p.set_progress(progress);
            m.unlock(); });
        }
        pool.~ThreadPool();
        p.mark_as_completed();
        show_console_cursor(true);
        return result;
    }
} // namespace Crawler
